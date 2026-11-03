# Prueba de la conexion Pi -> Pico -> cubo: enciende piso por piso, luego
# todo el cubo, y mide cuantos cuadros por segundo pasan por el USB.

import time

import numpy as np

try:
    import config
except ImportError:
    import config_ejemplo as config
from cubo_usb import CuboUSB

cubo = CuboUSB(config.PUERTO_CUBO)
print("Pico encontrada en", cubo.puerto)
try:
    for z in range(9):
        print("Piso", z)
        vox = np.zeros((9, 9, 9), bool)
        vox[z] = True
        cubo.mostrar(vox)
        time.sleep(0.4)
    print("Todo encendido")
    cubo.mostrar(np.ones((9, 9, 9), bool))
    time.sleep(1)

    print("Midiendo velocidad...")
    vox = np.zeros((9, 9, 9), bool)
    t = time.monotonic()
    for i in range(300):
        vox[:] = False
        vox.reshape(-1)[i % 729] = True
        cubo.mostrar(vox)
    s = time.monotonic() - t
    print("%.0f cuadros por segundo (el asistente usa %d)" % (300 / s, config.CUADROS_POR_SEGUNDO))
finally:
    cubo.cerrar()
