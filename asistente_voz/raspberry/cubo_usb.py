# Conexion con la Pico por USB: manda cada cuadro como una linea de texto.
# El protocolo esta en ../model.md (seccion 3).
#
# Para probar sin el cubo, abrir_cubo("consola") devuelve un cubo falso que
# dibuja en la terminal una barra con la cantidad de LEDs encendidos, y
# abrir_cubo("silencio") uno que no muestra nada.

import time

import numpy as np

VID_RASPBERRY = 0x2E8A      # fabricante USB de la Pico
BYTES_CUADRO = 92           # 1 bit por LED
BYTES_CUADRO_4BITS = 365    # 4 bits por LED (v3)
MAXIMO = 15                 # brillo maximo en un cuadro de 4 bits


def buscar_puerto():
    from serial.tools import list_ports
    for p in list_ports.comports():
        if p.vid == VID_RASPBERRY:
            return p.device
    return None


# Los cuadros son arreglos (9, 9, 9) indexados [z, y, x]. Pueden ser:
#   - booleanos: encendido/apagado;
#   - decimales 0..1 (v3): brillo lineal, se corrige con gamma para que se vea parejo;
#   - enteros 0..15 (v3): niveles que van tal cual a la Pico.


def binario(vox):
    # Encendido/apagado, para la Pico sin niveles de brillo (firmware de la v1)
    vox = np.asarray(vox)
    if vox.dtype == bool:
        return vox
    if np.issubdtype(vox.dtype, np.integer):
        return vox > MAXIMO // 2
    return vox >= 0.5


def a_niveles(vox, gamma=2.2):
    vox = np.asarray(vox)
    if vox.dtype == bool:
        return vox.astype(np.uint8) * MAXIMO
    if np.issubdtype(vox.dtype, np.integer):
        return np.clip(vox, 0, MAXIMO).astype(np.uint8)
    return np.rint(MAXIMO * np.clip(vox, 0, 1) ** gamma).astype(np.uint8)


def empaquetar(vox):
    # -> 92 bytes. Bit b del byte k = LED 8k+b, con LED = z*81 + y*9 + x (igual que la Pico)
    return np.packbits(binario(vox).reshape(-1), bitorder="little").tobytes()


def empaquetar4(vox, gamma=2.2):
    # -> 365 bytes. El LED 2k va en los 4 bits bajos del byte k, el 2k+1 en los altos
    q = np.append(a_niveles(vox, gamma).reshape(-1), 0)
    return (q[0::2] | (q[1::2] << 4)).astype(np.uint8).tobytes()


class CuboUSB:
    # bits: niveles de brillo que tiene la Pico (1 con el firmware de la v1, 4 desde la v3)
    def __init__(self, puerto=None, gamma=2.2):
        import serial
        puerto = puerto or buscar_puerto()
        if puerto is None:
            raise RuntimeError("No encontre la Pico por USB. Revisa el cable o pon "
                               "PUERTO_CUBO en config.py")
        self.puerto = puerto
        self.gamma = gamma
        self.bits = 1
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
            linea = self._s.readline()
            if b"CUBO9" in linea:
                # "CUBO9 2 4" = firmware 2, 4 bits de brillo; "CUBO9 1" = firmware de la v1
                partes = linea.split(b"CUBO9", 1)[1].split()
                if len(partes) >= 2 and partes[1].isdigit():
                    self.bits = int(partes[1])
                self._s.timeout = 0
                return True
        return False

    def mostrar(self, vox):
        vox = np.asarray(vox)
        if self.bits == 1 or vox.dtype == bool:     # encendido/apagado: el cuadro corto alcanza
            self._s.write(b"F" + empaquetar(vox).hex().encode() + b"\n")
        else:
            self._s.write(b"G" + empaquetar4(vox, self.gamma).hex().encode() + b"\n")
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
    # Cubo falso: muestra en la terminal cuantos LEDs estarian encendidos.
    # Con barra=False no muestra nada (para no tapar lo que se escribe).
    bits = 4

    def __init__(self, barra=True):
        self.puerto = "consola" if barra else "ninguno"
        self._barra = barra

    def mostrar(self, vox):
        if not self._barra:
            return
        n = int(np.count_nonzero(vox))
        print("\r[%-60s] %3d LEDs" % ("#" * (n * 60 // 729), n), end="", flush=True)

    def apagar(self):
        pass

    def cerrar(self):
        print()


def abrir_cubo(puerto=None, gamma=2.2):
    if puerto in ("consola", "silencio"):
        return CuboConsola(barra=puerto == "consola")
    return CuboUSB(puerto, gamma)
