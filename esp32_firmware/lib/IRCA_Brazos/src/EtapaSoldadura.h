#pragma once
#include <Arduino.h>
#include "Brazo.h"

enum class EstadoEtapa : uint8_t { PARO, MARCHA, EMERGENCIA };

// Etapa 1 - SOLDADURA DE CHASIS: brazos N1 y N2 trabajan a la vez.
// La secuencia de poses es la misma que usa la simulacion (index.html, ETAPAS[1]).
//
// Uso en el programa principal:
//   etapa1.iniciar();      // o por serial: GO
//   loop() { etapa1.Soldadura(); }   // avanza la secuencia sin bloquear
class EtapaSoldadura {
public:
  static const uint8_t NUM_BRAZOS = 2;

  EtapaSoldadura(Brazo &n1, Brazo &n2);

  void Soldadura();            // llamar en cada loop(): mueve los servos de N1 y N2

  void iniciar();              // arranca o reanuda la secuencia
  void pausar();               // se detiene donde esta (mantiene torque)
  void paroEmergencia();       // congela todo; solo reiniciar() lo libera
  void reiniciar();            // vuelve suave al reposo (90,90,90) y queda en PARO

  bool manual(uint8_t numeroBrazo, float base, float hombro, float codo);  // mover un brazo a mano (solo en PARO)
  bool tomarFin();             // true una vez cuando termina un ciclo completo

  EstadoEtapa estado() const { return _estado; }
  const __FlashStringHelper *nombreEstado() const;
  Brazo *brazo(uint8_t i) { return i < NUM_BRAZOS ? _b[i] : nullptr; }
  void setDuracionPaso(uint32_t ms) { _duracionPasoMs = ms; }

private:
  Brazo *_b[NUM_BRAZOS];
  EstadoEtapa _estado = EstadoEtapa::PARO;
  uint8_t _paso = 0;                 // 0 = en reposo; 1.. = pose destino actual
  uint32_t _duracionPasoMs = 2000;   // igual que la simulacion (10 s / 5 tramos)
  bool _fin = false;
  void enviarPaso();
};
