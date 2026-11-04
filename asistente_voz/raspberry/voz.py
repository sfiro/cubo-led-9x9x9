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
# Etiquetas para el cubo que escribe el modelo (v2): [ANIMACION: lluvia], [PARAR],
# y desde la v3 [BRILLO: 50]
_ETIQUETA = re.compile(r"\[\s*(ANIMACI[OÓ]N|TEXTO|PARAR|BRILLO)\s*(?::\s*([^\]]*))?\]", re.I)


def limpiar(texto):
    texto = " ".join(_EMOJIS.sub("", _FORMATO.sub("", texto)).split())
    return re.sub(r" ([?!.,;:])", r"\1", texto)


class Frases:
    # Junta los pedazos que llegan del modelo y entrega frases completas, para
    # empezar a hablar la primera mientras el modelo sigue escribiendo.
    # Las etiquetas para el cubo no se dicen: se guardan en .etiquetas como
    # pares (tipo, valor).
    MINIMO = 12             # no cortar en frases demasiado cortas ("Si.")

    def __init__(self):
        self._texto = ""
        self.etiquetas = []

    def _quitar_etiquetas(self, texto):
        def guardar(m):
            self.etiquetas.append((m.group(1), m.group(2)))
            return " "
        return _ETIQUETA.sub(guardar, texto)

    def agregar(self, pedazo):
        self._texto += pedazo
        if "<think>" in self._texto:                    # modelos que "piensan" en voz alta
            if "</think>" not in self._texto:
                return []
            self._texto = _PENSAMIENTO.sub("", self._texto)
        self._texto = self._quitar_etiquetas(self._texto)
        # Un "[" sin cerrar puede ser una etiqueta a medio llegar: se guarda para despues
        abierto = self._texto.rfind("[")
        corte = abierto if abierto >= 0 and "]" not in self._texto[abierto:] else len(self._texto)
        listo, pendiente = self._texto[:corte], self._texto[corte:]
        frases = []
        inicio = 0
        for m in _FIN_FRASE.finditer(listo):
            if m.end() - inicio >= self.MINIMO:
                frases.append(listo[inicio:m.end()])
                inicio = m.end()
        self._texto = listo[inicio:] + pendiente
        return [f for f in map(limpiar, frases) if f]

    def terminar(self):
        resto = self._quitar_etiquetas(_PENSAMIENTO.sub("", self._texto).replace("<think>", ""))
        resto = limpiar(re.sub(r"\[[^\]]*$", "", resto))      # etiqueta cortada al final
        self._texto = ""
        return [resto] if resto else []
