# Diagnostico del cubo, de lo mas simple a lo mas complejo.
# Correr desde Thonny con "Run" y responder s/n en la consola.
#
#   Etapa 1: senales lentas "a mano" (sin PIO ni DMA). Si aqui no enciende
#            nada, el problema es de cableado o alimentacion.
#   Etapa 2: el PIO enviando los datos (sin DMA).
#   Etapa 3: el driver completo (PIO + DMA), el mismo que usa main.py.
#   Prueba de pines: cada pin parpadea 1 vez por segundo para medirlo con
#            un multimetro o un LED con resistencia.

import sys
import time

from machine import Pin

try:
    import config
except ImportError:
    import config_ejemplo as config

import cubo

PINES = (("DATA", config.PIN_DATOS), ("CLOCK", config.PIN_RELOJ), ("LATCH", config.PIN_LATCH))
PIN_FISICO = {0: 1, 1: 2, 2: 4, 3: 5, 4: 6, 5: 7, 6: 9, 7: 10, 8: 11, 9: 12, 10: 14,
              11: 15, 12: 16, 13: 17, 14: 19, 15: 20, 16: 21, 17: 22, 18: 24, 19: 25,
              20: 26, 21: 27, 22: 29, 26: 31, 27: 32, 28: 34}


def pregunta(texto):
    r = input(texto + " (s/n): ").strip().lower()
    return r.startswith("s")


def bits_piso(n, anodos=True):
    # 96 bits para encender el piso n (todos sus LEDs), en el orden de envio
    bits = [0] * 96
    if anodos:
        for p in range(81):
            bits[cubo._TABLA[p]] = 1
    bits[cubo._TABLA[81 + n]] = 1
    return bits


# ---- etapa 1: bit-bang lento ------------------------------------------------

def enviar_lento(datos, reloj, latch, bits):
    for b in bits:
        datos(b)
        time.sleep_us(5)
        reloj(1)                 # el 74HC595 lee DATA en este flanco de subida
        time.sleep_us(5)
        reloj(0)
    latch(1)                     # pasa los 96 bits a las salidas
    time.sleep_us(5)
    latch(0)


def etapa1():
    print("\n== ETAPA 1: senales lentas, sin PIO ni DMA ==")
    datos = Pin(config.PIN_DATOS, Pin.OUT, value=0)
    reloj = Pin(config.PIN_RELOJ, Pin.OUT, value=0)
    latch = Pin(config.PIN_LATCH, Pin.OUT, value=0)

    print("1a) Todos los bits en 1: deberia encenderse TODO el cubo durante 5 s.")
    enviar_lento(datos, reloj, latch, [1] * 96)
    time.sleep(5)
    ok_todo = pregunta("    Se encendio el cubo completo (o al menos una parte)?")

    print("1b) Pisos uno por uno, de abajo hacia arriba (1 s cada uno).")
    for n in range(9):
        print("    piso", n)
        enviar_lento(datos, reloj, latch, bits_piso(n))
        time.sleep(1)
    ok_pisos = pregunta("    Se encendio cada piso por separado, en orden?")

    enviar_lento(datos, reloj, latch, [0] * 96)
    return ok_todo, ok_pisos


# ---- etapa 2: PIO sin DMA ----------------------------------------------------

def etapa2(freq):
    print("\n== ETAPA 2: PIO sin DMA, a %d Hz ==" % freq)
    import array
    import rp2
    vox = bytearray(b"\x01" * 729)
    cuadro = array.array("I", [0] * 36)
    for n in range(9):
        cuadro[n * 4 + 3] = 1000
    cubo._empaquetar(vox, cuadro, cubo._TABLA, 1)
    sm = rp2.StateMachine(0, cubo._pio_cubo, freq=freq, out_base=Pin(config.PIN_DATOS),
                          sideset_base=Pin(config.PIN_RELOJ), set_base=Pin(config.PIN_LATCH))
    sm.active(1)
    print("    Todo el cubo encendido (multiplexado) durante 4 s.")
    fin = time.ticks_add(time.ticks_ms(), 4000)
    while time.ticks_diff(fin, time.ticks_ms()) > 0:
        for w in cuadro:
            sm.put(w)
    for _ in range(4):           # apagar
        sm.put(0)
    time.sleep_ms(5)
    sm.active(0)
    return pregunta("    Se encendio el cubo completo, parejo y sin LEDs locos?")


# ---- etapa 3: driver completo ------------------------------------------------

def etapa3(freq):
    print("\n== ETAPA 3: driver completo (PIO + DMA) a %d Hz ==" % freq)
    c = cubo.Cubo(config.PIN_DATOS, config.PIN_RELOJ, config.PIN_LATCH, freq)
    try:
        c.encender()
        c.mostrar()
        print("    Todo encendido 3 s...")
        time.sleep(3)
        for z in range(9):
            c.apagar()
            c.plano(0, z, 1)
            c.mostrar()
            time.sleep_ms(400)
    finally:
        c.detener()
    return pregunta("    Se encendio todo y luego cada piso en orden?")


# ---- prueba de pines ---------------------------------------------------------

def prueba_pines():
    print("\n== PRUEBA DE PINES ==")
    print("Mide con un multimetro entre cada cable y GND, en el extremo del cubo:")
    print("debe alternar entre 0 V y 3.3 V cada medio segundo.")
    for nombre, gp in PINES:
        p = Pin(gp, Pin.OUT)
        print("  %s: GP%d (marcado '%d' en placas clon) parpadeando 10 s..." % (nombre, gp, gp))
        for _ in range(10):
            p(1)
            time.sleep_ms(500)
            p(0)
            time.sleep_ms(500)


# ---- programa ----------------------------------------------------------------

def main():
    print("MicroPython:", sys.version, "|", sys.implementation)
    for nombre, gp in PINES:
        print("  %-5s -> GP%d: pin marcado '%d' en placas clon, pin fisico %s en la Pico oficial"
              % (nombre, gp, gp, PIN_FISICO.get(gp, "?")))

    ok_todo, ok_pisos = etapa1()
    if not ok_todo:
        print("\n>> PROBLEMA DE CABLEADO O ALIMENTACION (no es el software).")
        print("   Revisa: fuente de 5 V del cubo encendida, GND comun, OE (pin 13) a GND,")
        print("   MR (pin 10) a 5 V, y que DATA vaya al pin 14 del PRIMER registro (U1).")
        if pregunta("   Hacer la prueba de pines para medir con multimetro?"):
            prueba_pines()
        return
    if not ok_pisos:
        print("\n>> Llegan datos, pero el orden de los pisos o de los bits no coincide.")
        print("   Anota que piso se encendio en cada paso y cuentamelo.")

    for freq in (config.FREQ_PIO, 1_000_000, 250_000):
        try:
            if etapa2(freq):
                break
        except Exception as e:
            sys.print_exception(e)
        print("   Probando mas lento...")
    else:
        print("\n>> El envio rapido falla: la senal de 3.3 V no alcanza para los 74HC595.")
        print("   Usa cables cortos, GND grueso, o un 74AHCT125 como adaptador de nivel.")
        return
    if freq != config.FREQ_PIO:
        print("\n>> Funciona a %d Hz: pon FREQ_PIO = %d en config.py." % (freq, freq))

    try:
        ok3 = etapa3(freq)
    except Exception as e:
        sys.print_exception(e)
        ok3 = False
    if ok3:
        print("\n>> TODO FUNCIONA. Si main.py no anima el cubo, revisa la consola al reiniciar.")
    else:
        print("\n>> El PIO funciona pero el driver con DMA no: copia todo lo que salio en la")
        print("   consola y enviamelo para corregirlo.")


main()
