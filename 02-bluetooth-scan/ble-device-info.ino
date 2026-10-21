/*
 * BLE Deep Scan — maximum passive extraction
 * Lists every field in each device's advertisement:
 * address (+type), RSSI, name, TX power, appearance,
 * service UUIDs (named), manufacturer vendor + raw hex.
 *
 * Board: ESP32 DevKit (WROOM-32)  |  Serial: 115200 baud
 * Passive only — no connection, no pairing.
 */
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

BLEScan* scanner;

// --- Bluetooth SIG company IDs (first 2 bytes of manufacturer data) ---
const char* vendor(uint16_t id) {
  switch (id) {
    case 0x004C: return "Apple";
    case 0x0075: return "Samsung";
    case 0x00E0: return "Google";
    case 0x0006: return "Microsoft";
    case 0x0059: return "Nordic";
    case 0x0157: return "Huawei";
    case 0x038F: return "Xiaomi";
    case 0x0171: return "Amazon";
    case 0x00D7: return "Qualcomm";
    case 0x0499: return "Ruuvi";
    default:     return "unknown";
  }
}

// --- common 16-bit GATT service UUIDs ---
const char* serviceName(uint16_t u) {
  switch (u) {
    case 0x1800: return "Generic Access";
    case 0x1801: return "Generic Attribute";
    case 0x180A: return "Device Information";
    case 0x180F: return "Battery";
    case 0x180D: return "Heart Rate";
    case 0x1812: return "HID (keyboard/mouse)";
    case 0x181A: return "Environmental Sensing";
    case 0x1809: return "Health Thermometer";
    case 0xFE9F: return "Google Fast Pair";
    case 0xFD6F: return "Exposure Notification";
    case 0xFEAA: return "Eddystone beacon";
    default:     return nullptr;
  }
}

// --- Apple manufacturer sub-type (3rd byte of Apple mfr data) ---
const char* appleType(uint8_t t) {
  switch (t) {
    case 0x05: return "AirDrop";
    case 0x07: return "Proximity-pair (AirPods/setup)";
    case 0x09: return "AirPlay";
    case 0x0A: return "AirPlay-target";
    case 0x0C: return "Handoff";
    case 0x10: return "Nearby-info (iPhone/Watch/Mac)";
    case 0x12: return "Find My";
    default:   return "Apple-other";
  }
}

class Found : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice d) {
    Serial.printf("\n%s  [%s]  %d dBm\n",
      d.getAddress().toString().c_str(),
      d.getAddressType() == BLE_ADDR_TYPE_PUBLIC ? "public/real" : "random",
      d.getRSSI());

    if (d.haveName())
      Serial.printf("  name       : %s\n", d.getName().c_str());
    if (d.haveTXPower())
      Serial.printf("  txPower    : %d dBm\n", d.getTXPower());
    if (d.haveAppearance())
      Serial.printf("  appearance : 0x%04X\n", d.getAppearance());

    if (d.haveServiceUUID()) {
      String u = d.getServiceUUID().toString().c_str();
      Serial.printf("  service    : %s", u.c_str());
      if (u.length() >= 8) {
        uint16_t s = strtol(u.substring(4, 8).c_str(), nullptr, 16);
        const char* n = serviceName(s);
        if (n) Serial.printf("  (%s)", n);
      }
      Serial.println();
    }

    if (d.haveServiceData())
      Serial.printf("  svcData    : %d bytes\n", (int)d.getServiceData().length());

    if (d.haveManufacturerData()) {
      String m = d.getManufacturerData();
      uint16_t company = (uint8_t)m[0] | ((uint8_t)m[1] << 8);
      Serial.printf("  mfr        : %s (0x%04X), %d bytes\n",
                    vendor(company), company, m.length());
      if (company == 0x004C && m.length() >= 3)
        Serial.printf("  apple-type : %s\n", appleType((uint8_t)m[2]));
      Serial.print("  raw        : ");
      for (int i = 0; i < m.length(); i++) Serial.printf("%02X", (uint8_t)m[i]);
      Serial.println();
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(300);
  BLEDevice::init("");
  scanner = BLEDevice::getScan();
  scanner->setAdvertisedDeviceCallbacks(new Found());
  scanner->setActiveScan(true);     // also requests scan-response (names)
  scanner->setInterval(100);
  scanner->setWindow(99);
  Serial.println("BLE deep scan (passive)...");
}

void loop() {
  BLEScanResults* r = scanner->start(6, false);
  Serial.printf("\n==== %d devices this round ====\n", r->getCount());
  scanner->clearResults();
  delay(1500);
}
