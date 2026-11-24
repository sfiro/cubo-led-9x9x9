# Cubo simulado: misma interfaz que pico_w/cubo.py, pero en vez de sacar los
# bits por los 74HC595 manda cada cuadro al visor 3D del navegador.
# En el simulador se carga con el nombre "cubo", asi que "from cubo import Cubo"
# funciona igual que en la Pico.

from binascii import hexlify

import sim

TAM = 9                 # el worker lo cambia a 10 con ?tam=10 en la direccion


class Cubo:
    def __init__(self, *args, **kwargs):
        self.tam = n = kwargs.get("tam") or TAM
        total = n * n * n
        self.vox = bytearray(total)
        self._ceros = bytes(total)
        self._unos = b"\x01" * total
        self.fuera = 0          # LEDs dibujados fuera del cubo (se ignoran)

    def led(self, z, y, x, v=1):
        n = self.tam
        if 0 <= z < n and 0 <= y < n and 0 <= x < n:
            self.vox[(z * n + y) * n + x] = 1 if v else 0
        else:
            self.fuera += 1

    def leer(self, z, y, x):
        n = self.tam
        if 0 <= z < n and 0 <= y < n and 0 <= x < n:
            return self.vox[(z * n + y) * n + x]
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
        # En hexadecimal ("00"/"01" por LED): un byte 0 cortaria el texto al pasar a JS.
        sim.mostrar(hexlify(self.vox).decode(), self.fuera)

    def detener(self):
        self.apagar()
        self.mostrar()
