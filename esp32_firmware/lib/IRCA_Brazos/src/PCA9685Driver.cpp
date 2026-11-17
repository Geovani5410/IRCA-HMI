#include "PCA9685Driver.h"

static const uint8_t REG_MODE1     = 0x00;
static const uint8_t REG_MODE2     = 0x01;
static const uint8_t REG_LED0_ON_L = 0x06;
static const uint8_t REG_ALL_OFF_H = 0xFD;
static const uint8_t REG_PRESCALE  = 0xFE;

static const uint8_t MODE1_SLEEP   = 0x10;
static const uint8_t MODE1_AI      = 0x20;
static const uint8_t MODE1_RESTART = 0x80;
static const uint8_t MODE2_OUTDRV  = 0x04;

PCA9685Driver::PCA9685Driver(uint8_t direccion, TwoWire &wire)
  : _dir(direccion), _wire(&wire), _hz(50.0f) {}

void PCA9685Driver::escribir8(uint8_t reg, uint8_t val) {
  _wire->beginTransmission(_dir);
  _wire->write(reg);
  _wire->write(val);
  _wire->endTransmission();
}

uint8_t PCA9685Driver::leer8(uint8_t reg) {
  _wire->beginTransmission(_dir);
  _wire->write(reg);
  _wire->endTransmission();
  _wire->requestFrom(_dir, (uint8_t)1);
  return _wire->available() ? _wire->read() : 0;
}

bool PCA9685Driver::begin(float frecuenciaHz) {
  _wire->beginTransmission(_dir);
  if (_wire->endTransmission() != 0) return false;

  _hz = frecuenciaHz;
  escribir8(REG_MODE1, MODE1_RESTART);          // reset
  delay(10);
  escribir8(REG_MODE2, MODE2_OUTDRV);           // salidas totem-pole

  // prescale = osc / (4096 * f) - 1   (osc interno = 25 MHz)
  uint8_t prescale = (uint8_t)(25000000.0f / (4096.0f * _hz) + 0.5f) - 1;
  uint8_t viejo = leer8(REG_MODE1);
  escribir8(REG_MODE1, (viejo & ~MODE1_RESTART) | MODE1_SLEEP);  // dormir para cambiar prescale
  escribir8(REG_PRESCALE, prescale);
  escribir8(REG_MODE1, viejo);
  delay(5);
  escribir8(REG_MODE1, viejo | MODE1_RESTART | MODE1_AI);        // despertar + auto-incremento
  return true;
}

void PCA9685Driver::setPulsoUs(uint8_t canal, uint16_t us) {
  if (canal > 15) return;
  uint32_t ticks = (uint32_t)(us * _hz * 4096.0f / 1000000.0f + 0.5f);
  if (ticks > 4095) ticks = 4095;
  _wire->beginTransmission(_dir);
  _wire->write((uint8_t)(REG_LED0_ON_L + 4 * canal));
  _wire->write((uint8_t)0);                  // ON_L
  _wire->write((uint8_t)0);                  // ON_H
  _wire->write((uint8_t)(ticks & 0xFF));     // OFF_L
  _wire->write((uint8_t)(ticks >> 8));       // OFF_H
  _wire->endTransmission();
}

void PCA9685Driver::apagarCanal(uint8_t canal) {
  if (canal > 15) return;
  _wire->beginTransmission(_dir);
  _wire->write((uint8_t)(REG_LED0_ON_L + 4 * canal));
  _wire->write((uint8_t)0);
  _wire->write((uint8_t)0);
  _wire->write((uint8_t)0);
  _wire->write((uint8_t)0x10);               // bit "full off"
  _wire->endTransmission();
}

void PCA9685Driver::apagarTodos() {
  escribir8(REG_ALL_OFF_H, 0x10);
}
