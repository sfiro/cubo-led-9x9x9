# Activacion con una frase: "hola cubo" (v4).
#
# Mientras esta dormido, el asistente transcribe con Whisper cada frase que
# oye, pero solo responde si contiene la frase de activacion. Despues queda
# atento un rato (la ventana de conversacion): se le puede seguir hablando
# sin repetir "hola cubo". Cada respuesta alarga la ventana.
#
# No hace falta entrenar un modelo: se reusa el Whisper que ya transcribe las
# preguntas, guiado con initial_prompt para que escriba bien la frase.

import difflib
import re
import time

from comandos import normalizar

SIMILITUD = 0.75            # cuanto se tiene que parecer cada palabra ("kubo" ~ "cubo")
_DESPEDIDA = re.compile(r"(?:adios|chao|chau|hasta luego|hasta manana|duermete|a dormir|"
                        r"eso es todo|nada mas)(?: cubo)?(?: gracias)?")


def _parecida(a, b):
    return a == b or difflib.SequenceMatcher(None, a, b).ratio() >= SIMILITUD


def buscar_frase(texto, frase):
    # Devuelve (encontrada, lo que se dijo despues). Tolera lo que Whisper suele
    # escribir mal: "Ola Kubo", "Tola, cubo", "Hoy os la cubo", "Hola Cuba".
    #   - la ultima palabra ("cubo") se compara palabra contra palabra;
    #   - lo de antes ("hola") se compara con las letras que la preceden, sin
    #     espacios, porque Whisper a veces la parte en dos ("to la").
    # "Hola, como estas" o "un cubo de hielo" no la despiertan.
    palabras = normalizar(texto).split()
    buscadas = normalizar(frase).split()
    ultima, antes = buscadas[-1], "".join(buscadas[:-1])
    for i, palabra in enumerate(palabras):
        if not _parecida(palabra, ultima):
            continue
        previas = "".join(palabras[max(0, i - 3):i])[-len(antes):] if antes else ""
        if not antes or (previas and difflib.SequenceMatcher(None, previas, antes).ratio() >= 0.7):
            return True, _resto(texto, i + 1)
    return False, ""


def _resto(texto, despues_de):
    # El texto original (con tildes y signos) desde la palabra numero "despues_de"
    partes = re.findall(r"\S+", texto)
    contadas = 0
    for j, parte in enumerate(partes):
        if normalizar(parte):
            contadas += 1
        if contadas == despues_de:
            return " ".join(partes[j + 1:]).lstrip(" ,.;:!?¡¿")
    return ""


def es_despedida(texto):
    return bool(_DESPEDIDA.fullmatch(normalizar(texto)))


class Vigilia:
    # Lleva la cuenta de si el asistente esta atento o dormido
    def __init__(self, ventana_s):
        self.ventana_s = ventana_s
        self._hasta = 0.0

    def atento(self, cuando=None):
        return (cuando or time.monotonic()) < self._hasta

    def despertar(self):
        self._hasta = time.monotonic() + self.ventana_s

    def dormir(self):
        self._hasta = 0.0
