#pragma once
#include <Arduino.h>
#include "ServoPCA.h"

// Brazo de 3 servos: j0 = base, j1 = hombro, j2 = codo.
// Canales consecutivos en el PCA9685: canalBase, +1, +2.
// Movimiento suave y NO bloqueante: llama actualizar() en cada loop().
class Brazo {
public:
  explicit Brazo(uint8_t numero) : _num(numero) {}

  // base y hombro = MG996R, codo = MG90S
  void begin(PCA9685Driver &pca, uint8_t canalBase);

  void irA(float base, float hombro, float codo);                      // salto inmediato
  void moverEn(float base, float hombro, float codo, uint32_t ms);     // interpolado, llegan juntos
  void reposo(uint32_t ms = 0) { moverEn(90, 90, 90, ms); }
  void congelar();                                                     // se queda donde esta (con torque)
  void liberar();                                                      // quita el PWM de los 3 servos

  bool actualizar();             // avanza el movimiento; true cuando llego al destino
  bool llego() const { return _llego; }

  float angulo(uint8_t j) const { return _actual[j]; }
  uint8_t numero() const { return _num; }
  ServoPCA &servo(uint8_t j) { return _servo[j]; }   // para calibrar

private:
  static const uint16_t PERIODO_MS = 20;   // 50 Hz de actualizacion
  uint8_t _num;
  ServoPCA _servo[3];
  float _ini[3] = {90, 90, 90};
  float _fin[3] = {90, 90, 90};
  float _actual[3] = {90, 90, 90};
  uint32_t _t0 = 0, _dur = 0, _ultimo = 0;
  bool _llego = true;
};
