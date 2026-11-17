#!/usr/bin/env python3
"""
IRCA - servidor de la Raspberry Pi 5.

  * Sirve el frontend (../frontend) por HTTP.
  * Puente Serial -> WebSocket: lee "P n a b c" de la ESP32/Arduino y se lo manda
    al HMI con el mismo formato rosbridge que ya entiende index.html
    (/brazo_N/joint_states, posiciones en radianes alrededor de 90 grados).
  * Opcional: el HMI puede mandar {"op":"cmd","data":"GO"} (GO, STOP, ESTOP, RESET).

Uso:
    python3 server.py                      # puerto serial automatico, HTTP en :8080
    python3 server.py --serial /dev/ttyUSB0 --baud 115200 --port 8080
"""
import argparse
import asyncio
import glob
import json
import logging
import math
import threading
import time
from pathlib import Path

import serial
from aiohttp import WSMsgType, web

log = logging.getLogger("irca")
FRONTEND = Path(__file__).resolve().parent.parent / "frontend"
COMANDOS_PERMITIDOS = {"GO", "STOP", "ESTOP", "RESET", "STATUS"}


class PuenteSerial:
    """Hilo que mantiene abierto el puerto serial y reconecta solo."""

    def __init__(self, puerto, baud, loop, cola):
        self.puerto, self.baud = puerto, baud
        self.loop, self.cola = loop, cola
        self.ser = None
        self.conectado = False
        self._parar = threading.Event()
        self._lock = threading.Lock()
        self.hilo = threading.Thread(target=self._run, daemon=True)

    def iniciar(self):
        self.hilo.start()

    def parar(self):
        self._parar.set()

    def _emitir(self, evento):
        self.loop.call_soon_threadsafe(self.cola.put_nowait, evento)

    def _buscar_puerto(self):
        if self.puerto != "auto":
            return self.puerto
        for patron in ("/dev/ttyUSB*", "/dev/ttyACM*"):
            candidatos = sorted(glob.glob(patron))
            if candidatos:
                return candidatos[0]
        return None

    def _abrir(self, puerto):
        s = serial.Serial()
        s.port, s.baudrate, s.timeout = puerto, self.baud, 0.2
        s.dtr = False   # evita (en lo posible) resetear la placa al abrir el puerto
        s.rts = False
        s.open()
        return s

    def _run(self):
        while not self._parar.is_set():
            puerto = self._buscar_puerto()
            if not puerto:
                time.sleep(1)
                continue
            try:
                self.ser = self._abrir(puerto)
                self.conectado = True
                log.info("serial abierto: %s @ %d", puerto, self.baud)
                self._emitir(("estado", True))
                while not self._parar.is_set():
                    linea = self.ser.readline()
                    if linea:
                        self._procesar(linea.decode("utf-8", errors="replace").strip())
            except (serial.SerialException, OSError) as e:
                log.warning("serial perdido: %s", e)
            finally:
                if self.conectado:
                    self.conectado = False
                    self._emitir(("estado", False))
                if self.ser:
                    try:
                        self.ser.close()
                    except Exception:
                        pass
                    self.ser = None
            time.sleep(1)

    def _procesar(self, linea):
        if not linea:
            return
        if linea.startswith("#"):
            log.info("placa: %s", linea[1:].strip())
            self._emitir(("log", linea[1:].strip()))
            return
        partes = linea.split()
        if partes[0] == "P" and len(partes) == 5:
            try:
                n = int(partes[1])
                grados = [float(x) for x in partes[2:5]]
            except ValueError:
                return
            if 1 <= n <= 6:
                self._emitir(("pose", n, grados))
        else:
            log.debug("linea ignorada: %r", linea)

    def enviar(self, texto):
        with self._lock:
            if self.ser and self.conectado:
                self.ser.write((texto + "\n").encode())
                return True
        return False


def msg_pose(n, grados):
    # El HMI hace: grados = rad * 180/pi + 90
    rad = [math.radians(g - 90.0) for g in grados]
    return json.dumps({"op": "publish", "topic": f"/brazo_{n}/joint_states",
                       "msg": {"position": rad}})


def msg_estado(ok):
    return json.dumps({"op": "publish", "topic": "/estado", "msg": {"serial": bool(ok)}})


async def ws_handler(request):
    app = request.app
    ws = web.WebSocketResponse(heartbeat=20)
    await ws.prepare(request)
    app["clientes"].add(ws)
    log.info("HMI conectado (%d)", len(app["clientes"]))

    # estado inicial: serial + ultima pose conocida de cada brazo
    await ws.send_str(msg_estado(app["puente"].conectado))
    for n, g in app["ultima"].items():
        await ws.send_str(msg_pose(n, g))

    try:
        async for m in ws:
            if m.type != WSMsgType.TEXT:
                continue
            try:
                d = json.loads(m.data)
            except json.JSONDecodeError:
                continue
            if d.get("op") == "cmd":
                cmd = str(d.get("data", "")).strip().upper()
                if cmd in COMANDOS_PERMITIDOS:
                    ok = app["puente"].enviar(cmd)
                    log.info("cmd %s -> %s", cmd, "enviado" if ok else "sin serial")
    finally:
        app["clientes"].discard(ws)
        log.info("HMI desconectado (%d)", len(app["clientes"]))
    return ws


async def difusor(app):
    """Toma eventos del hilo serial y los reparte a todos los HMI."""
    cola = app["cola"]
    while True:
        ev = await cola.get()
        if ev[0] == "pose":
            _, n, g = ev
            app["ultima"][n] = g
            texto = msg_pose(n, g)
        elif ev[0] == "estado":
            texto = msg_estado(ev[1])
        else:
            continue
        for ws in list(app["clientes"]):
            try:
                await ws.send_str(texto)
            except ConnectionResetError:
                app["clientes"].discard(ws)


async def index(request):
    return web.FileResponse(FRONTEND / "index.html")


async def on_startup(app):
    app["cola"] = asyncio.Queue()
    app["clientes"] = set()
    app["ultima"] = {}
    app["puente"] = PuenteSerial(app["args"].serial, app["args"].baud,
                                 asyncio.get_running_loop(), app["cola"])
    app["puente"].iniciar()
    app["tarea"] = asyncio.create_task(difusor(app))


async def on_cleanup(app):
    app["puente"].parar()
    app["tarea"].cancel()


def main():
    ap = argparse.ArgumentParser(description="Servidor IRCA (HTTP + WebSocket + Serial)")
    ap.add_argument("--serial", default="auto", help="puerto serial o 'auto' (default)")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--host", default="0.0.0.0")
    ap.add_argument("--port", type=int, default=8080)
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()
    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO,
                        format="%(asctime)s %(levelname)s %(message)s")

    app = web.Application()
    app["args"] = args
    app.router.add_get("/ws", ws_handler)
    app.router.add_get("/", index)
    app.router.add_static("/", FRONTEND, show_index=False)
    app.on_startup.append(on_startup)
    app.on_cleanup.append(on_cleanup)
    log.info("frontend: %s  ->  http://%s:%d", FRONTEND, args.host, args.port)
    web.run_app(app, host=args.host, port=args.port, print=None)


if __name__ == "__main__":
    main()
