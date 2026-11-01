# Prueba de arranque: hace lo mismo que el comienzo de main.py (driver completo
# sin pasar por las etapas 1 y 2 de diagnostico.py) e imprime el estado del PIO,
# del DMA y de los pines para ver por que no enciende.
# Correr desde Thonny con "Run", justo despues de reiniciar la Pico (Stop).

import sys
import time

from machine import Pin, mem32

try:
    import config
except ImportError:
    import config_ejemplo as config

import cubo

_IO_BANK0 = 0x40028000 if "RP2350" in sys.implementation._machine else 0x40014000
PINES = (("DATA", config.PIN_DATOS), ("CLOCK", config.PIN_RELOJ), ("LATCH", config.PIN_LATCH))


def pregunta(texto):
    return input(texto + " (s/n): ").strip().lower().startswith("s")


def estado_pines():
    for nombre, n in PINES:
        ctrl = mem32[_IO_BANK0 + 8 * n + 4] & 0xFFFFFFFF
        pad = mem32[cubo._PADS_BANK0 + 4 + 4 * n] & 0xFFFFFFFF
        print("  %-5s GP%d: funcion=%d (6 = PIO0)  pad=%03x" % (nombre, n, ctrl & 0x1F, pad))


def estado_dma(c):
    b = c._reg_lectura
    for _ in range(4):
        print("  DMA READ=%08x COUNT=%2d CTRL=%08x | FIFO del PIO=%d"
              % (mem32[b] & 0xFFFFFFFF, mem32[b + 8] & 0xFFFFFFFF,
                 mem32[b + 0xC] & 0xFFFFFFFF, c._sm.tx_fifo()))
        time.sleep_ms(100)
    print("  cuadros en", hex(c._dir[0]), "| buffers", hex(cubo.uctypes.addressof(c._cuadros[0])),
          hex(cubo.uctypes.addressof(c._cuadros[1])))


def probar(titulo):
    print("\n==", titulo, "==")
    c = cubo.Cubo(config.PIN_DATOS, config.PIN_RELOJ, config.PIN_LATCH, config.FREQ_PIO)
    print("  DMA: canal de datos", c._dma_datos.channel, "| canal de control", c._dma_ctrl.channel)
    c.encender()
    c.mostrar()
    estado_pines()
    estado_dma(c)
    ok = pregunta("  Se encendio TODO el cubo?")
    c.detener()
    return ok


print("MicroPython:", sys.version, "|", sys.implementation._machine)
print("Pads en", hex(cubo._PADS_BANK0), "| IO en", hex(_IO_BANK0))

if probar("1) Driver directo, como main.py"):
    print("\n>> Funciona. Corre main.py: deberia andar igual.")
else:
    print("\n2) Ahora como diagnostico.py: primero senales a mano por los 3 pines...")
    datos = Pin(config.PIN_DATOS, Pin.OUT, value=0)
    reloj = Pin(config.PIN_RELOJ, Pin.OUT, value=0)
    latch = Pin(config.PIN_LATCH, Pin.OUT, value=0)
    for _ in range(96):
        datos(0)
        reloj(1)
        reloj(0)
    latch(1)
    latch(0)
    estado_pines()
    if probar("2) Driver despues de usar los pines a mano"):
        print("\n>> Solo funciona despues de usar los pines a mano. Copiame TODO lo que")
        print("   salio en la consola para corregir el driver.")
    else:
        print("\n>> Tampoco. Copiame TODO lo que salio en la consola.")
