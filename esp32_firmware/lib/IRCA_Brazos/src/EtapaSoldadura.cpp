#include "EtapaSoldadura.h"

// [base, hombro, codo] - 90 = reposo. Mismas poses que ETAPAS[1] del HMI.
static const uint8_t POSES[][3] = {
  {90, 90, 90},
  {70, 75, 110},
  {70, 60, 125},
  {110, 75, 110},
  {110, 60, 125},
  {90, 90, 90},
};
static const uint8_t NUM_POSES = sizeof(POSES) / sizeof(POSES[0]);

EtapaSoldadura::EtapaSoldadura(Brazo &n1, Brazo &n2) {
  _b[0] = &n1;
  _b[1] = &n2;
}

void EtapaSoldadura::enviarPaso() {
  for (uint8_t i = 0; i < NUM_BRAZOS; i++)
    _b[i]->moverEn(POSES[_paso][0], POSES[_paso][1], POSES[_paso][2], _duracionPasoMs);
}

void EtapaSoldadura::Soldadura() {
  bool llegaron = true;
  for (uint8_t i = 0; i < NUM_BRAZOS; i++)
    if (!_b[i]->actualizar()) llegaron = false;

  if (_estado != EstadoEtapa::MARCHA || !llegaron) return;

  if (_paso >= NUM_POSES - 1) {      // ultima pose alcanzada: ciclo completo
    _paso = 0;
    _estado = EstadoEtapa::PARO;
    _fin = true;
    return;
  }
  _paso++;
  enviarPaso();
}

void EtapaSoldadura::iniciar() {
  if (_estado == EstadoEtapa::EMERGENCIA) return;   // primero reiniciar()
  if (_paso == 0) _paso = 1;
  _estado = EstadoEtapa::MARCHA;
  enviarPaso();
}

void EtapaSoldadura::pausar() {
  if (_estado != EstadoEtapa::MARCHA) return;
  _estado = EstadoEtapa::PARO;
  for (uint8_t i = 0; i < NUM_BRAZOS; i++) _b[i]->congelar();
}

void EtapaSoldadura::paroEmergencia() {
  _estado = EstadoEtapa::EMERGENCIA;
  for (uint8_t i = 0; i < NUM_BRAZOS; i++) _b[i]->congelar();
}

void EtapaSoldadura::reiniciar() {
  _paso = 0;
  _fin = false;
  _estado = EstadoEtapa::PARO;
  for (uint8_t i = 0; i < NUM_BRAZOS; i++) _b[i]->reposo(2000);
}

bool EtapaSoldadura::manual(uint8_t n, float b, float h, float c) {
  if (_estado != EstadoEtapa::PARO) return false;
  for (uint8_t i = 0; i < NUM_BRAZOS; i++) {
    if (_b[i]->numero() == n) { _b[i]->moverEn(b, h, c, 500); return true; }
  }
  return false;
}

bool EtapaSoldadura::tomarFin() {
  bool f = _fin;
  _fin = false;
  return f;
}

const __FlashStringHelper *EtapaSoldadura::nombreEstado() const {
  switch (_estado) {
    case EstadoEtapa::MARCHA:     return F("MARCHA");
    case EstadoEtapa::EMERGENCIA: return F("EMERGENCIA");
    default:                      return F("PARO");
  }
}
