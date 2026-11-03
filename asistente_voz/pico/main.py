# Cubo LED como pantalla de la Raspberry Pi 5 (proyecto asistente_voz).
#
# La Pico solo refresca el cubo (PIO + DMA, con el mismo cubo.py de pico_w/).
# La Pi le manda los cuadros por el cable USB, una linea de texto por cuadro:
#   F + 184 caracteres hexadecimales + "\n"  -> 92 bytes = 729 LEDs, 1 bit cada uno
#   A\n                                      -> apagar todo (y no mostrar la espera)
#   ?\n                                      -> responde "CUBO9 <version>"
# Se usa texto y no bytes crudos para que nunca viaje un 0x03 (Ctrl-C): asi
# Thonny puede seguir deteniendo el programa.
# Si la Pi no manda nada por 3 s, el cubo muestra una esfera que "respira".
#
# Copiar a la Pico: este main.py y pico_w/cubo.py.

import math
import select
import sys
import time
from binascii import unhexlify

import micropython
from micropython import const

try:
    import config               # opcional: el mismo config.py de pico_w/ sirve
except ImportError:
    config = None
from cubo import TOTAL, Cubo

VERSION = "1"
_BYTES_CUADRO = const(92)
_ESPERA_REPOSO_MS = const(3000)


def _ajuste(nombre, defecto):
    return getattr(config, nombre, defecto)


@micropython.viper
def _desempaquetar(datos: ptr8, vox: ptr8):
    # Bit b del byte k = LED 8k+b, con LED = z*81 + y*9 + x
    for i in range(729):
        vox[i] = (datos[i >> 3] >> (i & 7)) & 1


# Distancia de cada LED al centro del cubo, en decimas, para la esfera de espera
_DIST = bytearray(TOTAL)
for _i in range(TOTAL):
    _z, _r = divmod(_i, 81)
    _y, _x = divmod(_r, 9)
    _DIST[_i] = int(10 * ((_z - 4) ** 2 + (_y - 4) ** 2 + (_x - 4) ** 2) ** 0.5 + 0.5)


@micropython.viper
def _cascara(vox: ptr8, dist: ptr8, r: int):
    # Enciende los LEDs que estan a r +- 0.6 del centro (r en decimas)
    for i in range(729):
        d = int(dist[i]) - r
        if d >= -6 and d <= 6:
            vox[i] = 1
        else:
            vox[i] = 0


def principal():
    c = Cubo(_ajuste("PIN_DATOS", 2), _ajuste("PIN_RELOJ", 3), _ajuste("PIN_LATCH", 4),
             _ajuste("FREQ_PIO", 5_000_000))
    entrada = select.poll()
    entrada.register(sys.stdin, select.POLLIN)
    ultimo = time.ticks_add(time.ticks_ms(), -_ESPERA_REPOSO_MS)   # arranca en espera
    apagado = False
    print("CUBO9", VERSION, "listo: esperando cuadros por USB")
    try:
        while True:
            if entrada.poll(20):
                linea = sys.stdin.readline().strip()
                if linea[:1] == "F" and len(linea) == 1 + 2 * _BYTES_CUADRO:
                    try:
                        datos = unhexlify(linea[1:])
                    except ValueError:
                        continue                # linea danada: se descarta
                    _desempaquetar(datos, c.vox)
                    c.mostrar()
                    ultimo = time.ticks_ms()
                    apagado = False
                elif linea == "A":
                    c.apagar()
                    c.mostrar()
                    apagado = True
                elif linea == "?":
                    print("CUBO9", VERSION)
            elif not apagado and time.ticks_diff(time.ticks_ms(), ultimo) > _ESPERA_REPOSO_MS:
                # La Pi no esta mandando nada: esfera que crece y se encoge cada ~4 s
                r = 25 + int(10 * math.sin(time.ticks_ms() / 600))
                _cascara(c.vox, _DIST, r)
                c.mostrar()
    finally:
        c.detener()             # Stop en Thonny: deja el cubo apagado


principal()
