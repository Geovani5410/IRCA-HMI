#pragma once
#include <Arduino.h>
#include "PCA9685Driver.h"

// Calibracion de un servo. Los angulos "logicos" son los de la simulacion:
// 90 = reposo. offset/invertido ajustan el montaje fisico.
struct ConfigServo {
  uint8_t  canal;
  uint16_t minUs;        // pulso a 0 grados
  uint16_t maxUs;        // pulso a 180 grados
  int8_t   offsetGrados; // correccion de montaje
  bool     invertido;
  uint8_t  limMin;       // limite logico minimo (proteccion mecanica)
  uint8_t  limMax;       // limite logico maximo
};

// Valores iniciales por modelo. Ajusta limites/pulsos al calibrar.
inline ConfigServo ConfigMG996R(uint8_t canal) { return {canal, 500, 2500, 0, false, 20, 160}; }
inline ConfigServo ConfigMG90S(uint8_t canal)  { return {canal, 500, 2400, 0, false, 20, 160}; }

class ServoPCA {
public:
  ConfigServo cfg;            // se puede modificar despues de begin()

  void begin(PCA9685Driver &pca, const ConfigServo &c);
  float limitar(float grados) const;
  void escribir(float grados);   // aplica limites, offset, inversion y manda el pulso
  float angulo() const { return _grados; }
  void liberar();                // quita el PWM (servo flojo)

private:
  PCA9685Driver *_pca = nullptr;
  float _grados = 90.0f;
};
