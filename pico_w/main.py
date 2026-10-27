# Programa principal del cubo LED 9x9x9 con Raspberry Pi Pico W.
# Arranca en modo "demo" y, en paralelo, se conecta al WiFi y levanta
# la pagina web para elegir la animacion. La IP se imprime por la consola.

import asyncio
import time

import network

import animaciones
try:
    import config
except ImportError:          # sin config.py propio: usar la plantilla
    import config_ejemplo as config
import web
from cubo import Cubo


class Control:
    def __init__(self, cubo):
        self.cubo = cubo
        self.actual = None
        self._tarea = None

    def elegir(self, nombre):
        if nombre not in ("demo", "apagar") and nombre not in animaciones.POR_NOMBRE:
            return False
        if self._tarea:
            self._tarea.cancel()
        self.actual = nombre
        self._tarea = asyncio.create_task(self._correr(nombre))
        return True

    async def _correr(self, nombre):
        c = self.cubo
        if nombre == "apagar":
            c.apagar()
            c.mostrar()
            return
        while True:
            if nombre == "demo":
                for _, animacion in animaciones.LISTA:
                    await animacion(c)
            else:
                await animaciones.POR_NOMBRE[nombre](c)


async def conectar_wifi():
    sta = network.WLAN(network.STA_IF)
    sta.active(True)
    sta.connect(config.WIFI_RED, config.WIFI_CLAVE)
    inicio = time.ticks_ms()
    while not sta.isconnected() and time.ticks_diff(time.ticks_ms(), inicio) < 15000:
        await asyncio.sleep_ms(250)
    if sta.isconnected():
        ip = sta.ifconfig()[0]
        print("Conectado a", config.WIFI_RED, "-> abre http://%s" % ip)
        return ip

    sta.active(False)
    ap = network.WLAN(network.AP_IF)
    ap.config(essid=config.AP_NOMBRE, password=config.AP_CLAVE)
    ap.active(True)
    ip = ap.ifconfig()[0]
    print("No se pudo conectar a", config.WIFI_RED)
    print("Conectate a la red", config.AP_NOMBRE, "y abre http://%s" % ip)
    return ip


async def principal(cubo):
    control = Control(cubo)
    control.elegir("demo")
    await conectar_wifi()
    await web.servir(control)
    while True:
        await asyncio.sleep(3600)


cubo = Cubo(config.PIN_DATOS, config.PIN_RELOJ, config.PIN_LATCH, config.FREQ_PIO)
try:
    asyncio.run(principal(cubo))
finally:
    cubo.detener()          # Ctrl-C en Thonny: deja el cubo apagado
    asyncio.new_event_loop()
