/*
 * Wi-Fi Deep Scan — maximum info extraction
 * SSID, RSSI, channel, encryption, BSSID, vendor (OUI), hidden flag.
 * Board: ESP32 DevKit (WROOM-32)  |  Serial: 115200 baud
 * 2.4 GHz only.
 */
#include <WiFi.h>

const char* enc(wifi_auth_mode_t m) {
  switch (m) {
    case WIFI_AUTH_OPEN:            return "OPEN";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Ent";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/WPA3";
    default:                        return "?";
  }
}

// a few common OUIs (first 3 bytes of BSSID). Extend as you like.
const char* vendorOUI(const uint8_t* b) {
  uint32_t oui = (b[0] << 16) | (b[1] << 8) | b[2];
  switch (oui) {
    case 0x746DFA: return "Samsung";
    case 0x5CA6E6: return "TP-Link";
    case 0xC46E1F: return "TP-Link";
    case 0x001A11: return "Google";
    case 0xACDE48: return "Apple";
    case 0x3C84A6: return "Netgear";
    case 0xD4CA6D: return "Routerboard";
    case 0x001E42: return "Mikrotik";
    default:       return "";
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.println("Wi-Fi deep scanner ready");
}

void loop() {
  Serial.println("\nScanning (incl. hidden)...");
  int n = WiFi.scanNetworks(false /*async*/, true /*show hidden*/);
  Serial.printf("%d networks found\n\n", n);

  // simple signal strength bar + full details
  for (int i = 0; i < n; i++) {
    const uint8_t* bssid = WiFi.BSSID(i);
    const char* ven = vendorOUI(bssid);
    String ssid = WiFi.SSID(i);
    bool hidden = (ssid.length() == 0);

    Serial.printf("[%2d] %-24s %s\n", i + 1,
                  hidden ? "<hidden>" : ssid.c_str(),
                  hidden ? "(no SSID broadcast)" : "");
    Serial.printf("     RSSI %d dBm | ch %d | %s\n",
                  WiFi.RSSI(i), WiFi.channel(i), enc(WiFi.encryptionType(i)));
    Serial.printf("     BSSID %s%s%s\n",
                  WiFi.BSSIDstr(i).c_str(),
                  strlen(ven) ? "  vendor: " : "", ven);
    Serial.println();
  }

  // detect repeaters/mesh: same SSID on multiple BSSIDs
  Serial.println("(Tip: same SSID with different BSSIDs = mesh/repeater)");

  WiFi.scanDelete();
  delay(8000);
}
