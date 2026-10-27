# Ejecuta las animaciones dentro del simulador.
#
# Las animaciones no se modifican: solo se reemplazan sus modulos "time" y
# "asyncio" por versiones con reloj virtual. Asi se puede pausar, avanzar
# cuadro a cuadro y cambiar la velocidad sin que las duraciones se descuadren
# (el reloj virtual solo avanza con las esperas de la animacion).

import sys
import time as _time
import asyncio as _asyncio

import sim
import animaciones
from cubo import Cubo


class _Reloj:
    # Reemplazo de "time": ticks_ms() devuelve el tiempo virtual.
    def __init__(self):
        self.ms = 0

    def __getattr__(self, nombre):
        # time(), localtime(), etc. siguen siendo los reales (para el reloj)
        return getattr(_time, nombre)

    def ticks_ms(self):
        return self.ms

    def ticks_add(self, a, b):
        return a + b

    def ticks_diff(self, a, b):
        return a - b

    def sleep_ms(self, ms):
        self.ms += ms

    def sleep(self, s):
        self.ms += int(s * 1000)


class _AsyncioSim:
    # Reemplazo de "asyncio": sleep_ms respeta pausa, paso y velocidad.
    def __init__(self):
        self.ultimo = _time.ticks_ms()

    def __getattr__(self, nombre):
        return getattr(_asyncio, nombre)

    async def sleep_ms(self, ms):
        sim.calculo(_time.ticks_diff(_time.ticks_ms(), self.ultimo), reloj.ms)
        reloj.ms += ms
        while sim.pausado() and not sim.tomar_paso():
            await _asyncio.sleep_ms(25)
        if not sim.pausado():
            await _asyncio.sleep_ms(max(0, int(ms / sim.velocidad())))
        self.ultimo = _time.ticks_ms()

    async def sleep(self, s):
        await self.sleep_ms(int(s * 1000))


reloj = _Reloj()
asyncio_sim = _AsyncioSim()


def _preparar(modulo):
    if hasattr(modulo, "asyncio"):
        modulo.asyncio = asyncio_sim
    if hasattr(modulo, "time"):
        modulo.time = reloj


class Runner:
    def __init__(self):
        self.cubo = Cubo()
        self.tarea = None
        _preparar(animaciones)

    def nombres(self):
        return ",".join(n for n, _ in animaciones.LISTA)

    def elegir(self, nombre):
        if nombre == "demo":
            funciones = [f for _, f in animaciones.LISTA]
        elif nombre in animaciones.POR_NOMBRE:
            funciones = [animaciones.POR_NOMBRE[nombre]]
        else:
            print("No existe la animacion", nombre)
            return
        self._lanzar(funciones)

    def editor(self, funcion):
        if "editor_anim" in sys.modules:
            del sys.modules["editor_anim"]
        try:
            import editor_anim
        except Exception as e:
            sys.print_exception(e)
            sim.fallo()
            return
        _preparar(editor_anim)
        f = getattr(editor_anim, funcion, None)
        if f is None:
            print("No encuentro la funcion", funcion, "en el editor")
            sim.fallo()
            return
        self._lanzar([f])

    def _lanzar(self, funciones):
        self.detener()
        reloj.ms = 0
        self.cubo.fuera = 0
        self.cubo.apagar()
        self.cubo.mostrar()
        asyncio_sim.ultimo = _time.ticks_ms()
        self.tarea = _asyncio.create_task(self._correr(funciones))

    async def _correr(self, funciones):
        try:
            while True:
                for f in funciones:
                    await f(self.cubo)
                if not sim.repetir():
                    break
            self.cubo.mostrar()
            sim.terminado()
        except Exception as e:
            sys.print_exception(e)
            sim.fallo()

    def detener(self):
        if self.tarea:
            self.tarea.cancel()
            self.tarea = None


runner = Runner()
