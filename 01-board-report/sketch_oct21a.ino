#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_mac.h"
#include <WiFi.h>

void line() { Serial.println("----------------------------------------"); }

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n\n========== ESP32 BOARD REPORT ==========");

  // --- Chip ---
  esp_chip_info_t chip;
  esp_chip_info(&chip);
  line();
  Serial.println("CHIP");
  Serial.printf("  Model          : %s\n", ESP.getChipModel());
  Serial.printf("  Revision       : %d\n", ESP.getChipRevision());
  Serial.printf("  Cores          : %d\n", chip.cores);
  Serial.printf("  CPU freq       : %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("  Features       : %s%s%s%s\n",
    (chip.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi " : "",
    (chip.features & CHIP_FEATURE_BT)       ? "BT "   : "",
    (chip.features & CHIP_FEATURE_BLE)      ? "BLE "  : "",
    (chip.features & CHIP_FEATURE_EMB_FLASH)? "EmbFlash " : "");

  // --- Flash ---
  line();
  Serial.println("FLASH");
  Serial.printf("  Flash size     : %u MB\n", ESP.getFlashChipSize() / (1024*1024));
  Serial.printf("  Flash speed    : %u MHz\n", ESP.getFlashChipSpeed() / 1000000);

  // --- Memory ---
  line();
  Serial.println("MEMORY");
  Serial.printf("  Total heap     : %u bytes\n", ESP.getHeapSize());
  Serial.printf("  Free heap      : %u bytes\n", ESP.getFreeHeap());
  Serial.printf("  Min free heap  : %u bytes\n", ESP.getMinFreeHeap());
  Serial.printf("  PSRAM size     : %u bytes\n", ESP.getPsramSize());   // 0 on WROOM-32
  Serial.printf("  Free PSRAM     : %u bytes\n", ESP.getFreePsram());

  // --- SDK / firmware ---
  line();
  Serial.println("FIRMWARE");
  Serial.printf("  ESP-IDF ver    : %s\n", esp_get_idf_version());
  Serial.printf("  Sketch size    : %u bytes\n", ESP.getSketchSize());
  Serial.printf("  Free sketch sp : %u bytes\n", ESP.getFreeSketchSpace());

  // --- Identity ---
  line();
  Serial.println("IDENTITY");
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  Serial.printf("  WiFi STA MAC   : %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  esp_read_mac(mac, ESP_MAC_BT);
  Serial.printf("  Bluetooth MAC  : %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  uint64_t efuse = ESP.getEfuseMac();
  Serial.printf("  eFuse MAC (ID) : %012llX\n", efuse);

  line();
  Serial.println("========== END OF REPORT ==========");
}

void loop() {}