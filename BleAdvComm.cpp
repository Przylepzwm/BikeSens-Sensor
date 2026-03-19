#include "BleAdvComm.h"

void BleAdvComm::begin(uint16_t deviceId16) {
  if (_inited) return;

  char name[16];
  snprintf(name, sizeof(name), "BS-%04X", deviceId16);

  NimBLEDevice::init(name);
  NimBLEDevice::setPower(ESP_PWR_LVL_N0);

  _adv = NimBLEDevice::getAdvertising();
  _adv->setMinInterval(BLE_ADV_MIN_INTERVAL);
  _adv->setMaxInterval(BLE_ADV_MAX_INTERVAL);
  _adv->setName(name);
  _adv->enableScanResponse(false);
  _adv->setConnectableMode(BLE_GAP_CONN_MODE_NON);
  _adv->setDiscoverableMode(BLE_GAP_DISC_MODE_GEN);

  _inited = true;
}

void BleAdvComm::setTelegram(const BleTelegram& t) {
  _telegram = t;
  _dirty = true;
}

void BleAdvComm::start() {
  if (_running) return;

  if (_dirty) {
    NimBLEAdvertisementData ad;

    // [company_id LE(2)][prefix0][prefix1][device_id LE][seq LE][pulses LE][bat%]
    uint8_t buf[2 + 2 + 2 + 2 + 2 + 1 + 1];
    size_t i = 0;

    buf[i++] = (uint8_t)(BLE_COMPANY_ID & 0xFF);
    buf[i++] = (uint8_t)(BLE_COMPANY_ID >> 8);
    buf[i++] = BLE_PREFIX0;
    buf[i++] = BLE_PREFIX1;

    writeLE16(buf, i, _telegram.device_id); i += 2;
    writeLE16(buf, i, _telegram.seq); i += 2;
    writeLE16(buf, i, _telegram.pulses_in_window); i += 2;
    buf[i++] = _telegram.battery_percent;

    ad.setManufacturerData(std::string((char*)buf, i));
    _adv->setAdvertisementData(ad);
    _dirty = false;
  }

  _adv->start();
  _running = true;
}

void BleAdvComm::stop() {
  if (!_inited || !_running) return;
  _adv->stop();
  _running = false;
}

void BleAdvComm::writeLE16(uint8_t* dst, size_t off, uint16_t v) {
  dst[off + 0] = (uint8_t)(v & 0xFF);
  dst[off + 1] = (uint8_t)(v >> 8);
}
