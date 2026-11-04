# Cubo LED como pantalla de la Raspberry Pi 5 (proyecto asistente_voz).
#
# La Pico solo refresca el cubo (PIO + DMA, con el mismo cubo.py de pico_w/).
# La Pi le manda los cuadros por el cable USB, una linea de texto por cuadro:
#   F + 184 caracteres hexadecimales + "\n"  -> 92 bytes = 729 LEDs, 1 bit cada uno
#   G + 730 caracteres hexadecimales + "\n"  -> 365 bytes = 729 LEDs, 4 bits cada uno
#                                               (brillo 0..15, desde la v3)
#   A\n                                      -> apagar todo (y no mostrar la espera)
#   ?\n                                      -> responde "CUBO9 <version> <bits>"
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

VERSION = "2"
_BYTES_1BIT = const(92)
_BYTES_4BITS = const(365)
_ESPERA_REPOSO_MS = const(3000)


def _ajuste(nombre, defecto):
    return getattr(config, nombre, defecto)


@micropython.viper
def _desempaquetar(datos: ptr8, vox: ptr8, alto: int):
    # Cuadro de 1 bit: bit b del byte k = LED 8k+b, con LED = z*81 + y*9 + x.
    # Un LED encendido queda con el brillo maximo (alto).
    for i in range(729):
        vox[i] = ((datos[i >> 3] >> (i & 7)) & 1) * alto


@micropython.viper
def _desempaquetar4(datos: ptr8, vox: ptr8, corrimiento: int):
    # Cuadro de 4 bits: el LED 2k esta en los 4 bits bajos del byte k, el 2k+1
    # en los altos. Si el driver tiene menos de 4 bits, se descartan los bajos.
    for i in range(729):
        vox[i] = ((datos[i >> 1] >> ((i & 1) * 4)) & 15) >> corrimiento


# Distancia de cada LED al centro del cubo, en decimas, para la esfera de espera
_DIST = bytearray(TOTAL)
for _i in range(TOTAL):
    _z, _r = divmod(_i, 81)
    _y, _x = divmod(_r, 9)
    _DIST[_i] = int(10 * ((_z - 4) ** 2 + (_y - 4) ** 2 + (_x - 4) ** 2) ** 0.5 + 0.5)


def _degradado(alto):
    # Brillo segun la distancia a la cascara (en decimas, 0..11): se apaga a
    # 1.2 LEDs de ella. Con 1 bit queda una cascara de +- 0.6, como en la v1.
    return bytes((alto * (12 - d) + 6) // 12 for d in range(12))


@micropython.viper
def _cascara(vox: ptr8, dist: ptr8, r: int, degradado: ptr8):
    # Esfera hueca de radio r (en decimas)
    for i in range(729):
        d = int(dist[i]) - r
        if d < 0:
            d = 0 - d
        if d < 12:
            vox[i] = degradado[d]
        else:
            vox[i] = 0


def principal():
    pines = (_ajuste("PIN_DATOS", 2), _ajuste("PIN_RELOJ", 3), _ajuste("PIN_LATCH", 4),
             _ajuste("FREQ_PIO", 5_000_000))
    # 16 niveles necesitan el PIO a 5 MHz: a 1 MHz cargar los 96 bits tarda mas
    # y el cubo parpadearia (~33 Hz). Con el reloj lento se usan 4 niveles.
    bits = _ajuste("BITS_BRILLO", 4 if pines[3] >= 4_000_000 else 2)
    try:
        c = Cubo(*pines, bits=bits)
    except TypeError:                   # cubo.py viejo, sin niveles de brillo
        print("AVISO: cubo.py no tiene niveles de brillo; copia el de pico_w/ actualizado")
        c = Cubo(*pines)
        c.bits, c.maximo = 1, 1
    degradado = _degradado(c.maximo)
    entrada = select.poll()
    entrada.register(sys.stdin, select.POLLIN)
    ultimo = time.ticks_add(time.ticks_ms(), -_ESPERA_REPOSO_MS)   # arranca en espera
    apagado = False
    print("CUBO9", VERSION, c.bits, "listo: esperando cuadros por USB")
    try:
        while True:
            if entrada.poll(20):
                linea = sys.stdin.readline().strip()
                tipo = linea[:1]
                if (tipo == "G" and len(linea) == 1 + 2 * _BYTES_4BITS) or \
                        (tipo == "F" and len(linea) == 1 + 2 * _BYTES_1BIT):
                    try:
                        datos = unhexlify(linea[1:])
                    except ValueError:
                        continue                # linea danada: se descarta
                    if tipo == "G":
                        _desempaquetar4(datos, c.vox, 4 - c.bits)
                    else:
                        _desempaquetar(datos, c.vox, c.maximo)
                    c.mostrar()
                    ultimo = time.ticks_ms()
                    apagado = False
                elif linea == "A":
                    c.apagar()
                    c.mostrar()
                    apagado = True
                elif linea == "?":
                    print("CUBO9", VERSION, c.bits)
            elif not apagado and time.ticks_diff(time.ticks_ms(), ultimo) > _ESPERA_REPOSO_MS:
                # La Pi no esta mandando nada: esfera que crece y se encoge cada ~4 s
                r = 25 + int(10 * math.sin(time.ticks_ms() / 600))
                _cascara(c.vox, _DIST, r, degradado)
                c.mostrar()
    finally:
        c.detener()             # Stop en Thonny: deja el cubo apagado


principal()
