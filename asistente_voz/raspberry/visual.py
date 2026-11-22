# Lo que muestra el cubo en cada estado del asistente. Cada funcion devuelve
# un arreglo (9, 9, 9) de booleanos indexado [z, y, x] (z = piso, 0 abajo).
#
# Animador es el hilo que elige la animacion segun el estado y manda los
# cuadros a la Pico CUADROS_POR_SEGUNDO veces por segundo.

import collections
import threading
import time

import numpy as np

from audio import nivel_db

N = 9
ZI, YI, XI = np.indices((N, N, N))              # indices enteros 0..8
Z, Y, X = ZI - 4.0, YI - 4.0, XI - 4.0          # centrados en el LED del medio
DIST = np.sqrt(X ** 2 + Y ** 2 + Z ** 2)        # distancia al centro
RADIO_H = np.sqrt(X ** 2 + Y ** 2)              # distancia al eje vertical
ANGULO = np.arctan2(Y, X)


def _banda(d, medio):
    # Brillo 0..1 que baja con la distancia d: 1 en el centro, 0.5 a "medio" y 0
    # al doble. Con la Pico de 1 bit (encendido si >= 0.5) queda la misma forma
    # que en la v1, donde se encendia lo que estaba a menos de "medio".
    return np.clip(1 - np.abs(d) / (2 * medio), 0, 1)


def reposo(t):
    # Esfera hueca que respira cada 3 s, con los bordes difuminados
    r = 2.2 + 0.9 * np.sin(t * 2 * np.pi / 3)
    return _banda(DIST - r, 0.55)


def escuchando(t, nivel):
    # Cilindro que se ensancha y crece con el volumen de quien habla
    r = 1.0 + 2.8 * nivel
    alto = np.clip(0.5 + 3.6 * nivel - np.abs(Z) + 0.5, 0, 1)
    return np.minimum(_banda(RADIO_H - r, 0.6), alto)


def pensando(t):
    # Doble helice girando, como si estuviera procesando
    dif = np.abs(np.angle(np.exp(1j * (ANGULO - 3.5 * t - 0.5 * Z))))     # 0..pi
    al_brazo = np.minimum(dif, np.pi - dif)                                 # a cualquiera de los 2
    return np.minimum(_banda(al_brazo, 0.45), _banda(RADIO_H - 3.0, 0.8))


# ---- mientras habla: reciben el nivel (0..1) y 9 bandas (0..1) --------------


def esfera(t, nivel, bandas):
    # Una esfera que se infla con cada silaba
    return _banda(DIST - (0.6 + 3.6 * nivel), 0.6)


def espectro(t, nivel, bandas):
    # Ecualizador: cada columna x es una banda (graves a la izquierda). El LED
    # de arriba de cada barra queda a medio brillo segun cuanto le falta.
    alto = np.clip(bandas * 9 * 1.3, 0, 9)[XI]
    return np.clip(alto - ZI + 0.5, 0, 1) * (np.abs(Y) <= alto * 4 / 9 + 0.01)


class Ondas:
    # La voz sale del centro como ondas: el nucleo late con el volumen actual
    # y cada cascara muestra el volumen de unos cuadros atras, cada vez mas tenue.
    RADIOS = (1.8, 3.4, 5.0)
    RETARDO = 3                     # cuadros entre una cascara y la siguiente

    def __init__(self):
        self._historia = collections.deque([0.0] * 16, maxlen=16)

    def __call__(self, t, nivel, bandas):
        self._historia.appendleft(nivel)
        vox = np.clip((0.3 + 1.3 * nivel - DIST) / 0.6 + 0.5, 0, 1)      # nucleo
        for k, r in enumerate(self.RADIOS, 1):
            v = self._historia[k * self.RETARDO]
            if v > 0.2:
                luz = (0.5 + 0.5 * v) * (1 - 0.2 * (k - 1))
                vox = np.maximum(vox, _banda(DIST - r, 0.2 + 0.2 * v) * luz)
        return vox


MODOS_HABLANDO = {
    "ondas": Ondas,
    "esfera": lambda: esfera,
    "espectro": lambda: espectro,
}


class Animador(threading.Thread):
    # estado: "reposo", "escuchando" o "pensando". Mientras el reproductor
    # tiene audio sonando, el estado es "hablando" automaticamente.
    # motor (v2): si hay una animacion elegida, se muestra en vez del reposo.
    # Con voz_sobre_animacion, al escuchar, pensar y hablar se ve la voz y luego
    # vuelve la animacion; sin eso, la animacion no se interrumpe.
    # v3, si la Pico tiene niveles de brillo:
    #   brillo: 0..1, multiplica todo el cuadro (se cambia con la voz);
    #   estela_ms: los LEDs que se apagan en una animacion se desvanecen en ese tiempo.
    def __init__(self, cubo, reproductor=None, microfono=None, modo="ondas", fps=40,
                 motor=None, voz_sobre_animacion=True, brillo=1.0, estela_ms=150):
        super().__init__(daemon=True)
        if modo not in MODOS_HABLANDO:
            raise ValueError("Animacion desconocida: %s (opciones: %s)"
                             % (modo, ", ".join(MODOS_HABLANDO)))
        self.cubo = cubo
        self.reproductor = reproductor
        self.microfono = microfono
        self.motor = motor
        self.voz_sobre_animacion = voz_sobre_animacion
        self.estado = "reposo"
        self.atento = True          # v4: dormido (esperando "hola cubo") = reposo tenue y lento
        self._hablando = MODOS_HABLANDO[modo]()
        self._periodo = 1.0 / fps
        self.brillo = brillo
        # cuanto queda de un LED apagado despues de un cuadro: llega al 10 % en estela_ms
        self._estela = 0.1 ** (self._periodo * 1000 / estela_ms) if estela_ms > 0 else 0.0
        self._rastro = np.zeros((N, N, N), np.float32)
        self._parar = threading.Event()
        self._ultimo_error = None

    def cuadro(self, t):
        # Devuelve (cuadro, si es la animacion elegida)
        animacion = self.motor.cuadro() if self.motor is not None else None
        if animacion is not None and not self.voz_sobre_animacion:
            return animacion, True
        if self.reproductor is not None and self.reproductor.sonando():
            nivel, bandas = self.reproductor.estado_audio()
            return self._hablando(t, nivel, bandas), False
        # v5.11: una animacion que pidieron sigue en el cubo mientras escucha o piensa;
        # antes cualquier ruido de la tienda la tapaba con "escuchando"
        if animacion is not None:
            return animacion, True
        if self.estado == "escuchando":
            return escuchando(t, nivel_db(self.microfono.rms) if self.microfono else 0.0), False
        if self.estado == "pensando":
            return pensando(t), False
        if not self.atento and getattr(self.cubo, "bits", 1) > 1:
            return reposo(t * 0.6) * 0.5, False
        return reposo(t), False

    def componer(self, t):
        # El cuadro que se manda: con estela y brillo, si la Pico tiene niveles
        vox, es_animacion = self.cuadro(t)
        if getattr(self.cubo, "bits", 1) == 1:
            return vox
        v = np.asarray(vox, np.float32)
        if es_animacion and self._estela > 0:
            self._rastro = np.maximum(v, self._rastro * self._estela)
            v = self._rastro
        else:
            self._rastro = v        # al volver a la animacion, la voz se desvanece
        return v * self.brillo

    def run(self):
        inicio = time.monotonic()
        siguiente = inicio
        while not self._parar.is_set():
            vox = self.componer(time.monotonic() - inicio)
            try:
                self.cubo.mostrar(vox)
            except Exception as e:          # cable suelto: avisar una vez y seguir
                if str(e) != self._ultimo_error:
                    print("\n[cubo] %s" % e)
                    self._ultimo_error = str(e)
            siguiente += self._periodo
            espera = siguiente - time.monotonic()
            if espera > 0:
                time.sleep(espera)
            else:
                siguiente = time.monotonic()

    def detener(self):
        self._parar.set()
        self.join(1)
        try:
            self.cubo.apagar()
        except Exception:
            pass
