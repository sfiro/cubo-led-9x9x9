# Corre en la Pi las mismas animaciones de la Pico (pico_w/animaciones.py), sin
# cambiarlas: se cargan desde ese archivo y se les reemplazan los modulos
# "time" y "asyncio" por equivalentes de Python normal (MicroPython tiene
# ticks_ms() y asyncio.sleep_ms(), Python no). Es lo mismo que hace el simulador.
#
# MotorAnimaciones las ejecuta en su propio hilo y deja el ultimo cuadro listo
# para que el Animador lo mande a la Pico.

import asyncio
import importlib.util
import os
import sys
import threading
import time

import numpy as np

PICO_W = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "pico_w")
TOTAL = 729


def _cargar(nombre):
    # Se carga por ruta, sin agregar pico_w/ a sys.path: alli hay otro config.py
    # (el de la Pico, con la clave del WiFi) que no debe mezclarse con el de la Pi.
    spec = importlib.util.spec_from_file_location(nombre, os.path.join(PICO_W, nombre + ".py"))
    modulo = importlib.util.module_from_spec(spec)
    sys.modules[nombre] = modulo
    spec.loader.exec_module(modulo)
    return modulo


class _Tiempo:
    # Reemplazo de "time" de MicroPython
    def __getattr__(self, nombre):
        return getattr(time, nombre)        # time(), localtime()... los reales

    def ticks_ms(self):
        return int(time.monotonic() * 1000)

    def ticks_add(self, a, b):
        return a + b

    def ticks_diff(self, a, b):
        return a - b

    def sleep_ms(self, ms):
        time.sleep(ms / 1000)


class _Asyncio:
    # Reemplazo de "asyncio" de MicroPython
    def __getattr__(self, nombre):
        return getattr(asyncio, nombre)

    async def sleep_ms(self, ms):
        await asyncio.sleep(ms / 1000)


_cargar("fuente")
animaciones = _cargar("animaciones")
animaciones.time = _Tiempo()
animaciones.asyncio = _Asyncio()
animaciones.ZONA_HORARIA = 0        # en la Pi, localtime() ya es la hora local
NOMBRES = [nombre for nombre, _ in animaciones.LISTA]


class CuboVirtual:
    # Misma interfaz que pico_w/cubo.py. mostrar() publica una copia del cuadro.
    def __init__(self):
        self.vox = bytearray(TOTAL)
        self._ceros = bytes(TOTAL)
        self._unos = b"\x01" * TOTAL
        self.cuadro = np.zeros((9, 9, 9), bool)

    def led(self, z, y, x, v=1):
        if 0 <= z < 9 and 0 <= y < 9 and 0 <= x < 9:
            self.vox[z * 81 + y * 9 + x] = 1 if v else 0

    def leer(self, z, y, x):
        if 0 <= z < 9 and 0 <= y < 9 and 0 <= x < 9:
            return self.vox[z * 81 + y * 9 + x]
        return 0

    def apagar(self):
        self.vox[:] = self._ceros

    def encender(self):
        self.vox[:] = self._unos

    def plano(self, eje, k, v=1):
        for a in range(9):
            for b in range(9):
                if eje == 0:
                    self.led(k, a, b, v)
                elif eje == 1:
                    self.led(a, k, b, v)
                else:
                    self.led(a, b, k, v)

    def mostrar(self):
        self.cuadro = np.frombuffer(bytes(self.vox), np.uint8).reshape(9, 9, 9).astype(bool)


class MotorAnimaciones:
    # La animacion elegida se repite hasta que se elige otra o se detiene.
    # "demo" las recorre todas.
    # al_empezar (v4.1): funcion que se llama en el modo demo antes de cada
    # animacion, con su nombre. Puede tardar (decir el nombre en voz alta): la
    # animacion empieza cuando termina. Corre en otro hilo, para no frenar el motor.
    def __init__(self, al_empezar=None):
        self.cubo = CuboVirtual()
        self.al_empezar = al_empezar
        self.actual = None              # nombre de la animacion, o None
        self.mensaje = None             # texto de la marquesina, si hay
        self._tarea = None
        self._loop = asyncio.new_event_loop()
        threading.Thread(target=self._loop.run_forever, daemon=True).start()

    def iniciar(self, nombre, mensaje=None):
        if nombre != "demo" and nombre not in animaciones.POR_NOMBRE:
            raise ValueError("Animacion desconocida: %s" % nombre)
        self.detener()
        self.actual = nombre
        self.mensaje = mensaje
        self._tarea = asyncio.run_coroutine_threadsafe(self._repetir(nombre, mensaje), self._loop)

    def detener(self):
        if self._tarea is not None:
            self._tarea.cancel()
            self._tarea = None
        self.actual = None
        self.mensaje = None
        self.cubo.apagar()
        self.cubo.mostrar()

    def cuadro(self):
        return self.cubo.cuadro if self.actual else None

    async def _repetir(self, nombre, mensaje):
        while True:
            lista = animaciones.LISTA if nombre == "demo" else [(nombre, animaciones.POR_NOMBRE[nombre])]
            for nombre_animacion, funcion in lista:
                try:
                    if nombre == "demo" and self.al_empezar is not None:
                        self.cubo.apagar()
                        self.cubo.mostrar()
                        await asyncio.get_running_loop().run_in_executor(
                            None, self.al_empezar, nombre_animacion)
                    if mensaje:
                        await funcion(self.cubo, mensaje=mensaje)
                    else:
                        await funcion(self.cubo)
                except asyncio.CancelledError:
                    raise
                except Exception as e:      # una animacion con error no frena el resto
                    print("[animacion %s] %s: %s" % (nombre, type(e).__name__, e))
                    await asyncio.sleep(0.5)
