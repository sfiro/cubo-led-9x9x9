# Cubo 10x10x10 con Raspberry Pi Pico: arranca solo y recorre unas animaciones
# (v1). Las animaciones completas, el control desde la Raspberry Pi y la pagina
# con QR vienen en las versiones siguientes (ver cubo10/README.md).
#
# Necesita en la Pico: cubo.py (de pico_w/), este main.py y, si lo cambiaste,
# config.py.

import math
import random
import time

try:
    import config
except ImportError:
    import config_ejemplo as config
from cubo import Cubo

T = 10
try:
    c = Cubo(config.PIN_DATOS, config.PIN_RELOJ, config.PIN_LATCH, config.FREQ_PIO,
             bits=getattr(config, "BITS_BRILLO", 4), tam=T)
except TypeError:
    raise SystemExit("cubo.py es viejo: copia el cubo.py de pico_w/ del repositorio")
M = c.maximo
V = c.vox


def idx(z, y, x):
    return (z * T + y) * T + x


def oscurecer(factor):
    # Deja una estela: todos los LEDs bajan un poco de brillo
    for i in range(c.total):
        if V[i]:
            V[i] = V[i] * factor // 16


def durante(segundos):
    fin = time.ticks_add(time.ticks_ms(), int(segundos * 1000))
    while time.ticks_diff(fin, time.ticks_ms()) > 0:
        yield time.ticks_ms() / 1000


def planos(segundos=12):
    eje, k, paso = 0, 0, 1
    for _ in durante(segundos):
        oscurecer(9)
        c.plano(eje, k, 1)
        c.mostrar()
        time.sleep_ms(40)
        k += paso
        if k in (0, T - 1):
            paso = -paso
            if k == 0:
                eje = (eje + 1) % 3


def lluvia(segundos=12):
    gotas = []
    for _ in durante(segundos):
        oscurecer(10)
        if len(gotas) < 14:
            gotas.append([T - 1, random.randrange(T), random.randrange(T)])
        for g in gotas:
            V[idx(g[0], g[1], g[2])] = M
            g[0] -= 1
        gotas = [g for g in gotas if g[0] >= 0]
        c.mostrar()
        time.sleep_ms(50)


def onda(segundos=12):
    centro = (T - 1) / 2
    for t in durante(segundos):
        c.apagar()
        for y in range(T):
            for x in range(T):
                d = math.sqrt((x - centro) ** 2 + (y - centro) ** 2)
                z = (math.sin(d * 0.9 - t * 4) + 1) * (T - 1) / 2
                base = int(z)
                V[idx(base, y, x)] = M
                if base + 1 < T:        # el LED de arriba, segun que tan cerca este
                    V[idx(base + 1, y, x)] = int((z - base) * M)
        c.mostrar()


_DIST = bytearray(T * T * T)             # distancia al centro, en decimos
for _z in range(T):
    for _y in range(T):
        for _x in range(T):
            _DIST[idx(_z, _y, _x)] = int(10 * math.sqrt(sum((v - (T - 1) / 2) ** 2 for v in (_z, _y, _x))))


def esfera(segundos=12):
    for t in durante(segundos):
        r = int(10 * (1.5 + 3.3 * (math.sin(t * 2) + 1) / 2))
        for i in range(c.total):
            diferencia = abs(_DIST[i] - r)
            V[i] = M - diferencia * M // 8 if diferencia < 8 else 0
        c.mostrar()


def bordes(segundos=10):
    for t in durante(segundos):
        c.apagar()
        n = int((t * 6) % (2 * T))
        largo = n if n < T else 2 * T - n
        for a in range(largo):
            for p, q in ((0, 0), (0, T - 1), (T - 1, 0), (T - 1, T - 1)):
                V[idx(a, p, q)] = M
                V[idx(p, a, q)] = M
                V[idx(p, q, a)] = M
        c.mostrar()
        time.sleep_ms(30)


ANIMACIONES = (planos, lluvia, onda, esfera, bordes)

print("Cubo 10x10x10 listo: %d niveles de brillo" % (M + 1))
while True:
    for animacion in ANIMACIONES:
        print("-", animacion.__name__)
        animacion()
