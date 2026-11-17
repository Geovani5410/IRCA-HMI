#include "PuenteSerial.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

void PuenteSerial::begin(unsigned long baud) {
  _s.begin(baud);
  for (uint8_t i = 0; i < 6; i++)
    for (uint8_t j = 0; j < 3; j++) _enviado[i][j] = -1.0f;
}

void PuenteSerial::log(const __FlashStringHelper *msg) {
  _s.print(F("# "));
  _s.println(msg);
}

void PuenteSerial::log(const char *msg) {
  _s.print(F("# "));
  _s.println(msg);
}

bool PuenteSerial::leerLinea() {
  while (_s.available()) {
    char c = (char)_s.read();
    if (c == '\r') continue;
    if (c == '\n') {
      bool valida = !_descartar && _n > 0;
      _buf[_n] = 0;
      _n = 0;
      _descartar = false;
      if (valida) return true;
      continue;
    }
    if (_n < sizeof(_buf) - 1) _buf[_n++] = c;
    else _descartar = true;               // linea demasiado larga: se ignora completa
  }
  return false;
}

void PuenteSerial::atender(EtapaSoldadura &etapa) {
  if (leerLinea()) procesar(etapa);
}

void PuenteSerial::procesar(EtapaSoldadura &etapa) {
  char *tok = strtok(_buf, " ");
  if (!tok) return;
  for (char *p = tok; *p; ++p) *p = (char)toupper(*p);

  if (!strcmp(tok, "GO")) {
    etapa.iniciar();
    log(etapa.nombreEstado());
  } else if (!strcmp(tok, "STOP")) {
    etapa.pausar();
    log(etapa.nombreEstado());
  } else if (!strcmp(tok, "ESTOP")) {
    etapa.paroEmergencia();
    log(etapa.nombreEstado());
  } else if (!strcmp(tok, "RESET")) {
    etapa.reiniciar();
    log(etapa.nombreEstado());
  } else if (!strcmp(tok, "STATUS")) {
    log(etapa.nombreEstado());
  } else if (!strcmp(tok, "S")) {
    char *t[4];
    for (uint8_t i = 0; i < 4; i++) t[i] = strtok(NULL, " ");
    if (!t[0] || !t[1] || !t[2] || !t[3]) { log(F("S: faltan datos (S n a b c)")); return; }
    uint8_t n = (uint8_t)atoi(t[0]);
    if (!etapa.manual(n, atof(t[1]), atof(t[2]), atof(t[3])))
      log(F("S rechazado (brazo no pertenece a la etapa o sistema en MARCHA/EMERGENCIA)"));
  } else {
    log(F("comando desconocido"));
  }
}

void PuenteSerial::enviarPose(Brazo &b) {
  _s.print(F("P "));
  _s.print(b.numero());
  for (uint8_t j = 0; j < 3; j++) {
    _s.print(' ');
    _s.print(b.angulo(j), 1);
  }
  _s.println();
}

void PuenteSerial::reportar(EtapaSoldadura &etapa) {
  if (etapa.tomarFin()) log(F("etapa 1: ciclo terminado"));

  uint32_t ahora = millis();
  if (ahora - _ultimoRep < 50) return;
  _ultimoRep = ahora;

  bool latido = (ahora - _ultimoLatido) >= 1000;
  if (latido) _ultimoLatido = ahora;

  for (uint8_t i = 0; i < EtapaSoldadura::NUM_BRAZOS; i++) {
    Brazo *b = etapa.brazo(i);
    uint8_t idx = b->numero() - 1;
    if (idx >= 6) continue;
    bool cambio = false;
    for (uint8_t j = 0; j < 3; j++)
      if (fabsf(b->angulo(j) - _enviado[idx][j]) > 0.05f) cambio = true;
    if (cambio || latido) {
      enviarPose(*b);
      for (uint8_t j = 0; j < 3; j++) _enviado[idx][j] = b->angulo(j);
    }
  }
}
