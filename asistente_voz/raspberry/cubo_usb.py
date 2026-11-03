# Conexion con la Pico por USB: manda cada cuadro como una linea de texto.
# El protocolo esta en ../model.md (seccion 3).
#
# Para probar sin el cubo, abrir_cubo("consola") devuelve un cubo falso que
# dibuja en la terminal una barra con la cantidad de LEDs encendidos.

import time

import numpy as np

VID_RASPBERRY = 0x2E8A      # fabricante USB de la Pico
BYTES_CUADRO = 92


def buscar_puerto():
    from serial.tools import list_ports
    for p in list_ports.comports():
        if p.vid == VID_RASPBERRY:
            return p.device
    return None


def empaquetar(vox):
    # vox: arreglo (9, 9, 9) de booleanos indexado [z, y, x] -> 92 bytes.
    # Bit b del byte k = LED 8k+b, con LED = z*81 + y*9 + x (igual que la Pico)
    return np.packbits(np.asarray(vox, dtype=bool).reshape(-1), bitorder="little").tobytes()


class CuboUSB:
    def __init__(self, puerto=None):
        import serial
        puerto = puerto or buscar_puerto()
        if puerto is None:
            raise RuntimeError("No encontre la Pico por USB. Revisa el cable o pon "
                               "PUERTO_CUBO en config.py")
        self.puerto = puerto
        self._s = serial.Serial(puerto, 115200, timeout=0.2, write_timeout=0.5)
        if not self._saludar():
            self._s.close()
            raise RuntimeError("La Pico en %s no responde. Revisa que tenga el main.py de "
                               "asistente_voz/pico y que Thonny este cerrado." % puerto)

    def _saludar(self):
        self._s.reset_input_buffer()
        self._s.write(b"\n?\n")
        limite = time.monotonic() + 1.5
        while time.monotonic() < limite:
            if b"CUBO9" in self._s.readline():
                self._s.timeout = 0
                return True
        return False

    def mostrar(self, vox):
        self._s.write(b"F" + empaquetar(vox).hex().encode() + b"\n")
        if self._s.in_waiting:          # descartar lo que imprima la Pico
            self._s.read(self._s.in_waiting)

    def apagar(self):
        self._s.write(b"A\n")

    def cerrar(self):
        try:
            self.apagar()
            self._s.flush()
        finally:
            self._s.close()


class CuboConsola:
    # Cubo falso: muestra en la terminal cuantos LEDs estarian encendidos
    puerto = "consola"

    def mostrar(self, vox):
        n = int(np.count_nonzero(vox))
        print("\r[%-60s] %3d LEDs" % ("#" * (n * 60 // 729), n), end="", flush=True)

    def apagar(self):
        pass

    def cerrar(self):
        print()


def abrir_cubo(puerto=None):
    if puerto == "consola":
        return CuboConsola()
    return CuboUSB(puerto)
