# Driver de los cubos LED 9x9x9 y 10x10x10 para Raspberry Pi Pico (MicroPython).
#
# Mismo hardware que con el Arduino, registros 74HC595 en cadena:
#   9x9x9:    12 registros (U1..U11 para los 81 anodos de un piso, U11 y U12
#             para los 9 catodos). Es el de siempre: Cubo() o Cubo(tam=9).
#   10x10x10: 14 registros (U1..U13 para los 100 anodos, U13 y U14 para los
#             10 catodos), como el sketch cuboLED10por10. Cubo(tam=10).
# El cableado de cada uno esta en MAPAS, mas abajo.
#
# El multiplexado de los pisos lo hacen el PIO y el DMA sin usar el CPU:
#   - el PIO saca los 96 bits de un piso por DATA/CLOCK, da el pulso de LATCH
#     y espera un rato antes de pasar al siguiente piso;
#   - dos canales de DMA le entregan el cuadro al PIO en un ciclo infinito.
#     Al arrancar y al detener se abortan, para que no quede uno andando
#     de una corrida anterior.
# Asi el cubo no parpadea aunque Python este ocupado, recolectando basura
# o atendiendo el WiFi.
#
# Niveles de brillo (opcional, bits=2..4): BAM (Bit Angle Modulation). Cada
# piso se muestra en "bits" pasadas; la pasada b dura 2**b veces la primera y
# enciende los LEDs que tienen el bit b de su nivel en 1. Con bits=4 hay 16
# niveles (0..15). Con bits=1 (lo normal) es encendido/apagado, como siempre.
#
# Uso:
#   c = Cubo()
#   c.led(z, y, x, 1)      # z = piso, y = fila, x = columna (0..8, o 0..9 con tam=10)
#   c.mostrar()            # publica lo dibujado (doble buffer, sin parpadeo)
#   c = Cubo(bits=4)
#   c.brillo(z, y, x, 7)   # nivel 0..15 (c.maximo)

import array
import sys
import time

import micropython
import rp2
import uctypes
from machine import Pin, mem32
from micropython import const

TAM = const(9)
LEDS_PISO = const(81)
TOTAL = const(729)
_PALABRAS_PASADA = const(4)             # 3 palabras de datos + 1 de espera
# Ciclos del PIO para sacar los 96 bits y dar el LATCH (ver _pio_cubo): mientras
# tanto sigue encendida la pasada anterior, asi que se descuentan de su espera.
_CICLOS_CARGA = const(204)

_DMA_BASE = const(0x50000000)
# Algunos registros cambian de lugar en la RP2350 (Pico 2 y clones)
_RP2350 = "RP2350" in sys.implementation._machine
_PADS_BANK0 = 0x40038000 if _RP2350 else 0x4001C000
_DMA_ABORT = _DMA_BASE + (0x464 if _RP2350 else 0x444)
_CHAIN_TO = 13 if _RP2350 else 11        # posicion del campo CHAIN_TO en CTRL


# ---- cableado ----------------------------------------------------------------
# El PIO saca palabras de 32 bits, asi que cada piso son 3 palabras (96 bits, 12
# registros justos) en el 9x9x9 y 4 palabras (128 bits) en el 10x10x10: ahi los
# primeros 16 bits sobran, pasan por toda la cadena y se pierden al final.
#
# Para cada tamano: (registros, como se calcula el catodo de cada piso).
# Anodo p = fila*tam + columna -> registro p//8 + 1, bit p%8 (en los dos cubos).


def _catodo9(n):
    # pisos 0..6 en U11 bits 1..7, pisos 7..8 en U12 bits 0..1 (vector anode[] del sketch)
    c = n + 1
    return (11, c) if c < 8 else (12, c - 8)


def _catodo10(n):
    # pisos 0..3 en U13 bits 4..7, pisos 4..9 en U14 bits 0..5 (vector catodos[] del
    # sketch cuboLED10por10: los 4 bits que le sobran al registro 13 y luego el 14)
    return (13, n + 4) if n < 4 else (14, n - 4)


MAPAS = {9: (12, _catodo9), 10: (14, _catodo10)}


def tabla_de(tam):
    # bytearray: [tam, palabras por pasada, posicion de cada anodo..., de cada catodo...]
    # La posicion es el bit (0..palabras*32-1) del flujo que sale por DATA.
    registros, catodo = MAPAS[tam]
    palabras = (registros * 8 + 31) // 32
    sobra = palabras * 32 - registros * 8

    def pos(registro, bit):
        # El primer bit que sale termina en el ultimo registro (bit 7), igual que
        # con SPI MSB primero en el Arduino; U1 es el que esta conectado a DATA.
        return sobra + (registros - registro) * 8 + (7 - bit)

    t = bytearray(2 + tam * tam + tam)
    t[0], t[1] = tam, palabras
    for p in range(tam * tam):
        t[2 + p] = pos(p // 8 + 1, p % 8)
    for n in range(tam):
        t[2 + tam * tam + n] = pos(*catodo(n))
    return t


def _bit_en_cadena(registro, bit):
    # Posicion (0..95) dentro de los 96 bits que salen por DATA.
    # registro: 1..12 (U1 es el que esta conectado a DATA, U12 el ultimo).
    # El primer bit que sale termina en el ultimo registro (U12, bit 7),
    # igual que con SPI MSB primero en el Arduino.
    return (12 - registro) * 8 + (7 - bit)


# Tabla de posiciones: 81 anodos y luego los catodos de los 9 pisos.
# Anodo p = fila*9 + columna -> registro p//8 + 1, bit p%8.
# Catodo del piso n -> bit n+1 de U11:U12 (pisos 0..6 en U11 bits 1..7,
# pisos 7..8 en U12 bits 0..1), como el vector anode[] del sketch original.
_TABLA = bytearray(LEDS_PISO + TAM)
for _p in range(LEDS_PISO):
    _TABLA[_p] = _bit_en_cadena(_p // 8 + 1, _p % 8)
for _n in range(TAM):
    _c = _n + 1
    _TABLA[LEDS_PISO + _n] = _bit_en_cadena(11, _c) if _c < 8 else _bit_en_cadena(12, _c - 8)


@rp2.asm_pio(out_init=rp2.PIO.OUT_LOW, sideset_init=rp2.PIO.OUT_LOW,
             set_init=rp2.PIO.OUT_LOW, out_shiftdir=rp2.PIO.SHIFT_LEFT,
             autopull=True, pull_thresh=32, fifo_join=rp2.PIO.JOIN_TX)
def _pio_cubo():
    # out = DATA, side-set = CLOCK, set = LATCH
    set(y, 2)               .side(0)    # 3 palabras de 32 bits = 96 bits
    label("palabra")
    set(x, 31)              .side(0)
    label("bit")
    out(pins, 1)            .side(0)    # pone el bit con CLOCK en bajo
    jmp(x_dec, "bit")       .side(1)    # flanco de subida: el 595 lee el bit
    jmp(y_dec, "palabra")   .side(0)
    out(x, 32)              .side(0)    # 4a palabra: tiempo de espera del piso
    set(pins, 1)            .side(0) [1]  # LATCH: muestra el piso nuevo
    set(pins, 0)            .side(0)
    label("espera")
    jmp(x_dec, "espera")    .side(0)


@rp2.asm_pio(out_init=rp2.PIO.OUT_LOW, sideset_init=rp2.PIO.OUT_LOW,
             set_init=rp2.PIO.OUT_LOW, out_shiftdir=rp2.PIO.SHIFT_LEFT,
             autopull=True, pull_thresh=32, fifo_join=rp2.PIO.JOIN_TX)
def _pio_cubo4():
    # Igual que _pio_cubo, con 4 palabras por piso (10x10x10: 128 bits)
    set(y, 3)               .side(0)
    label("palabra")
    set(x, 31)              .side(0)
    label("bit")
    out(pins, 1)            .side(0)
    jmp(x_dec, "bit")       .side(1)
    jmp(y_dec, "palabra")   .side(0)
    out(x, 32)              .side(0)
    set(pins, 1)            .side(0) [1]
    set(pins, 0)            .side(0)
    label("espera")
    jmp(x_dec, "espera")    .side(0)


_PROGRAMAS = {3: _pio_cubo, 4: _pio_cubo4}       # palabras por piso -> programa


@micropython.viper
def _empaquetar_n(vox: ptr8, cuadro: ptr32, tabla: ptr8, bits: int):
    # Como _empaquetar, para cualquier tamano: el tamano y las palabras por piso
    # vienen al principio de la tabla (ver tabla_de). Lo usa el 10x10x10; el
    # 9x9x9 sigue con _empaquetar, sin cambios.
    tam = int(tabla[0])
    palabras = int(tabla[1])
    leds = tam * tam
    paso = palabras + 1
    for n in range(tam):
        base = n * leds
        for k in range(bits):
            b = (n * bits + k) * paso
            for w in range(palabras):
                cuadro[b + w] = 0
            for p in range(leds):
                if (vox[base + p] >> k) & 1:
                    s = tabla[2 + p]
                    i = b + (s >> 5)
                    cuadro[i] = cuadro[i] | (1 << (31 - (s & 31)))
            s = tabla[2 + leds + n]
            i = b + (s >> 5)
            cuadro[i] = cuadro[i] | (1 << (31 - (s & 31)))


@micropython.viper
def _empaquetar(vox: ptr8, cuadro: ptr32, tabla: ptr8, bits: int):
    # Convierte los 729 voxeles (1 byte = nivel de brillo) en las palabras que
    # lee el PIO: por piso, una pasada por cada bit del nivel. El catodo del
    # piso va en todas las pasadas. La 4a palabra (la espera) no se toca.
    for n in range(9):
        base = n * 81
        for k in range(bits):
            b = (n * bits + k) * 4
            cuadro[b] = 0
            cuadro[b + 1] = 0
            cuadro[b + 2] = 0
            for p in range(81):
                if (vox[base + p] >> k) & 1:
                    s = tabla[p]
                    i = b + (s >> 5)
                    cuadro[i] = cuadro[i] | (1 << (31 - (s & 31)))
            s = tabla[81 + n]
            i = b + (s >> 5)
            cuadro[i] = cuadro[i] | (1 << (31 - (s & 31)))


class Cubo:
    def __init__(self, pin_datos=2, pin_reloj=3, pin_latch=4,
                 freq_pio=5_000_000, espera=1000, sm_id=0, bits=1, base_us=60, tam=9):
        # freq_pio: cada bit toma 2 ciclos -> 5 MHz da 2.5 MHz en CLOCK.
        #   Si aparecen LEDs locos (senal de 3.3 V debil), bajarla a 1_000_000.
        # espera: con bits=1, ciclos del PIO que cada piso queda encendido despues
        #   de cargarse (1000 a 5 MHz = 200 us -> ~460 cuadros por segundo).
        # bits: 1 = encendido/apagado; 2, 3 o 4 = 4, 8 o 16 niveles de brillo.
        # base_us: con bits >= 2, lo que dura la pasada mas corta. Con 4 bits y
        #   60 us, cada piso toma 15 x 60 = 900 us -> ~120 cuadros por segundo.
        #   No puede ser menor que la carga de los 96 bits (~40 us a 5 MHz).
        # tam: 9 (el cubo de siempre) o 10 (el 10x10x10). Ver MAPAS.
        if not 1 <= bits <= 4:
            raise ValueError("bits debe ser 1, 2, 3 o 4")
        if tam not in MAPAS:
            raise ValueError("tam debe ser uno de %s" % sorted(MAPAS))
        self.tam = tam
        self.leds_piso = tam * tam
        self.total = tam * tam * tam
        self.bits = bits
        self.maximo = (1 << bits) - 1
        self.vox = bytearray(self.total)
        self._ceros = bytes(self.total)
        self._unos = bytes([self.maximo]) * self.total
        if tam == 9:                    # exactamente lo de antes
            self._tabla, palabras, self._pack = _TABLA, 3, _empaquetar
        else:
            self._tabla = tabla_de(tam)
            palabras, self._pack = self._tabla[1], _empaquetar_n
        self._por_piso = palabras + 1           # palabras de datos + 1 de espera
        carga = 66 * palabras + 6               # ciclos para sacar el piso (204 en el 9x9x9)
        self._args_sm = (sm_id, _PROGRAMAS[palabras], freq_pio, pin_datos, pin_reloj, pin_latch)

        if bits == 1:
            esperas = (espera,)
        else:
            corta = max(base_us * freq_pio // 1_000_000, carga + 20)
            esperas = [(corta << k) - carga for k in range(bits)]
        self._palabras = tam * bits * self._por_piso
        self._bytes = self._palabras * 4
        self._cuadros = (array.array("I", [0] * self._palabras),
                         array.array("I", [0] * self._palabras))
        for cuadro in self._cuadros:
            for n in range(tam):
                for k in range(bits):
                    cuadro[(n * bits + k) * self._por_piso + palabras] = esperas[k]
            self._pack(self.vox, cuadro, self._tabla, bits)
        self._atras = 1
        self._dir = array.array("I", [uctypes.addressof(self._cuadros[0])])

        # Reservar los canales de DMA y frenarlos ANTES de tocar el PIO: si una
        # corrida anterior los dejo andando (Stop en Thonny, soft reboot), siguen
        # escribiendo en el FIFO del PIO y el cubo se queda sin mostrar nada.
        self._dma_datos = rp2.DMA()
        self._dma_ctrl = rp2.DMA()
        self._reg_lectura = _DMA_BASE + self._dma_datos.channel * 0x40
        _abortar_dma(self._dma_ctrl.channel, self._dma_datos.channel)

        # Primero como salidas comunes: deja el pad listo (en la RP2350 lo saca
        # del aislamiento). Sin esto, recien encendida, la Pico podia no sacar
        # nada por los pines; diagnostico.py funcionaba porque su etapa 1 lo hacia.
        for pin in (pin_datos, pin_reloj, pin_latch):
            Pin(pin, Pin.OUT, value=0)
        self._sm = self._iniciar_sm()
        for pin in (pin_datos, pin_reloj, pin_latch):
            _fuerza_pin(pin)

        pio, sm = sm_id // 4, sm_id % 4
        fifo_tx = 0x50200010 + pio * 0x100000 + sm * 4
        dreq = pio * 8 + sm

        # Canal de datos: cuadro -> FIFO del PIO; al terminar dispara al de control.
        self._dma_datos.config(
            read=self._dir[0], write=fifo_tx, count=self._palabras,
            ctrl=self._dma_datos.pack_ctrl(size=2, inc_read=True, inc_write=False,
                                           treq_sel=dreq, chain_to=self._dma_ctrl.channel),
            trigger=False)
        # Canal de control: escribe la direccion del cuadro actual en READ_ADDR_TRIG
        # del canal de datos, que vuelve a arrancar. Asi el ciclo nunca se detiene.
        self._dma_ctrl.config(
            read=uctypes.addressof(self._dir), write=self._reg_lectura + 0x3C, count=1,
            ctrl=self._dma_ctrl.pack_ctrl(size=2, inc_read=False, inc_write=False),
            trigger=True)

    def _iniciar_sm(self):
        sm_id, prog, freq, datos, reloj, latch = self._args_sm
        sm = rp2.StateMachine(sm_id, prog, freq=freq, out_base=Pin(datos),
                              sideset_base=Pin(reloj), set_base=Pin(latch))
        sm.active(1)
        return sm

    # ---- dibujo -------------------------------------------------------------

    def led(self, z, y, x, v=1):
        # Coordenadas fuera del cubo se ignoran (antes se pegaban a los bordes).
        t = self.tam
        if 0 <= z < t and 0 <= y < t and 0 <= x < t:
            self.vox[(z * t + y) * t + x] = self.maximo if v else 0

    def brillo(self, z, y, x, nivel):
        # nivel 0..self.maximo (con bits=1, 0 o 1)
        t = self.tam
        if 0 <= z < t and 0 <= y < t and 0 <= x < t:
            self.vox[(z * t + y) * t + x] = max(0, min(self.maximo, int(nivel)))

    def leer(self, z, y, x):
        t = self.tam
        if 0 <= z < t and 0 <= y < t and 0 <= x < t:
            return self.vox[(z * t + y) * t + x]
        return 0

    def apagar(self):
        self.vox[:] = self._ceros

    def encender(self):
        self.vox[:] = self._unos

    def plano(self, eje, k, v=1):
        # eje 0: piso z = k, eje 1: fila y = k, eje 2: columna x = k
        for a in range(self.tam):
            for b in range(self.tam):
                if eje == 0:
                    self.led(k, a, b, v)
                elif eje == 1:
                    self.led(a, k, b, v)
                else:
                    self.led(a, b, k, v)

    def mostrar(self):
        # Empaqueta lo dibujado en el buffer de atras y lo pone en cola.
        # El DMA lo toma al terminar el cuadro actual (no hay cuadros a medias).
        atras = self._cuadros[self._atras]
        ini = uctypes.addressof(atras)
        t = time.ticks_ms()
        # Si el DMA todavia esta leyendo este buffer, esperar (maximo un cuadro).
        while (ini <= mem32[self._reg_lectura] <= ini + self._bytes
               and time.ticks_diff(time.ticks_ms(), t) < 20):
            pass
        self._pack(self.vox, atras, self._tabla, self.bits)
        self._dir[0] = ini
        self._atras ^= 1

    # ---- apagado ------------------------------------------------------------

    def detener(self):
        # Para el refresco y deja todos los LEDs apagados.
        _abortar_dma(self._dma_ctrl.channel, self._dma_datos.channel)
        self._dma_ctrl.close()
        self._dma_datos.close()
        self._sm.active(0)
        self._sm = self._iniciar_sm()     # reinicia el PIO y vacia su FIFO
        for _ in range(self._por_piso):
            self._sm.put(0)               # un piso en cero: sin anodos ni catodos
        time.sleep_ms(2)
        self._sm.active(0)


def _abortar_dma(*canales):
    # Detiene los canales del todo. Primero los deshabilita y les quita el
    # encadenamiento (AL1_CTRL = 0, CHAIN_TO = el mismo canal), porque al abortar
    # un canal encadenado la RP2040 puede disparar al otro (errata RP2040-E13).
    mascara = 0
    for ch in canales:
        mem32[_DMA_BASE + ch * 0x40 + 0x10] = ch << _CHAIN_TO
        mascara |= 1 << ch
    mem32[_DMA_ABORT] = mascara
    t = time.ticks_ms()
    while mem32[_DMA_ABORT] & mascara and time.ticks_diff(time.ticks_ms(), t) < 10:
        pass


def _fuerza_pin(n):
    # Maxima corriente (12 mA) y flancos rapidos: el CLOCK y el LATCH van a 12
    # registros en paralelo y a 3.3 V la senal llega justa.
    # Tambien borra OD (bit 7, salida desactivada) e ISO (bit 8, aislamiento
    # del pad en la RP2350; en la RP2040 ese bit no existe).
    dir_pad = _PADS_BANK0 + 4 + 4 * n
    mem32[dir_pad] = (mem32[dir_pad] & ~0x1B0) | 0x30 | 0x01
