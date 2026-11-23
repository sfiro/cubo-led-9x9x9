# Diagnostico del cubo 10x10x10, de lo mas simple a lo mas complejo.
# Correr desde Thonny con "Run" y responder s/n en la consola.
#
#   Etapa 1: senales lentas "a mano" (sin PIO ni DMA). Si aqui no enciende
#            nada, el problema es de cableado o alimentacion.
#   Etapa 2: el PIO enviando los datos (sin DMA).
#   Etapa 3: el driver completo (PIO + DMA), el mismo que usa main.py:
#            pisos, filas, columnas, un LED recorriendo un piso y 16 niveles.
#
# Necesita en la Pico: cubo.py (de pico_w/), este archivo y, si lo cambiaste,
# config.py.

import sys
import time

from machine import Pin

try:
    import config
except ImportError:
    import config_ejemplo as config

import cubo

TAM = 10
TABLA = cubo.tabla_de(TAM)              # [tam, palabras, anodos..., catodos...]
BITS = TABLA[1] * 32                    # 128 bits por piso (los primeros 16 sobran)


def pregunta(texto):
    return input(texto + " (s/n): ").strip().lower().startswith("s")


def bits_piso(n, anodos=None):
    # Los 128 bits para encender el piso n. anodos: lista de LEDs del piso
    # (fila*10 + columna); None = todos.
    bits = [0] * BITS
    for p in (range(TAM * TAM) if anodos is None else anodos):
        bits[TABLA[2 + p]] = 1
    bits[TABLA[2 + TAM * TAM + n]] = 1
    return bits


# ---- etapa 1: bit-bang lento ------------------------------------------------

def enviar_lento(datos, reloj, latch, bits):
    for b in bits:
        datos(b)
        time.sleep_us(5)
        reloj(1)                 # el 74HC595 lee DATA en este flanco de subida
        time.sleep_us(5)
        reloj(0)
    latch(1)                     # pasa los bits a las salidas
    time.sleep_us(5)
    latch(0)


def etapa1():
    print("\n== ETAPA 1: senales lentas, sin PIO ni DMA ==")
    datos = Pin(config.PIN_DATOS, Pin.OUT, value=0)
    reloj = Pin(config.PIN_RELOJ, Pin.OUT, value=0)
    latch = Pin(config.PIN_LATCH, Pin.OUT, value=0)

    print("1a) Todos los bits en 1: deberia encenderse TODO el cubo durante 5 s.")
    enviar_lento(datos, reloj, latch, [1] * BITS)
    time.sleep(5)
    ok_todo = pregunta("    Se encendio el cubo completo (o al menos una parte)?")

    print("1b) Pisos uno por uno, del 0 al 9 (1 s cada uno).")
    for n in range(TAM):
        print("    piso", n)
        enviar_lento(datos, reloj, latch, bits_piso(n))
        time.sleep(1)
    ok_pisos = pregunta("    Se encendio cada piso por separado, en orden?")

    print("1c) En el piso 0: la primera fila, luego la primera columna (2 s cada una).")
    enviar_lento(datos, reloj, latch, bits_piso(0, range(TAM)))
    time.sleep(2)
    enviar_lento(datos, reloj, latch, bits_piso(0, range(0, TAM * TAM, TAM)))
    time.sleep(2)
    ok_filas = pregunta("    Se vio una fila de 10 LEDs y luego una columna de 10?")

    enviar_lento(datos, reloj, latch, [0] * BITS)
    return ok_todo, ok_pisos, ok_filas


# ---- etapa 2: PIO sin DMA ----------------------------------------------------

def etapa2(freq):
    print("\n== ETAPA 2: PIO sin DMA, a %d Hz ==" % freq)
    import array
    import rp2
    paso = TABLA[1] + 1
    vox = bytearray(b"\x01" * (TAM ** 3))
    cuadro = array.array("I", [0] * (TAM * paso))
    for n in range(TAM):
        cuadro[n * paso + paso - 1] = 1000
    cubo._empaquetar_n(vox, cuadro, TABLA, 1)
    sm = rp2.StateMachine(0, cubo._PROGRAMAS[TABLA[1]], freq=freq, out_base=Pin(config.PIN_DATOS),
                          sideset_base=Pin(config.PIN_RELOJ), set_base=Pin(config.PIN_LATCH))
    sm.active(1)
    print("    Todo el cubo encendido (multiplexado) durante 4 s.")
    fin = time.ticks_add(time.ticks_ms(), 4000)
    while time.ticks_diff(fin, time.ticks_ms()) > 0:
        for w in cuadro:
            sm.put(w)
    for _ in range(paso):        # apagar
        sm.put(0)
    time.sleep_ms(5)
    sm.active(0)
    return pregunta("    Se encendio el cubo completo, parejo y sin LEDs locos?")


# ---- etapa 3: driver completo ------------------------------------------------

def etapa3(freq):
    print("\n== ETAPA 3: driver completo (PIO + DMA) a %d Hz ==" % freq)
    c = cubo.Cubo(config.PIN_DATOS, config.PIN_RELOJ, config.PIN_LATCH, freq, tam=TAM)
    try:
        c.encender()
        c.mostrar()
        print("    Todo encendido 3 s...")
        time.sleep(3)
        for eje, nombre in ((0, "pisos (de abajo hacia arriba)"), (1, "filas (de atras hacia adelante)"),
                            (2, "columnas (de izquierda a derecha)")):
            print("    Planos:", nombre)
            for k in range(TAM):
                c.apagar()
                c.plano(eje, k, 1)
                c.mostrar()
                time.sleep_ms(300)
        print("    Un LED recorriendo el piso 0, fila por fila...")
        for y in range(TAM):
            for x in range(TAM):
                c.apagar()
                c.led(0, y, x, 1)
                c.mostrar()
                time.sleep_ms(60)
    finally:
        c.detener()
    return pregunta("    Se vieron los pisos, las filas, las columnas y el LED recorriendo todo el piso, sin saltos?")


def etapa_brillo(freq):
    print("\n== BRILLO: 16 niveles (BAM) ==")
    c = cubo.Cubo(config.PIN_DATOS, config.PIN_RELOJ, config.PIN_LATCH, freq, bits=4, tam=TAM)
    try:
        print("    Cada piso con un brillo distinto: del piso 0 (apagado) al 9 (maximo), 5 s.")
        for z in range(TAM):
            nivel = round(15 * z / (TAM - 1))
            for y in range(TAM):
                for x in range(TAM):
                    c.brillo(z, y, x, nivel)
        c.mostrar()
        time.sleep(5)
    finally:
        c.detener()
    return pregunta("    Se vio un degradado parejo, sin parpadeo?")


# ---- programa ----------------------------------------------------------------

def main():
    print("MicroPython:", sys.version, "|", sys.implementation)
    print("Cubo 10x10x10: DATA GP%d, CLOCK GP%d, LATCH GP%d"
          % (config.PIN_DATOS, config.PIN_RELOJ, config.PIN_LATCH))

    ok_todo, ok_pisos, ok_filas = etapa1()
    if not ok_todo:
        print("\n>> PROBLEMA DE CABLEADO O ALIMENTACION (no es el software).")
        print("   Revisa: fuente de 5 V del cubo encendida, GND comun con la Pico, OE (pin 13)")
        print("   a GND, MR (pin 10) a 5 V, y que DATA vaya al pin 14 del PRIMER registro (U1).")
        return
    if not (ok_pisos and ok_filas):
        print("\n>> Llegan datos, pero el orden de los pisos o de los LEDs no coincide con el")
        print("   sketch cuboLED10por10. Anota que se encendio en cada paso y cuentamelo.")

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
        ok3 = etapa3(freq) and etapa_brillo(freq)
    except Exception as e:
        sys.print_exception(e)
        ok3 = False
    if ok3:
        print("\n>> TODO FUNCIONA. Copia main.py y reinicia la Pico: el cubo arranca solo.")
    else:
        print("\n>> Copia todo lo que salio en la consola y enviamelo para corregirlo.")


main()
