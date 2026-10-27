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


def _i(v):
    # Redondea al entero mas cercano (int() redondea hacia cero y pegaria
    # al borde lo que se sale por el lado negativo).
    return math.floor(v + 0.5)


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


_DIRECCIONES = ((1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1))


def _anillo(c, centro, eje, r):
    # Contorno cuadrado de radio r en el plano perpendicular al eje (0=z, 1=y, 2=x)
    a, b = [k for k in range(3) if k != eje]
    for i in range(-r, r + 1):
        for j in range(-r, r + 1):
            if max(abs(i), abs(j)) == r:
                p = list(centro)
                p[a] += i
                p[b] += j
                c.led(p[0], p[1], p[2], 1)


async def disparos(c, duracion=10000):
    # Proyectiles con estela que salen de las 6 caras a la vez; al chocar con
    # la pared opuesta dejan una onda expansiva cuadrada.
    balas = []          # [posicion [z, y, x], direccion]
    impactos = []       # [posicion, eje, edad]
    sigue = _durante(duracion)
    while sigue() or balas or impactos:
        if sigue() and len(balas) < 4 and _r(4) == 0:
            d = random.choice(_DIRECCIONES)
            eje = d.index(1) if 1 in d else d.index(-1)
            p = [_r(7) + 1, _r(7) + 1, _r(7) + 1]
            p[eje] = 0 if d[eje] > 0 else M
            balas.append([p, d])

        c.apagar()
        for b in balas[:]:
            p, d = b
            for k in range(4):                      # cabeza + 3 de estela
                c.led(p[0] - d[0] * k, p[1] - d[1] * k, p[2] - d[2] * k, 1)
            siguiente = [p[0] + d[0], p[1] + d[1], p[2] + d[2]]
            if all(0 <= v < N for v in siguiente):
                b[0] = siguiente
            else:
                eje = d.index(1) if 1 in d else d.index(-1)
                impactos.append([p, eje, 0])
                balas.remove(b)
        for imp in impactos[:]:
            p, eje, edad = imp
            _anillo(c, p, eje, edad)
            imp[2] += 1
            if imp[2] > 3:
                impactos.remove(imp)
        await _paso(c, 35)
    c.apagar()


async def planos(c):
    # 1) Dos planos que se abren desde el centro por cada eje.
    for eje in range(3):
        for k in list(range(5)) + list(range(3, -1, -1)):
            c.apagar()
            c.plano(eje, 4 - k, 1)
            c.plano(eje, 4 + k, 1)
            await _paso(c, 60)

    # 2) Plano diagonal x + y + z = k que atraviesa el cubo de esquina a esquina.
    for k in list(range(3 * M + 1)) + list(range(3 * M - 1, -1, -1)):
        c.apagar()
        for z in range(N):
            for y in range(N):
                x = k - z - y
                if 0 <= x < N:
                    c.led(z, y, x, 1)
        await _paso(c, 45)

    # 3) Plano que pasa por el centro y gira, inclinandose de a poco.
    for paso in range(110):
        # Normal en esfericas: gira alrededor del eje vertical (a) mientras
        # su inclinacion oscila entre casi horizontal y casi vertical.
        a = paso * 0.09
        inclinacion = 0.85 + 0.6 * math.sin(paso * 0.05)
        nz = math.cos(inclinacion)
        ny = math.sin(inclinacion) * math.cos(a)
        nx = math.sin(inclinacion) * math.sin(a)
        c.apagar()
        for z in range(N):
            pz = (z - 4) * nz
            for y in range(N):
                pzy = pz + (y - 4) * ny
                for x in range(N):
                    if abs(pzy + (x - 4) * nx) < 0.5:     # distancia al plano
                        c.led(z, y, x, 1)
        await _paso(c, 40)
    c.apagar()


async def flash(c):
    for _ in range(3):
        c.apagar()
        await _paso(c, 100)
        c.encender()
        await _paso(c, 100)
    c.apagar()
    await _paso(c, 100)


async def lluvia(c, duracion=10000):
    # Lluvia continua: gotas con estela que caen a distinta velocidad, se
    # inclinan con un viento que cambia y salpican al llegar al piso.
    gotas = []          # [z, y, x, velocidad]
    salpicaduras = []   # [y, x, edad]
    t = 0
    sigue = _durante(duracion)
    while sigue() or gotas or salpicaduras:
        t += 1
        viento = 0.12 * math.sin(t * 0.03)
        if sigue():
            for _ in range(_r(3)):
                gotas.append([M + 0.0, _r() + 0.0, _r() + 0.0, 0.3 + _r(50) / 100])

        c.apagar()
        for g in gotas[:]:
            g[0] -= g[3]
            g[2] += viento
            if g[0] < 0:
                salpicaduras.append([_i(g[1]), _i(g[2]), 0])
                gotas.remove(g)
                continue
            z, y, x = _i(g[0]), _i(g[1]), _i(g[2])
            c.led(z, y, x, 1)
            if g[3] > 0.55:                                 # las rapidas dejan estela
                c.led(z + 1, y, _i(g[2] - viento), 1)
        for s in salpicaduras[:]:
            y, x, edad = s
            if edad == 0:
                c.led(0, y, x, 1)
            elif edad == 1:
                for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    c.led(0, y + dy, x + dx, 1)
                c.led(1, y, x, 1)                           # la gotita que rebota
            else:
                for dy, dx in ((1, 1), (1, -1), (-1, 1), (-1, -1)):
                    c.led(0, y + dy, x + dx, 1)
            s[2] += 1
            if s[2] > 2:
                salpicaduras.remove(s)
        await _paso(c, 40)
    c.apagar()


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


# ---------------------------------------------------------------------------
# Animaciones con fisica y geometria

def _linea(c, a, b, v=1):
    # Linea 3D entre dos puntos (z, y, x), que pueden tener decimales.
    dz, dy, dx = b[0] - a[0], b[1] - a[1], b[2] - a[2]
    pasos = int(max(abs(dz), abs(dy), abs(dx))) + 1
    for i in range(pasos + 1):
        t = i / pasos
        c.led(_i(a[0] + dz * t), _i(a[1] + dy * t), _i(a[2] + dx * t), v)


def _limitar(v):
    return 0 if v < 0 else (M if v > M else v)


async def cubo_giratorio(c, duracion=10000):
    # Cubo de alambre que gira en dos ejes y "respira" (cambia de tamano)
    vertices = [(z, y, x) for z in (-1, 1) for y in (-1, 1) for x in (-1, 1)]
    aristas = [(i, j) for i in range(8) for j in range(i + 1, 8)
               if sum(1 for k in range(3) if vertices[i][k] != vertices[j][k]) == 1]
    t = 0.0
    sigue = _durante(duracion)
    while sigue():
        lado = 1.9 + 0.7 * math.sin(t * 0.8)
        ca, sa = math.cos(t), math.sin(t)
        cb, sb = math.cos(t * 0.6), math.sin(t * 0.6)
        puntos = []
        for z, y, x in vertices:
            y1, x1 = y * ca - x * sa, y * sa + x * ca          # giro sobre el eje vertical
            z2, y2 = z * cb - y1 * sb, z * sb + y1 * cb        # giro sobre el eje x
            puntos.append((4 + z2 * lado, 4 + y2 * lado, 4 + x1 * lado))
        c.apagar()
        for i, j in aristas:
            _linea(c, puntos[i], puntos[j])
        t += 0.06
        await _paso(c, 40)
    c.apagar()


async def helice(c, duracion=10000):
    # Doble helice tipo ADN que gira, con peldanos cada dos pisos
    t = 0.0
    sigue = _durante(duracion)
    while sigue():
        c.apagar()
        for z in range(N):
            a = t + z * 0.7
            ya, xa = 4 + 3.6 * math.cos(a), 4 + 3.6 * math.sin(a)
            yb, xb = 8 - ya, 8 - xa
            c.led(z, _i(ya), _i(xa), 1)
            c.led(z, _i(yb), _i(xb), 1)
            if z % 2 == 0:
                _linea(c, (z, ya, xa), (z, yb, xb))
        t += 0.15
        await _paso(c, 50)
    c.apagar()


async def tornado(c, duracion=10000):
    # Particulas que suben en espiral; el embudo se ensancha arriba y su base
    # se pasea por el piso.
    particulas = []     # [altura, angulo, velocidad de giro]
    t = 0
    sigue = _durante(duracion)
    while sigue() or particulas:
        t += 1
        by = 4 + 1.6 * math.sin(t * 0.05)
        bx = 4 + 1.6 * math.cos(t * 0.037)
        if sigue():
            for _ in range(2):
                particulas.append([0.0, _r(628) / 100, 0.25 + _r(20) / 100])
        c.apagar()
        for p in particulas[:]:
            p[0] += 0.16
            p[1] += p[2]
            if p[0] > M + 0.4:
                particulas.remove(p)
                continue
            k = p[0] / M                                  # 0 abajo, 1 arriba
            cy, cx = by + (4 - by) * k, bx + (4 - bx) * k
            r = 0.5 + 3.4 * k
            c.led(_i(p[0]), _i(cy + r * math.cos(p[1])), _i(cx + r * math.sin(p[1])), 1)
        await _paso(c, 40)
    c.apagar()


async def rebotes(c, duracion=12000):
    # Tres pelotas con gravedad que rebotan en el piso y las paredes
    bolas = []
    for _ in range(3):
        bolas.append([2.0 + _r(5), 1.0 + _r(7), 1.0 + _r(7),          # z, y, x
                      0.0, (_r(30) + 10) / 100 * random.choice((1, -1)),
                      (_r(30) + 10) / 100 * random.choice((1, -1))])  # vz, vy, vx
    sigue = _durante(duracion)
    while sigue():
        c.apagar()
        for b in bolas:
            b[3] -= 0.045                                 # gravedad
            for k in range(3):
                b[k] += b[k + 3]
                if b[k] < 1:
                    b[k] = 1.0
                    b[k + 3] = abs(b[k + 3]) * (0.9 if k == 0 else 1)
                    if k == 0 and b[3] < 0.3:             # se quedo sin energia: otro impulso
                        b[3] = 0.6 + _r(20) / 100
                elif b[k] > M - 1:
                    b[k] = M - 1.0
                    b[k + 3] = -abs(b[k + 3])
            z, y, x = _i(b[0]), _i(b[1]), _i(b[2])
            c.led(z, y, x, 1)
            for dz, dy, dx in _DIRECCIONES:
                c.led(z + dz, y + dy, x + dx, 1)
        await _paso(c, 35)
    c.apagar()


async def fuente(c, duracion=10000):
    # Chorro de agua desde el centro del piso que cae en parabola
    gotas = []          # [z, y, x, vz, vy, vx]
    sigue = _durante(duracion)
    while sigue() or gotas:
        if sigue():
            for _ in range(3):
                a = _r(628) / 100
                v = 0.06 + _r(8) / 100
                gotas.append([0.0, 4.0, 4.0, 0.85 + _r(25) / 100, v * math.cos(a), v * math.sin(a)])
        c.apagar()
        c.led(0, 4, 4, 1)                                 # boquilla
        for g in gotas[:]:
            g[3] -= 0.07
            g[0] += g[3]
            g[1] += g[4]
            g[2] += g[5]
            if g[0] < 0:
                c.led(0, _i(g[1]), _i(g[2]), 1)           # salpica en el piso
                gotas.remove(g)
            else:
                c.led(_i(g[0]), _i(g[1]), _i(g[2]), 1)
        await _paso(c, 40)
    c.apagar()


async def olas(c, duracion=10000):
    # Interferencia de dos fuentes de ondas que se mueven por el cubo
    t = 0.0
    sigue = _durante(duracion)
    while sigue():
        f1y, f1x = 4 + 4 * math.cos(t * 0.13), 4 + 4 * math.sin(t * 0.11)
        f2y, f2x = 4 - 4 * math.cos(t * 0.09), 4 - 4 * math.sin(t * 0.12)
        c.apagar()
        for y in range(N):
            for x in range(N):
                d1 = math.sqrt((y - f1y) ** 2 + (x - f1x) ** 2)
                d2 = math.sqrt((y - f2y) ** 2 + (x - f2x) ** 2)
                z = 4 + 2.0 * (math.sin(d1 * 1.1 - t) + math.sin(d2 * 1.1 - t))
                c.led(_limitar(_i(z)), y, x, 1)
        t += 0.25
        await _paso(c, 45)
    c.apagar()


# Regla de la vida 3D (cuenta los 26 vecinos). Se eligio simulando varias en un
# cubo de 9x9x9: con B5/S45 todo muere en ~10 generaciones y la mayoria de las
# que sobreviven parecen ruido. B5/S4567 forma estructuras que cambian despacio.
_NACE = (5,)
_SOBREVIVE = (4, 5, 6, 7)


def _sembrar():
    vivos = set()
    for z in range(2, 7):
        for y in range(2, 7):
            for x in range(2, 7):
                if _r(100) < 35:
                    vivos.add(z * 81 + y * 9 + x)
    return vivos


async def vida(c, generaciones=150):
    # Juego de la vida en 3D. El cubo esta conectado por los bordes (lo que sale
    # por un lado entra por el otro). Si la poblacion muere o se estanca, vuelve
    # a sembrar.
    vivos = _sembrar()
    historia = []
    for _ in range(generaciones):
        c.apagar()
        for i in vivos:
            c.led(i // 81, (i // 9) % 9, i % 9, 1)
        await _paso(c, 160)

        cuenta = {}
        for i in vivos:
            z, y, x = i // 81, (i // 9) % 9, i % 9
            for dz in (-1, 0, 1):
                zz = ((z + dz) % N) * 81
                for dy in (-1, 0, 1):
                    yy = zz + ((y + dy) % N) * 9
                    for dx in (-1, 0, 1):
                        j = yy + (x + dx) % N
                        cuenta[j] = cuenta.get(j, 0) + 1
        nuevos = set()
        for j, n in cuenta.items():
            if j in vivos:
                if n - 1 in _SOBREVIVE:                   # n incluye a la propia celda
                    nuevos.add(j)
            elif n in _NACE:
                nuevos.add(j)

        historia.append(len(nuevos))
        if not nuevos or (len(historia) >= 12 and len(set(historia[-12:])) == 1):
            vivos = _sembrar()
            historia = []
        else:
            vivos = nuevos
    c.apagar()


def _columnas_texto(mensaje):
    # Convierte el mensaje en columnas de 8 bits (bit i = fila i, 0 arriba).
    # Las letras usan solo sus columnas con pixeles (fuente proporcional).
    columnas = []
    for ch in mensaje:
        if ch == " ":
            columnas += [0, 0, 0]
            continue
        filas = caracter(ch)
        for j in range(8):
            col = 0
            for i in range(8):
                if (filas[i] >> (7 - j)) & 1:
                    col |= 1 << i
            if col:
                columnas.append(col)
        columnas.append(0)
    return columnas


# Recorrido de las 4 caras laterales, en el orden en que se leen desde afuera.
_PERIMETRO = ([(0, x) for x in range(N)] + [(y, M) for y in range(1, N)] +
              [(M, x) for x in range(M - 1, -1, -1)] + [(y, 0) for y in range(M - 1, 0, -1)])


async def marquesina(c, mensaje="CUBO LED 9X9X9   ", espera_ms=70):
    # Texto que corre alrededor de las cuatro caras del cubo
    columnas = _columnas_texto(mensaje)
    total = len(columnas)
    for s in range(total + len(_PERIMETRO)):
        c.apagar()
        for p, (y, x) in enumerate(_PERIMETRO):
            col = columnas[(p + s) % total]
            for i in range(8):
                if (col >> i) & 1:
                    c.led(M - i, y, x, 1)
        await _paso(c, espera_ms)
    c.apagar()


async def espectro(c, duracion=10000):
    # Ecualizador en cascada: el frente muestra 9 barras y cada cuadro
    # la medicion anterior se aleja hacia el fondo como una superficie.
    niveles = [0.0] * N
    historia = [[0] * N for _ in range(N)]
    sigue = _durante(duracion)
    while sigue():
        golpe = _r(8) == 0
        for x in range(N):
            objetivo = M if golpe and x < 4 else _r(N) * (1.2 - x * 0.08)
            niveles[x] += (objetivo - niveles[x]) * 0.5
        historia.pop()
        historia.insert(0, [_limitar(_i(v)) for v in niveles])
        c.apagar()
        for y in range(N):
            fila = historia[y]
            for x in range(N):
                if y == 0:
                    for z in range(fila[x] + 1):
                        c.led(z, 0, x, 1)
                else:
                    c.led(fila[x], y, x, 1)
        await _paso(c, 70)
    c.apagar()


# Orden del modo "demo". El nombre es el que aparece en la pagina web.
LISTA = (
    ("marquesina", marquesina),
    ("cubo_giratorio", cubo_giratorio),
    ("lluvia", lluvia),
    ("fuegos", fuegos),
    ("helice", helice),
    ("planos", planos),
    ("rebotes", rebotes),
    ("disparos", disparos),
    ("olas", olas),
    ("tornado", tornado),
    ("fuente", fuente),
    ("vida", vida),
    ("espectro", espectro),
    ("texto", texto),
    ("bordes", bordes),
    ("flash", flash),
    ("llenar_vaciar", llenar_vaciar),
    ("onda", onda),
    ("aleatorio", aleatorio),
    ("serpiente", serpiente),
    ("esfera", esfera),
    ("ascensores", ascensores),
)

POR_NOMBRE = dict(LISTA)
