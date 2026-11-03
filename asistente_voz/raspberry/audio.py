# Audio: analisis del volumen y del espectro, reproductor sincronizado con el
# cubo y captura del microfono.

import collections
import queue
import threading
import time
import wave

import numpy as np

# ---- utilidades -------------------------------------------------------------


def remuestrear(x, sr_entrada, sr_salida):
    x = np.asarray(x, dtype=np.float32)
    if sr_entrada == sr_salida or len(x) == 0:
        return x
    if sr_entrada % sr_salida == 0:             # 48000 -> 16000: promedio de a 3
        r = sr_entrada // sr_salida
        return x[:len(x) // r * r].reshape(-1, r).mean(axis=1)
    n = int(round(len(x) * sr_salida / sr_entrada))
    t = np.arange(n) * (sr_entrada / sr_salida)
    return np.interp(t, np.arange(len(x)), x).astype(np.float32)


def leer_wav(ruta):
    # Devuelve (muestras float32 mono, frecuencia de muestreo)
    with wave.open(ruta) as w:
        sr, canales, ancho = w.getframerate(), w.getnchannels(), w.getsampwidth()
        datos = w.readframes(w.getnframes())
    if ancho != 2:
        raise ValueError("Solo WAV de 16 bits. Conviertelo con: "
                         "ffmpeg -i entrada.mp3 -ac 1 -ar 22050 salida.wav")
    x = np.frombuffer(datos, np.int16).astype(np.float32) / 32768
    if canales > 1:
        x = x.reshape(-1, canales).mean(axis=1)
    return x, sr


def nivel_db(rms):
    # -50 dBFS (silencio) -> 0 ... -10 dBFS (voz fuerte) -> 1
    return float(np.clip((20 * np.log10(rms + 1e-9) + 50) / 40, 0, 1))


# ---- analisis ---------------------------------------------------------------


class Analizador:
    # Convierte trozos de audio en un nivel (0..1) y N bandas de frecuencia
    # (0..1), suavizados: suben rapido y bajan lento, como un vumetro.
    def __init__(self, sr, bandas=9, f_min=120, f_max=5000):
        self.sr = sr
        self.nivel = 0.0
        self.bandas = np.zeros(bandas)
        self._bordes = np.geomspace(f_min, f_max, bandas + 1)
        self._cache = {}                # largo del trozo -> (ventana, bin de cada banda)
        self._maximo = 1e-3             # maximo reciente, para normalizar las bandas

    def _preparar(self, n):
        if n not in self._cache:
            f = np.fft.rfftfreq(n, 1 / self.sr)
            banda = np.searchsorted(self._bordes, f) - 1     # -1 o N = fuera de rango
            self._cache[n] = (np.hanning(n), banda)
        return self._cache[n]

    def procesar(self, x):
        x = np.asarray(x, dtype=np.float32)
        if len(x) < 16:
            return self.nivel, self.bandas.copy()
        objetivo = nivel_db(float(np.sqrt(np.mean(x * x))))
        k = 0.6 if objetivo > self.nivel else 0.2
        self.nivel += k * (objetivo - self.nivel)

        ventana, banda = self._preparar(len(x))
        espectro = np.abs(np.fft.rfft(x * ventana))
        n = len(self.bandas)
        validos = (banda >= 0) & (banda < n)
        suma = np.bincount(banda[validos], espectro[validos], minlength=n)
        cuenta = np.maximum(np.bincount(banda[validos], minlength=n), 1)
        e = np.log1p(suma / cuenta * 10)
        self._maximo = max(self._maximo * 0.995, float(e.max()))
        objetivo = e / self._maximo * self.nivel
        k = np.where(objetivo > self.bandas, 0.6, 0.2)
        self.bandas += k * (objetivo - self.bandas)
        return self.nivel, self.bandas.copy()


# ---- reproductor ------------------------------------------------------------


class Reproductor:
    # Reproduce trozos de audio en cola, sin cortes entre uno y otro. Por cada
    # bloque que manda al parlante anota cuando va a sonar y su nivel/bandas;
    # asi el cubo muestra exactamente lo que se esta escuchando.
    def __init__(self, dispositivo=None, ajuste_s=0.0, bloque_ms=20):
        import sounddevice as sd
        info = sd.query_devices(dispositivo, "output")
        self.sr = int(info["default_samplerate"])
        self._cola = collections.deque()
        self._pendiente = 0             # muestras en cola
        self._lock = threading.Lock()
        self._analizador = Analizador(self.sr)
        self._eventos = collections.deque()     # (cuando suena, nivel, bandas)
        self._actual = (0.0, np.zeros(9))
        self._fin_sonido = 0.0
        self._stream = sd.OutputStream(samplerate=self.sr, channels=1, dtype="float32",
                                       device=dispositivo, latency="low",
                                       blocksize=int(self.sr * bloque_ms / 1000),
                                       callback=self._llenar)
        self._retardo = self._stream.latency + ajuste_s
        self._stream.start()

    def agregar(self, audio, sr):
        audio = remuestrear(audio, sr, self.sr)
        with self._lock:
            self._cola.append(audio)
            self._pendiente += len(audio)

    def vaciar(self):
        with self._lock:
            self._cola.clear()
            self._pendiente = 0

    def _llenar(self, salida, frames, tiempo, estado):
        buf = salida[:, 0]
        n = 0
        with self._lock:
            while n < frames and self._cola:
                a = self._cola[0]
                k = min(frames - n, len(a))
                buf[n:n + k] = a[:k]
                if k == len(a):
                    self._cola.popleft()
                else:
                    self._cola[0] = a[k:]
                n += k
            self._pendiente -= n
        buf[n:] = 0
        if n:
            cuando = time.monotonic() + self._retardo
            nivel, bandas = self._analizador.procesar(buf)
            self._eventos.append((cuando, nivel, bandas))
            self._fin_sonido = cuando + frames / self.sr

    def sonando(self):
        return self._pendiente > 0 or time.monotonic() < self._fin_sonido

    def estado_audio(self):
        # Nivel y bandas del bloque que esta sonando en este momento
        ahora = time.monotonic()
        while self._eventos and self._eventos[0][0] <= ahora:
            _, nivel, bandas = self._eventos.popleft()
            self._actual = (nivel, bandas)
        if ahora > self._fin_sonido:
            return 0.0, np.zeros(9)
        return self._actual

    def esperar(self):
        while self.sonando():
            time.sleep(0.05)

    def cerrar(self):
        self._stream.stop()
        self._stream.close()


# ---- microfono --------------------------------------------------------------


class Microfono:
    # Captura continua. Deja el volumen actual en .rms y los trozos en .cola
    def __init__(self, dispositivo=None):
        import sounddevice as sd
        info = sd.query_devices(dispositivo, "input")
        self.sr = int(info["default_samplerate"])
        self.rms = 0.0
        self.cola = queue.Queue(maxsize=1000)
        self._stream = sd.InputStream(samplerate=self.sr, channels=1, dtype="float32",
                                      device=dispositivo, blocksize=int(self.sr * 0.03),
                                      callback=self._recibir)
        self._stream.start()

    def _recibir(self, datos, frames, tiempo, estado):
        x = datos[:, 0].copy()
        self.rms = float(np.sqrt(np.mean(x * x)))
        try:
            self.cola.put_nowait(x)
        except queue.Full:
            pass

    def vaciar(self):
        while not self.cola.empty():
            self.cola.get_nowait()

    def cerrar(self):
        self._stream.stop()
        self._stream.close()
