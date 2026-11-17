#pragma once
#include <Arduino.h>
#include "EtapaSoldadura.h"

// Protocolo de texto, una orden por linea, 115200 baud:
//
//  RPi -> placa                     placa -> RPi
//  GO      inicia / reanuda         P n a b c   angulos actuales del brazo n (0-180, 90 = reposo)
//  STOP    pausa                    # texto     mensajes de log
//  ESTOP   paro de emergencia
//  RESET   vuelve a reposo
//  S n a b c   mueve el brazo n (solo en PARO)
//  STATUS  imprime el estado
class PuenteSerial {
public:
  explicit PuenteSerial(HardwareSerial &s) : _s(s) {}

  void begin(unsigned long baud);
  void atender(EtapaSoldadura &etapa);     // lee ordenes y las aplica a la etapa
  void reportar(EtapaSoldadura &etapa);    // manda "P n a b c" (~20 Hz si hay cambios, 1 Hz de latido)

  void log(const __FlashStringHelper *msg);
  void log(const char *msg);

private:
  HardwareSerial &_s;
  char _buf[40];
  uint8_t _n = 0;
  bool _descartar = false;
  float _enviado[6][3];
  uint32_t _ultimoRep = 0, _ultimoLatido = 0;

  bool leerLinea();
  void procesar(EtapaSoldadura &etapa);
  void enviarPose(Brazo &b);
};
