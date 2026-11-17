#include "ServoPCA.h"

void ServoPCA::begin(PCA9685Driver &pca, const ConfigServo &c) {
  _pca = &pca;
  cfg = c;
}

float ServoPCA::limitar(float g) const {
  if (g < cfg.limMin) g = cfg.limMin;
  if (g > cfg.limMax) g = cfg.limMax;
  return g;
}

void ServoPCA::escribir(float grados) {
  if (!_pca) return;
  _grados = limitar(grados);
  float f = cfg.invertido ? 180.0f - _grados : _grados;
  f += cfg.offsetGrados;
  if (f < 0) f = 0;
  if (f > 180) f = 180;
  uint16_t us = cfg.minUs + (uint16_t)((float)(cfg.maxUs - cfg.minUs) * (f / 180.0f) + 0.5f);
  _pca->setPulsoUs(cfg.canal, us);
}

void ServoPCA::liberar() {
  if (_pca) _pca->apagarCanal(cfg.canal);
}
