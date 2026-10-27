# Driver del cubo LED 9x9x9 para Raspberry Pi Pico W (MicroPython).
#
# Mismo hardware que con el Arduino: 12 registros 74HC595 en cadena
# (U1..U11 para los 81 anodos de un piso, U11 y U12 para los 9 catodos).
#
# El multiplexado de los pisos lo hacen el PIO y el DMA sin usar el CPU:
#   - el PIO saca los 96 bits de un piso por DATA/CLOCK, da el pulso de LATCH
#     y espera un rato antes de pasar al siguiente piso;
#   - dos canales de DMA le entregan el cuadro al PIO en un ciclo infinito.
# Asi el cubo no parpadea aunque Python este ocupado, recolectando basura
# o atendiendo el WiFi.
#
# Uso:
#   c = Cubo()
#   c.led(z, y, x, 1)      # z = piso, y = fila, x = columna (0..8)
#   c.mostrar()            # publica lo dibujado (doble buffer, sin parpadeo)

import array
import time

import micropython
import rp2
import uctypes
from machine import Pin, mem32
from micropython import const

TAM = const(9)
LEDS_PISO = const(81)
TOTAL = const(729)
_PALABRAS_CUADRO = const(36)            # 9 pisos x (3 palabras de datos + 1 de espera)
_BYTES_CUADRO = const(144)

_DMA_BASE = const(0x50000000)
_PADS_BANK0 = const(0x4001C000)


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


@micropython.viper
def _empaquetar(vox: ptr8, cuadro: ptr32, tabla: ptr8):
    # Convierte los 729 voxeles (1 byte cada uno) en las palabras que lee el PIO.
    # La 4a palabra de cada piso (la espera) no se toca.
    for n in range(9):
        b = n * 4
        cuadro[b] = 0
        cuadro[b + 1] = 0
        cuadro[b + 2] = 0
        base = n * 81
        for p in range(81):
            if vox[base + p]:
                s = tabla[p]
                i = b + (s >> 5)
                cuadro[i] = cuadro[i] | (1 << (31 - (s & 31)))
        s = tabla[81 + n]
        i = b + (s >> 5)
        cuadro[i] = cuadro[i] | (1 << (31 - (s & 31)))


class Cubo:
    def __init__(self, pin_datos=2, pin_reloj=3, pin_latch=4,
                 freq_pio=5_000_000, espera=1000, sm_id=0):
        # freq_pio: cada bit toma 2 ciclos -> 5 MHz da 2.5 MHz en CLOCK.
        #   Si aparecen LEDs locos (senal de 3.3 V debil), bajarla a 1_000_000.
        # espera: ciclos del PIO que cada piso queda encendido despues de
        #   cargarse (1000 a 5 MHz = 200 us -> ~460 cuadros por segundo).
        self.vox = bytearray(TOTAL)
        self._ceros = bytes(TOTAL)
        self._unos = b"\x01" * TOTAL
        self._args_sm = (sm_id, _pio_cubo, freq_pio, pin_datos, pin_reloj, pin_latch)

        self._cuadros = (array.array("I", [0] * _PALABRAS_CUADRO),
                         array.array("I", [0] * _PALABRAS_CUADRO))
        for cuadro in self._cuadros:
            for n in range(TAM):
                cuadro[n * 4 + 3] = espera
            _empaquetar(self.vox, cuadro, _TABLA)
        self._atras = 1
        self._dir = array.array("I", [uctypes.addressof(self._cuadros[0])])

        self._sm = self._iniciar_sm()
        for pin in (pin_datos, pin_reloj, pin_latch):
            _fuerza_pin(pin)

        pio, sm = sm_id // 4, sm_id % 4
        fifo_tx = 0x50200010 + pio * 0x100000 + sm * 4
        dreq = pio * 8 + sm

        self._dma_datos = rp2.DMA()
        self._dma_ctrl = rp2.DMA()
        self._reg_lectura = _DMA_BASE + self._dma_datos.channel * 0x40

        # Canal de datos: cuadro -> FIFO del PIO; al terminar dispara al de control.
        self._dma_datos.config(
            read=self._dir[0], write=fifo_tx, count=_PALABRAS_CUADRO,
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
        if 0 <= z < TAM and 0 <= y < TAM and 0 <= x < TAM:
            self.vox[z * 81 + y * 9 + x] = 1 if v else 0

    def leer(self, z, y, x):
        if 0 <= z < TAM and 0 <= y < TAM and 0 <= x < TAM:
            return self.vox[z * 81 + y * 9 + x]
        return 0

    def apagar(self):
        self.vox[:] = self._ceros

    def encender(self):
        self.vox[:] = self._unos

    def plano(self, eje, k, v=1):
        # eje 0: piso z = k, eje 1: fila y = k, eje 2: columna x = k
        for a in range(TAM):
            for b in range(TAM):
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
        while (ini <= mem32[self._reg_lectura] <= ini + _BYTES_CUADRO
               and time.ticks_diff(time.ticks_ms(), t) < 20):
            pass
        _empaquetar(self.vox, atras, _TABLA)
        self._dir[0] = ini
        self._atras ^= 1

    # ---- apagado ------------------------------------------------------------

    def detener(self):
        # Para el refresco y deja todos los LEDs apagados.
        self._dma_ctrl.close()
        self._dma_datos.close()
        self._sm.active(0)
        self._sm = self._iniciar_sm()     # reinicia el PIO y vacia su FIFO
        for _ in range(4):
            self._sm.put(0)               # 96 bits en cero: sin anodos ni catodos
        time.sleep_ms(2)
        self._sm.active(0)


def _fuerza_pin(n):
    # Maxima corriente (12 mA) y flancos rapidos: el CLOCK y el LATCH van a 12
    # registros en paralelo y a 3.3 V la senal llega justa.
    dir_pad = _PADS_BANK0 + 4 + 4 * n
    mem32[dir_pad] = (mem32[dir_pad] & ~0x30) | 0x30 | 0x01
