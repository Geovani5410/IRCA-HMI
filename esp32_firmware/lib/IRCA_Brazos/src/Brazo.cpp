#include "Brazo.h"

void Brazo::begin(PCA9685Driver &pca, uint8_t canalBase) {
  _servo[0].begin(pca, ConfigMG996R(canalBase));       // base
  _servo[1].begin(pca, ConfigMG996R(canalBase + 1));   // hombro
  _servo[2].begin(pca, ConfigMG90S(canalBase + 2));    // codo
  irA(90, 90, 90);
}

void Brazo::irA(float b, float h, float c) {
  float v[3] = {b, h, c};
  for (uint8_t j = 0; j < 3; j++) {
    _actual[j] = _ini[j] = _fin[j] = _servo[j].limitar(v[j]);
    _servo[j].escribir(_actual[j]);
  }
  _llego = true;
}

void Brazo::moverEn(float b, float h, float c, uint32_t ms) {
  if (ms == 0) { irA(b, h, c); return; }
  float v[3] = {b, h, c};
  for (uint8_t j = 0; j < 3; j++) {
    _ini[j] = _actual[j];
    _fin[j] = _servo[j].limitar(v[j]);
  }
  _t0 = millis();
  _dur = ms;
  _ultimo = _t0 - PERIODO_MS;   // permite actualizar de inmediato
  _llego = false;
}

void Brazo::congelar() {
  for (uint8_t j = 0; j < 3; j++) _ini[j] = _fin[j] = _actual[j];
  _llego = true;
}

void Brazo::liberar() {
  for (uint8_t j = 0; j < 3; j++) _servo[j].liberar();
}

bool Brazo::actualizar() {
  if (_llego) return true;
  uint32_t ahora = millis();
  if (ahora - _ultimo < PERIODO_MS) return false;
  _ultimo = ahora;

  float f = (float)(ahora - _t0) / (float)_dur;
  if (f >= 1.0f) { f = 1.0f; _llego = true; }
  float s = f * f * (3.0f - 2.0f * f);           // suavizado (igual que la simulacion)
  for (uint8_t j = 0; j < 3; j++) {
    _actual[j] = _ini[j] + (_fin[j] - _ini[j]) * s;
    _servo[j].escribir(_actual[j]);
  }
  return _llego;
}
