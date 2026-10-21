/*
 * BLE Scan — lists nearby Bluetooth LE devices
 * Board: ESP32 DevKit (WROOM-32)  |  Serial: 115200 baud
 */
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

BLEScan* scanner;

class Found : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice d) {
    Serial.printf("%-18s  %4d dBm  %s%s\n",
      d.getAddress().toString().c_str(),
      d.getRSSI(),
      d.haveName() ? d.getName().c_str() : "(no name)",
      d.haveServiceUUID() ? "  [has services]" : "");
  }
};

void setup() {
  Serial.begin(115200);
  delay(300);
  BLEDevice::init("");
  scanner = BLEDevice::getScan();
  scanner->setAdvertisedDeviceCallbacks(new Found());
  scanner->setActiveScan(true);     // also asks devices for their name
  scanner->setInterval(100);
  scanner->setWindow(99);
  Serial.println("BLE scanning...");
}

void loop() {
  BLEScanResults* r = scanner->start(5, false);   // 5-second scan
  Serial.printf("---- %d devices this round ----\n\n", r->getCount());
  scanner->clearResults();
  delay(1000);
}
