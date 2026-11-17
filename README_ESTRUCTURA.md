# IRCA - cambios de esta actualizacion

- `esp32_firmware/lib/IRCA_Brazos/`  libreria (ESP32 + Arduino Uno): PCA9685, servos, brazo, etapa Soldadura, puente serial
- `esp32_firmware/control_brazos/control_brazos.ino`  programa principal (etapa 1)
- `rpi_backend/server.py`  HTTP + WebSocket + Serial (pyserial)
- `rpi_backend/mediamtx.yml`  video de /dev/video0 por WebRTC
- `tools/parche_video.py`  adapta frontend/index.html (assets/, video, WebSocket)
