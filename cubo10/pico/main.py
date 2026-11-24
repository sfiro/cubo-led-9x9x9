# Cubo 10x10x10 con Raspberry Pi Pico: arranca solo y recorre todas las
# animaciones en modo demo (v2). Usa las mismas animaciones del 9x9x9
# (pico_w/animaciones.py) en 10x10x10, mas las del sketch cuboLEDeeprom.
# El control desde la Raspberry Pi y la pagina con QR vienen en la v3.
#
# Necesita en la Pico, de pico_w/: cubo.py, animaciones.py y fuente.py; y de
# cubo10/pico/: este main.py y config_ejemplo.py (o tu config.py).

import asyncio
import sys

try:
    import config
except ImportError:
    import config_ejemplo as config
import animaciones
from cubo import Cubo

TAM = 10
# El reloj no va en el demo: la Pico sin WiFi no sabe la hora.
SIN_DEMO = ("reloj",)

if not hasattr(animaciones, "configurar"):
    raise SystemExit("animaciones.py es viejo: copia el de pico_w/ del repositorio")
animaciones.configurar(TAM)
try:
    cubo = Cubo(config.PIN_DATOS, config.PIN_RELOJ, config.PIN_LATCH, config.FREQ_PIO,
                bits=getattr(config, "BITS_BRILLO", 4), tam=TAM)
except TypeError:
    raise SystemExit("cubo.py es viejo: copia el cubo.py de pico_w/ del repositorio")


async def demo():
    while True:
        for nombre, animacion in animaciones.todas():
            if nombre in SIN_DEMO:
                continue
            print("-", nombre)
            try:
                await animacion(cubo)
            except Exception as e:          # una animacion con error no detiene el demo
                sys.print_exception(e)
                cubo.apagar()


print("Cubo 10x10x10 listo: %d animaciones, %d niveles de brillo"
      % (len(animaciones.todas()) - len(SIN_DEMO), cubo.maximo + 1))
asyncio.run(demo())
