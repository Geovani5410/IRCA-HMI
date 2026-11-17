#pragma once
#include <Arduino.h>
#include <Wire.h>

// Driver minimo del PCA9685 (16 canales PWM, I2C). Sin dependencias externas.
class PCA9685Driver {
public:
  explicit PCA9685Driver(uint8_t direccion = 0x40, TwoWire &wire = Wire);

  // Inicializa el chip. Devuelve false si no responde en el bus I2C.
  bool begin(float frecuenciaHz = 50.0f);

  // Pulso en microsegundos (0..4095 ticks). Para servos: 500..2500 us.
  void setPulsoUs(uint8_t canal, uint16_t us);
  void apagarCanal(uint8_t canal);   // sin PWM: el servo queda libre
  void apagarTodos();

private:
  uint8_t _dir;
  TwoWire *_wire;
  float _hz;
  void escribir8(uint8_t reg, uint8_t val);
  uint8_t leer8(uint8_t reg);
};
