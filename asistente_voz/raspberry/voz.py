# Texto a voz con Piper, y limpieza del texto que genera el modelo.

import os
import re

import numpy as np

AQUI = os.path.dirname(os.path.abspath(__file__))


class Voz:
    def __init__(self, ruta):
        from piper import PiperVoice
        ruta = os.path.join(AQUI, ruta)
        if not os.path.exists(ruta):
            raise FileNotFoundError("No esta la voz %s. Descargala con los pasos del README." % ruta)
        self._voz = PiperVoice.load(ruta)
        self.sr = self._voz.config.sample_rate

    def sintetizar(self, texto):
        # Devuelve las muestras float32 mono a self.sr
        v = self._voz
        if hasattr(v, "synthesize_stream_raw"):         # piper-tts 1.2
            datos = b"".join(v.synthesize_stream_raw(texto))
            return np.frombuffer(datos, np.int16).astype(np.float32) / 32768
        partes = [np.asarray(c.audio_float_array, np.float32)   # piper-tts 1.3 o mas nuevo
                  for c in v.synthesize(texto)]
        return np.concatenate(partes) if partes else np.zeros(0, np.float32)


_FORMATO = re.compile(r"[*#_`>\[\]|~]")
_EMOJIS = re.compile("[\U00010000-\U0010FFFF\u2600-\u27BF\uFE0F]")
_PENSAMIENTO = re.compile(r"<think>.*?</think>", re.S)
_FIN_FRASE = re.compile(r"[.!?…;:\n]+[\"')»]*\s")


def limpiar(texto):
    texto = " ".join(_EMOJIS.sub("", _FORMATO.sub("", texto)).split())
    return re.sub(r" ([?!.,;:])", r"\1", texto)


class Frases:
    # Junta los pedazos que llegan del modelo y entrega frases completas, para
    # empezar a hablar la primera mientras el modelo sigue escribiendo.
    MINIMO = 12             # no cortar en frases demasiado cortas ("Si.")

    def __init__(self):
        self._texto = ""

    def agregar(self, pedazo):
        self._texto += pedazo
        if "<think>" in self._texto:                    # modelos que "piensan" en voz alta
            if "</think>" not in self._texto:
                return []
            self._texto = _PENSAMIENTO.sub("", self._texto)
        frases = []
        inicio = 0
        for m in _FIN_FRASE.finditer(self._texto):
            if m.end() - inicio >= self.MINIMO:
                frases.append(self._texto[inicio:m.end()])
                inicio = m.end()
        self._texto = self._texto[inicio:]
        return [f for f in map(limpiar, frases) if f]

    def terminar(self):
        resto = limpiar(_PENSAMIENTO.sub("", self._texto).replace("<think>", ""))
        self._texto = ""
        return [resto] if resto else []
