/*
 * Wi-Fi Capture Tool v4 — full passive 802.11 analyzer
 * ----------------------------------------------------
 *  - Decodes frame types/subtypes (beacon, probe, assoc, deauth, data...)
 *  - Maps client <-> AP associations (reads all 3 MAC addresses)
 *  - Extracts SSIDs from beacons and probe requests
 *  - Labels vendor from OUI; flags randomised MACs
 *  - Filters corrupt frames
 *  Board: ESP32 DevKit (WROOM-32)  |  115200 baud
 *  PASSIVE ONLY — reads headers, decrypts nothing, transmits nothing.
 */
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <string.h>

volatile uint32_t totalFrames = 0, droppedFrames = 0;

// ---- device table ----
#define MAX_DEV 80
struct Dev {
  uint8_t  mac[6];
  uint32_t frames;
  int8_t   rssi;
  uint8_t  channel;
  uint8_t  apMac[6];      // the AP this device is associated with (if known)
  bool     isAP;          // true if we've seen it send a beacon
  char     ssid[33];      // SSID if AP (from beacon) or last probe
  uint16_t beacons, probes, data, deauths, other;
};
Dev devs[MAX_DEV];
int devCount = 0;

// ---- OUI vendor lookup ----
const char* ouiVendor(const uint8_t* m) {
  if (m[0] & 0x02) return "randomised";
  uint32_t o = (m[0] << 16) | (m[1] << 8) | m[2];
  switch (o) {
    case 0x149877: return "Apple";
    case 0x8C147D: case 0x746DFA: return "Samsung";
    case 0xB0BE76: return "ISP-router";
    case 0xE0D362: return "router-ext";
    case 0x60BD2C: case 0x62BD2C: return "Airtel";
    case 0x6C4F89: case 0x8CA399: case 0xB4A7C6:
    case 0x200C86: case 0xF0EDB8: case 0x5437BB:
    case 0x5637BB: return "router";
    default:       return "";
  }
}

int findDev(const uint8_t* mac) {
  for (int i = 0; i < devCount; i++)
    if (memcmp(devs[i].mac, mac, 6) == 0) return i;
  if (devCount >= MAX_DEV) return -1;
  int n = devCount++;
  memset(&devs[n], 0, sizeof(Dev));
  memcpy(devs[n].mac, mac, 6);
  return n;
}

const char* subtypeName(uint8_t ftype, uint8_t fsub) {
  if (ftype == 0) { // management
    switch (fsub) {
      case 0x0: return "assoc-req";
      case 0x1: return "assoc-resp";
      case 0x4: return "probe-req";
      case 0x5: return "probe-resp";
      case 0x8: return "beacon";
      case 0xA: return "disassoc";
      case 0xB: return "auth";
      case 0xC: return "deauth";
      default:  return "mgmt";
    }
  } else if (ftype == 2) return "data";
  else if (ftype == 1)   return "ctrl";
  return "?";
}

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) {
  totalFrames++;
  const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  const uint8_t* p = pkt->payload;
  int len = pkt->rx_ctrl.sig_len;
  int8_t rssi = pkt->rx_ctrl.rssi;
  uint8_t ch = pkt->rx_ctrl.channel;

  // ---- drop corrupt frames ----
  if (len < 24 || rssi > 0 || ch < 1 || ch > 14) { droppedFrames++; return; }

  uint8_t fctrl = p[0];
  uint8_t ftype = (fctrl >> 2) & 0x03;    // 0=mgmt 1=ctrl 2=data
  uint8_t fsub  = (fctrl >> 4) & 0x0F;

  // 802.11 addresses: addr1=receiver(p+4), addr2=transmitter(p+10), addr3=BSSID(p+16)
  const uint8_t* rx  = p + 4;
  const uint8_t* tx  = p + 10;
  const uint8_t* bss = p + 16;

  int i = findDev(tx);
  if (i < 0) return;
  Dev& d = devs[i];
  d.frames++;
  d.rssi = rssi;
  d.channel = ch;

  // count by subtype
  if (ftype == 0 && fsub == 0x8) d.beacons++;
  else if (ftype == 0 && fsub == 0x4) d.probes++;
  else if (ftype == 0 && fsub == 0xC) d.deauths++;
  else if (ftype == 2) d.data++;
  else d.other++;

  // beacon → this transmitter is an AP; grab its SSID
  if (ftype == 0 && fsub == 0x8 && len >= 38) {
    d.isAP = true;
    uint8_t tag = p[36], tlen = p[37];     // first tagged param = SSID
    if (tag == 0 && tlen > 0 && tlen <= 32 && 38 + tlen <= len) {
      memcpy(d.ssid, p + 38, tlen); d.ssid[tlen] = 0;
    }
  }

  // probe request → grab the SSID being searched for
  if (ftype == 0 && fsub == 0x4 && len >= 26) {
    uint8_t tag = p[24], tlen = p[25];
    if (tag == 0 && tlen > 0 && tlen <= 32 && 26 + tlen <= len) {
      memcpy(d.ssid, p + 26, tlen); d.ssid[tlen] = 0;
    }
  }

  // data frame → record which AP this client is talking to (addr3 = BSSID)
  if (ftype == 2) {
    // if transmitter != BSSID, transmitter is a client associated to BSSID
    if (memcmp(tx, bss, 6) != 0)
      memcpy(d.apMac, bss, 6);
  }
}

void printMac(const uint8_t* m) {
  Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x", m[0],m[1],m[2],m[3],m[4],m[5]);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  nvs_flash_init(); esp_netif_init(); esp_event_loop_create_default();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  wifi_promiscuous_filter_t f = { .filter_mask = WIFI_PROMIS_FILTER_MASK_ALL };
  esp_wifi_set_promiscuous_filter(&f);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  Serial.println("Capture tool v4 running...");
}

uint8_t hops[] = {1, 6, 11}, hi = 0;
uint32_t lastHop = 0, lastPrint = 0;

void loop() {
  if (millis() - lastHop > 500) {
    hi = (hi + 1) % 3;
    esp_wifi_set_channel(hops[hi], WIFI_SECOND_CHAN_NONE);
    lastHop = millis();
  }

  if (millis() - lastPrint > 8000) {
    Serial.printf("\n======== frames:%lu dropped:%lu devices:%d ch:%d ========\n",
                  totalFrames, droppedFrames, devCount, hops[hi]);

    // --- Access Points ---
    Serial.println("\n-- ACCESS POINTS --");
    Serial.println("BSSID              rssi ch  vendor       SSID");
    for (int i = 0; i < devCount; i++) {
      Dev& d = devs[i];
      if (!d.isAP) continue;
      printMac(d.mac);
      Serial.printf("  %4d %2d  %-11s  %s\n", d.rssi, d.channel, ouiVendor(d.mac), d.ssid);
    }

    // --- Clients (with their AP, if known) ---
    Serial.println("\n-- CLIENTS --");
    Serial.println("MAC                rssi ch  vendor       data  -> AP / probing-for");
    for (int i = 0; i < devCount; i++) {
      Dev& d = devs[i];
      if (d.isAP) continue;
      printMac(d.mac);
      Serial.printf("  %4d %2d  %-11s  %4lu  ", d.rssi, d.channel, ouiVendor(d.mac), d.data);
      bool hasAP = false;
      for (int b = 0; b < 6; b++) if (d.apMac[b]) hasAP = true;
      if (hasAP) { Serial.print("-> "); printMac(d.apMac); }
      else if (d.ssid[0]) Serial.printf("probe:%s", d.ssid);
      Serial.println();
    }

    // --- Security-relevant frame counts ---
    uint32_t totalDeauth = 0;
    for (int i = 0; i < devCount; i++) totalDeauth += devs[i].deauths;
    Serial.printf("\n[deauth frames seen: %lu]\n", totalDeauth);

    lastPrint = millis();
  }
}
