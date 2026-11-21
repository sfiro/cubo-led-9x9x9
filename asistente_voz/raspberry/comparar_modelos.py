# Compara modelos de Ollama con las mismas preguntas y las mismas instrucciones
# que usa el asistente (personalidad, documentos de la tienda, estado del cubo).
# Mide cuanto tarda cada uno en empezar a hablar y muestra que responde, para
# elegir el mas rapido que todavia responda bien (v5.5).
#
#   python comparar_modelos.py qwen2.5:1.5b gemma3:1b llama3.2:1b qwen2.5:3b
#
# Los modelos tienen que estar descargados (ollama pull nombre). No usa el
# parlante ni el cubo.

import sys
import time

import asistente
import comandos
import conocimiento
from voz import Frases

PREGUNTAS = [
    "¿Cuánto valen los imanes?",
    "¿Qué es el champús?",
    "¿Qué lugares puedo visitar en Cali?",
    "¿Tienen tazas?",
    "¿Cómo hacen los productos?",
    "¿Cuál es el mejor restaurante de Cali?",   # no esta en los documentos: no deberia inventar
]


def preparar():
    # Lo mismo que hace asistente.principal() con los documentos
    config = asistente.config
    carpeta = conocimiento.carpeta_conocimiento(getattr(config, "CARPETA_CONOCIMIENTO", None))
    asistente.INSTRUCCIONES = (conocimiento.cargar_personalidad(carpeta) or config.INSTRUCCIONES) \
        + conocimiento.REGLAS
    return conocimiento.Conocimiento(
        carpeta, config.OLLAMA_URL, getattr(config, "MODELO_EMBEDDINGS", "bge-m3"),
        getattr(config, "FRAGMENTOS", 2), getattr(config, "UMBRAL_FRAGMENTO", 0.35),
        getattr(config, "UMBRAL_PREGUNTA_EXACTA", 0.88))


def mensaje_para(saber, pregunta):
    # Igual que asistente.responder(): estado del cubo, documentos y pregunta
    contexto, _ = saber.contexto(pregunta)
    mensaje = "(%s)\n" % comandos.estado_cubo()
    if contexto:
        mensaje += "Información que puede servir:\n%s\n" % contexto
    return mensaje + "Pregunta: %s" % pregunta


def probar(modelo, saber):
    asistente.config.MODELO = modelo
    extra = comandos.instrucciones_cubo()
    t = time.monotonic()
    for _ in asistente.preguntar([{"role": "user", "content": "Hola"}], extra):     # cargar
        pass
    print("\n### %s (cargado en %.1f s)" % (modelo, time.monotonic() - t))
    filas = []
    for pregunta in PREGUNTAS:
        frases = Frases()
        respuesta, primera = "", None
        t = time.monotonic()
        for pedazo in asistente.preguntar([{"role": "user", "content": mensaje_para(saber, pregunta)}], extra):
            respuesta += pedazo
            if frases.agregar(pedazo) and primera is None:
                primera = time.monotonic() - t
        total = time.monotonic() - t
        primera = primera if primera is not None else total
        filas.append((primera, total))
        print("- %s\n  %.1f s a la 1a frase, %.1f s en total | %s\n  > %s"
              % (pregunta, primera, total, asistente.resumen_ollama(), " ".join(respuesta.split())))
    promedio = sum(p for p, _ in filas) / len(filas)
    return modelo, promedio, sum(t for _, t in filas) / len(filas)


if __name__ == "__main__":
    modelos = sys.argv[1:] or [asistente.config.MODELO]
    saber = preparar()
    resultados = []
    for modelo in modelos:
        try:
            resultados.append(probar(modelo, saber))
        except Exception as e:
            print("\n### %s: ERROR %s (¿lo descargaste con ollama pull %s?)" % (modelo, e, modelo))
    print("\n== Resumen (promedio de %d preguntas) ==" % len(PREGUNTAS))
    for modelo, primera, total in sorted(resultados, key=lambda r: r[1]):
        print("  %-18s 1a frase %.1f s | respuesta completa %.1f s" % (modelo, primera, total))
    print("Lee tambien las respuestas: el mas rapido no sirve si inventa o habla mal.")
