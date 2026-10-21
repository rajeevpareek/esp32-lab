/*
 * Probe-Request Harvester — captures the network names devices are searching for
 * When a phone/laptop looks for Wi-Fi, it broadcasts the SSIDs it remembers,
 * in the clear. This logs them per device.
 * Board: ESP32 DevKit (WROOM-32)  |  115200 baud
 *
 * PASSIVE ONLY. Your own airspace. The leaked names are broadcast openly by
 * the devices themselves — observe them, don't use them to target anyone.
 */
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <string.h>

#define MAX_ENTRIES 120
struct Probe {
  uint8_t mac[6];
  char    ssid[33];
  int8_t  rssi;
  uint16_t count;
};
Probe entries[MAX_ENTRIES];
int entryCount = 0;
uint32_t totalProbes = 0;

const char* vendor(const uint8_t* m) {
  if (m[0] & 0x02) return "rand";
  uint32_t o = (m[0]<<16)|(m[1]<<8)|m[2];
  switch (o) {
    case 0x149877: return "Apple";
    case 0x8C147D: case 0x746DFA: return "Samsung";
    default: return "";
  }
}

// store a unique (MAC, SSID) pair; bump count if seen again
void store(const uint8_t* mac, const char* ssid, int8_t rssi) {
  for (int i = 0; i < entryCount; i++)
    if (memcmp(entries[i].mac, mac, 6) == 0 && strcmp(entries[i].ssid, ssid) == 0) {
      entries[i].count++; entries[i].rssi = rssi; return;
    }
  if (entryCount >= MAX_ENTRIES) return;
  memcpy(entries[entryCount].mac, mac, 6);
  strncpy(entries[entryCount].ssid, ssid, 32);
  entries[entryCount].ssid[32] = 0;
  entries[entryCount].rssi = rssi;
  entries[entryCount].count = 1;
  entryCount++;
}

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;
  const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  const uint8_t* p = pkt->payload;
  int len = pkt->rx_ctrl.sig_len;
  if (len < 26 || pkt->rx_ctrl.rssi > 0) return;

  uint8_t subtype = (p[0] >> 4) & 0x0F;
  if (subtype != 4) return;              // 4 = probe request only

  totalProbes++;
  const uint8_t* src = p + 10;           // transmitter MAC
  uint8_t tag = p[24], tlen = p[25];     // tag 0 = SSID
  if (tag == 0 && tlen > 0 && tlen <= 32 && 26 + tlen <= len) {
    char ssid[33];
    memcpy(ssid, p + 26, tlen); ssid[tlen] = 0;
    store(src, ssid, pkt->rx_ctrl.rssi);
  }
  // tlen == 0 is a "wildcard" probe (any network) — we skip those, no name leaked
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
  wifi_promiscuous_filter_t f = { .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT };
  esp_wifi_set_promiscuous_filter(&f);   // only management frames — less noise
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  Serial.println("Probe harvester running. Wake a phone's Wi-Fi nearby...");
}

uint8_t hops[] = {1, 6, 11}, hi = 0;
uint32_t lastHop = 0, lastPrint = 0;

void loop() {
  if (millis() - lastHop > 300) {
    hi = (hi + 1) % 3;
    esp_wifi_set_channel(hops[hi], WIFI_SECOND_CHAN_NONE);
    lastHop = millis();
  }
  if (millis() - lastPrint > 5000) {
    Serial.printf("\n==== %lu probes | %d unique (device, network) pairs ====\n",
                  totalProbes, entryCount);
    Serial.println("device MAC         vnd    seen  rssi  looking-for");
    for (int i = 0; i < entryCount; i++) {
      Probe& e = entries[i];
      Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x  %-5s  %4u  %4d  %s\n",
        e.mac[0],e.mac[1],e.mac[2],e.mac[3],e.mac[4],e.mac[5],
        vendor(e.mac), e.count, e.rssi, e.ssid);
    }
    lastPrint = millis();
  }
}
