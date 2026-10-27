# Pruebas del cableado (equivalente a PruebaDeRegistros.ino).
# Correr desde Thonny con el boton "Run" (no hace falta borrar main.py:
# detener primero el programa con Ctrl-C).
#
#   pisos()      -> enciende cada piso completo, de abajo hacia arriba
#   recorrer()   -> un solo LED recorre las 729 posiciones en orden (z, y, x)
#   todo()       -> todo encendido 5 segundos
#
# Si en recorrer() un LED se salta, se repite o se enciende otro, anotar
# la posicion que imprime: indica el registro o la soldadura con problema.

import time

try:
    import config
except ImportError:          # sin config.py propio: usar la plantilla
    import config_ejemplo as config
from cubo import Cubo

c = Cubo(config.PIN_DATOS, config.PIN_RELOJ, config.PIN_LATCH, config.FREQ_PIO)


def pisos(ms=700):
    for z in range(9):
        c.apagar()
        c.plano(0, z, 1)
        c.mostrar()
        print("piso", z)
        time.sleep_ms(ms)
    c.apagar()
    c.mostrar()


def recorrer(ms=60):
    for z in range(9):
        for y in range(9):
            for x in range(9):
                c.apagar()
                c.led(z, y, x, 1)
                c.mostrar()
                p = y * 9 + x
                print("z=%d y=%d x=%d -> registro U%d bit %d" % (z, y, x, p // 8 + 1, p % 8))
                time.sleep_ms(ms)
    c.apagar()
    c.mostrar()


def todo(ms=5000):
    c.encender()
    c.mostrar()
    time.sleep_ms(ms)
    c.apagar()
    c.mostrar()


try:
    pisos()
    todo()
    recorrer()
finally:
    c.detener()
