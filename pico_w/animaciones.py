# Animaciones del cubo 9x9x9, portadas de prueba_cubo_funcionLED_4.ino.
#
# Cada animacion es una corrutina que recibe el cubo, dibuja con c.led(z, y, x, v)
# y llama a "await _paso(c, ms)" para mostrar el cuadro y esperar.
# Como usan asyncio se pueden cancelar en cualquier momento (desde la pagina web).
#
# Para agregar una animacion: escribir "async def mi_animacion(c): ..." y
# sumarla a la lista LISTA al final del archivo.

import asyncio
import math
import random
import time

from fuente import caracter

N = 9
M = N - 1


def _r(n=N):
    return random.randrange(n)


def _durante(ms):
    fin = time.ticks_add(time.ticks_ms(), ms)
    return lambda: time.ticks_diff(fin, time.ticks_ms()) > 0


async def _paso(c, ms):
    c.mostrar()
    await asyncio.sleep_ms(ms)


# ---------------------------------------------------------------------------

async def aleatorio(c, duracion=8000):
    # LEDs que se encienden y apagan al azar
    c.apagar()
    sigue = _durante(duracion)
    while sigue():
        for _ in range(25):
            c.led(_r(), _r(), _r(), random.getrandbits(1))
        await _paso(c, 20)


async def llenar_vaciar(c):
    # aleatorioEncender + aleatorioApagar: se llena al azar y luego se vacia
    c.apagar()
    for v in (1, 0):
        sigue = _durante(5000)
        while sigue():
            for _ in range(3):
                c.led(_r(), _r(), _r(), v)
            await _paso(c, 10)


def _rayo(c, eje, a, b, k, v, cantidad):
    # Las mismas combinaciones de disparos / disparosDobles / disparosTriples
    if eje == 0:
        puntos = ((a, b, k), (k, b, a), (b, k, a))
    elif eje == 1:
        puntos = ((a, k, b), (a, b, k), (k, b, a))
    else:
        puntos = ((k, b, a), (b, k, a), (b, a, k))
    for i in range(cantidad):
        z, y, x = puntos[i]
        c.led(z, y, x, v)


async def disparos(c, cantidad=1, duracion=3000):
    c.apagar()
    for eje in range(3):
        sigue = _durante(duracion)
        while sigue():
            a, b = _r(), _r()
            for v in (1, 0):
                for k in range(N):
                    _rayo(c, eje, a, b, k, v, cantidad)
                    await _paso(c, 30)


async def disparos_triples(c):
    await disparos(c, 3)


async def planos(c, duracion=5000):
    # Un plano que sube y baja por cada eje
    c.apagar()
    for eje in range(3):
        sigue = _durante(duracion)
        while sigue():
            for k in list(range(N)) + list(range(M, -1, -1)):
                c.plano(eje, k, 1)
                await _paso(c, 50)
                c.plano(eje, k, 0)


async def flash(c):
    for _ in range(3):
        c.apagar()
        await _paso(c, 100)
        c.encender()
        await _paso(c, 100)
    c.apagar()
    await _paso(c, 100)


async def lluvia(c, duracion=6000):
    # Gotas que caen del piso de arriba al de abajo.
    # (En el original vector[1][6] se escribia fuera del arreglo.)
    c.apagar()
    sigue = _durante(duracion)
    while sigue():
        gotas = [(_r(), _r()) for _ in range(6)]
        for z in range(M, -1, -1):
            for y, x in gotas:
                c.led(z, y, x, 1)
            await _paso(c, 30)
            for y, x in gotas:
                c.led(z, y, x, 0)


def _aristas(c, etapa, k, v):
    if etapa == 0:
        lineas = ((0, 0, k), (0, k, 0), (k, 0, 0))
    elif etapa == 1:
        lineas = ((k, 0, M), (0, k, M), (0, M, k), (k, M, 0), (M, k, 0), (M, 0, k))
    else:
        lineas = ((M, M, k), (k, M, M), (M, k, M))
    for z, y, x in lineas:
        c.led(z, y, x, v)


async def bordes(c):
    # La funcion cubo(): se dibujan y borran las 12 aristas desde una esquina
    c.apagar()
    for v in (1, 0):
        for etapa in range(3):
            for k in range(N):
                _aristas(c, etapa, k, v)
                await _paso(c, 100)
        await _paso(c, 500)


async def ascensor(c, eje=0, invertir=False, espera_ms=50, suspendido_ms=1000):
    # effect_axis_updown_randsuspend. Corregido: usaba posiciones[x*8+y] y 7
    # (restos del cubo de 8x8x8), por eso fallaba una parte del cubo.
    pos = bytearray(81)
    destino = bytearray(_r() for _ in range(81))

    def dibujar():
        c.apagar()
        for a in range(N):
            for b in range(N):
                p = pos[a * N + b]
                if invertir:
                    p = M - p
                if eje == 0:
                    c.led(p, a, b, 1)
                elif eje == 1:
                    c.led(b, a, p, 1)
                else:
                    c.led(a, p, b, 1)

    for fase in range(2):
        for _ in range(N):
            for i in range(81):
                if pos[i] < destino[i]:
                    pos[i] += 1
                elif pos[i] > destino[i]:
                    pos[i] -= 1
            dibujar()
            await _paso(c, espera_ms)
        if fase == 0:
            destino = bytearray(b"\x08" * 81)
            await _paso(c, suspendido_ms)


async def ascensores(c):
    for eje in (0, 0, 2, 1):
        await ascensor(c, eje, False)
        await ascensor(c, eje, True)


async def fuegos(c, cantidad=10, particulas=30):
    # fireworks(): sube un cohete y explota con gravedad
    for _ in range(cantidad):
        c.apagar()
        ox = _r(4) + 2
        oy = _r(4) + 2
        oz = _r(2) + 5
        for z in range(oz):
            c.apagar()
            c.led(z, ox, oy, 1)
            await _paso(c, 30 + 20 * z)

        p = []
        for _ in range(particulas):
            p.append([ox, oy, oz,
                      1 - _r(200) / 100, 1 - _r(200) / 100, 1 - _r(200) / 100])

        for e in range(25):
            lento = 1 + math.tan((e + 0.1) / 20) * 10
            gravedad = math.tan((e + 0.1) / 20) / 2
            c.apagar()
            for q in p:
                q[0] += q[3] / lento
                q[1] += q[4] / lento
                q[2] += q[5] / lento - gravedad
                c.led(int(q[2]), int(q[0]), int(q[1]), 1)
            await _paso(c, 40)
    c.apagar()


async def esfera(c, duracion=6000):
    # Cascaras esfericas que crecen desde un punto al azar (esferaCascaraAleatoria)
    sigue = _durante(duracion)
    while sigue():
        cz, cy, cx = _r(), _r(), _r()
        for r in range(1, 13):
            r_ext = (r * 0.5) ** 2
            r_int = (r * 0.4) ** 2
            for z in range(N):
                for y in range(N):
                    for x in range(N):
                        d = (x - cx) ** 2 + (y - cy) ** 2 + (z - cz) ** 2
                        c.led(z, y, x, r_int <= d <= r_ext)
            await _paso(c, 30)


async def onda(c, duracion=8000):
    # Superficie senoidal que se mueve desde el centro
    distancias = [math.sqrt((x - 4) ** 2 + (y - 4) ** 2) for y in range(N) for x in range(N)]
    t = 0.0
    sigue = _durante(duracion)
    while sigue():
        c.apagar()
        for i in range(81):
            z = int(4.5 + 4 * math.sin(distancias[i] * 0.9 - t))
            c.led(z, i // N, i % N, 1)
        t += 0.3
        await _paso(c, 40)


def _letra(c, ch, plano, sobre_x):
    # Dibuja la letra (8x8) en el plano x = plano (o y = plano).
    # Corregido: el original leia una fila 9 que no existe en la fuente.
    filas = caracter(ch)
    for i in range(8):
        fila = filas[i]
        for b in range(8):
            v = (fila >> b) & 1
            if sobre_x:
                c.led(M - i, M - b, plano, v)
            else:
                c.led(M - i, plano, b + 1, v)


async def texto(c, mensaje="HOLA", espera_ms=50):
    # letras(): cada letra atraviesa el cubo, primero en X y luego en Y
    for sobre_x in (True, False):
        for ch in mensaje:
            for k in range(N):
                c.apagar()
                _letra(c, ch, k, sobre_x)
                await _paso(c, espera_ms)
    c.apagar()


async def serpiente(c, duracion=10000, largo=14):
    # Culebra que avanza al azar sin salirse del cubo
    c.apagar()
    cuerpo = [(4, 4, 4)]
    direcciones = ((1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1))
    d = direcciones[0]
    sigue = _durante(duracion)
    while sigue():
        z, y, x = cuerpo[-1]
        opciones = [d] * 4 + list(direcciones)    # tiende a seguir derecho
        while True:
            nd = random.choice(opciones)
            nz, ny, nx = z + nd[0], y + nd[1], x + nd[2]
            if 0 <= nz < N and 0 <= ny < N and 0 <= nx < N:
                break
        d = nd
        cuerpo.append((nz, ny, nx))
        c.led(nz, ny, nx, 1)
        if len(cuerpo) > largo:
            vz, vy, vx = cuerpo.pop(0)
            if (vz, vy, vx) not in cuerpo:
                c.led(vz, vy, vx, 0)
        await _paso(c, 70)
    c.apagar()


# Orden del modo "demo". El nombre es el que aparece en la pagina web.
LISTA = (
    ("texto", texto),
    ("bordes", bordes),
    ("fuegos", fuegos),
    ("lluvia", lluvia),
    ("flash", flash),
    ("disparos", disparos),
    ("llenar_vaciar", llenar_vaciar),
    ("onda", onda),
    ("aleatorio", aleatorio),
    ("planos", planos),
    ("serpiente", serpiente),
    ("disparos_triples", disparos_triples),
    ("esfera", esfera),
    ("ascensores", ascensores),
)

POR_NOMBRE = dict(LISTA)
