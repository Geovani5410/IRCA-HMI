// IRCA - firmware de los brazos (ESP32 DevKit v1 y Arduino Uno)
// Etapa 1: SOLDADURA DE CHASIS (brazos N1 y N2).
//
// Conexion I2C al PCA9685:  ESP32: SDA=21 SCL=22   |   Uno: SDA=A4 SCL=A5
// Canales del PCA9685 (0x40):  N1 = 0,1,2 (base, hombro, codo)   N2 = 3,4,5
#include <Wire.h>
#include <IRCA_Brazos.h>

PCA9685Driver  pca1(0x40);
Brazo          brazo1(1);                 // N1
Brazo          brazo2(2);                 // N2
EtapaSoldadura etapa1(brazo1, brazo2);
PuenteSerial   puente(Serial);

void setup() {
  puente.begin(115200);
  Wire.begin();
  Wire.setClock(400000);

  if (!pca1.begin(50)) puente.log(F("ERROR: PCA9685 0x40 no responde"));
  brazo1.begin(pca1, 0);                  // N1_1, N1_2, N1_3 -> canales 0,1,2
  brazo2.begin(pca1, 3);                  // N2_1, N2_2, N2_3 -> canales 3,4,5

  puente.log(F("IRCA listo - etapa 1 (envia GO para iniciar)"));
}

void loop() {
  puente.atender(etapa1);    // ordenes desde la Raspberry: GO / STOP / ESTOP / RESET / S
  etapa1.Soldadura();        // mueve los servos de N1 y N2
  puente.reportar(etapa1);   // manda los angulos a la simulacion
}
