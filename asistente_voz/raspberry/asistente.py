# Fase 3: asistente de voz completo, todo local en la Raspberry Pi 5.
#
#   microfono -> faster-whisper -> Ollama -> Piper -> parlante
#                                              \-> cubo (al ritmo de la voz)
#
# v2: ademas se pueden elegir las animaciones con la voz ("pon la lluvia",
# "para", "escribe HOLA"), sin dejar de conversar con el modelo. Ver
# ../versiones/v2.md.
#
# Un turno a la vez: mientras el cubo habla, el microfono se ignora para que
# no se escuche a si mismo. Ctrl-C para salir.
#
#   python asistente.py               hablando por el microfono
#   python asistente.py --escribir    escribiendo las preguntas (para probar)
#   python asistente.py --sin-cubo    sin la Pico conectada

import argparse
import json
import queue
import threading
import time
import urllib.request

import numpy as np

try:
    import config
except ImportError:
    import config_ejemplo as config
import comandos
from animaciones_pi import MotorAnimaciones
from audio import Microfono, Reproductor, remuestrear
from cubo_usb import abrir_cubo
from visual import Animador
from voz import Frases, Voz

VERSION = "2.0"
SR_WHISPER = 16000


# ---- escuchar ---------------------------------------------------------------


def esperar_palabra(mic, detector):
    # Bloquea hasta que openWakeWord detecta la palabra de activacion
    resto = np.zeros(0, np.float32)
    while True:
        resto = np.concatenate([resto, remuestrear(mic.cola.get(), mic.sr, SR_WHISPER)])
        while len(resto) >= 1280:                       # trozos de 80 ms
            trozo = (resto[:1280] * 32767).astype(np.int16)
            resto = resto[1280:]
            if max(detector.predict(trozo).values()) > 0.5:
                detector.reset()
                return


def escuchar_pregunta(mic, anim, espera_max_s=None):
    # Espera a que alguien hable y graba hasta un silencio. Devuelve el audio a
    # 16 kHz, o None si no hubo voz (o fue solo un ruido corto).
    mic.vaciar()
    bloque_s = 0.03
    previo = []                     # 0.3 s antes de detectar la voz, para no cortar el inicio
    grabado = []
    silencio = 0.0
    esperado = 0.0
    while True:
        x = mic.cola.get()
        hay_voz = float(np.sqrt(np.mean(x * x))) > config.UMBRAL_VOZ
        if not grabado:
            previo = (previo + [x])[-10:]
            esperado += bloque_s
            if hay_voz:
                grabado = previo
                anim.estado = "escuchando"
            elif espera_max_s and esperado > espera_max_s:
                return None
            continue
        grabado.append(x)
        silencio = 0.0 if hay_voz else silencio + len(x) / mic.sr
        duracion = sum(len(g) for g in grabado) / mic.sr
        if silencio >= config.SILENCIO_FIN_S or duracion >= config.MAX_PREGUNTA_S:
            break
    if duracion - silencio < 0.4:
        return None
    return remuestrear(np.concatenate(grabado), mic.sr, SR_WHISPER)


# ---- pensar -----------------------------------------------------------------


def preguntar(historial, extra=""):
    # Pide la respuesta a Ollama y la entrega en pedazos a medida que se genera.
    # extra: lo que se agrega a las instrucciones (la lista de animaciones)
    cuerpo = {
        "model": config.MODELO,
        "messages": [{"role": "system", "content": config.INSTRUCCIONES + extra}] + historial,
        "stream": True,
        "keep_alive": "30m",        # deja el modelo cargado entre preguntas
    }
    pedido = urllib.request.Request(config.OLLAMA_URL + "/api/chat",
                                    data=json.dumps(cuerpo).encode(),
                                    headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(pedido, timeout=120) as r:
        for linea in r:
            if not linea.strip():
                continue
            dato = json.loads(linea)
            if "error" in dato:
                raise RuntimeError(dato["error"])
            yield dato.get("message", {}).get("content", "")
            if dato.get("done"):
                return


# ---- hablar -----------------------------------------------------------------


class Locutor(threading.Thread):
    # Convierte las frases en audio y las pone en la cola del reproductor
    def __init__(self, voz, reproductor):
        super().__init__(daemon=True)
        self.voz = voz
        self.reproductor = reproductor
        self.frases = queue.Queue()

    def run(self):
        while True:
            frase = self.frases.get()
            try:
                self.reproductor.agregar(self.voz.sintetizar(frase), self.voz.sr)
            except Exception as e:
                print("[voz] %s" % e)
            finally:
                self.frases.task_done()


# ---- programa principal -----------------------------------------------------


def ejecutar(accion, motor):
    if accion.tipo == "parar":
        motor.detener()
        print("      [cubo] animacion detenida")
    else:
        motor.iniciar(accion.nombre, accion.mensaje)
        print("      [cubo] %s%s" % (accion.nombre,
                                   " '%s'" % accion.mensaje.strip() if accion.mensaje else ""))


def entender(whisper, audio):
    t = time.monotonic()
    segmentos, _ = whisper.transcribe(audio, language=config.IDIOMA, beam_size=1,
                                      vad_filter=True)
    texto = " ".join(s.text.strip() for s in segmentos).strip()
    if len(texto) >= 2:
        print("\nTu:   %s  (%.1f s)" % (texto, time.monotonic() - t))
    return texto


def responder(texto, historial, motor, locutor):
    # Comando directo o pregunta al modelo. Devuelve la respuesta, con sus
    # etiquetas, para guardarla en el historial.
    accion = comandos.interpretar(texto) if getattr(config, "COMANDOS_DIRECTOS", True) else None
    if accion is not None:
        print("Cubo: %s  (comando directo)" % accion.respuesta)
        ejecutar(accion, motor)
        locutor.frases.put(accion.respuesta)
        return accion.respuesta + " " + accion.etiqueta()

    t = time.monotonic()
    frases = Frases()
    respuesta = ""
    primera = None
    extra = comandos.instrucciones_cubo(motor.actual, motor.mensaje)
    try:
        for pedazo in preguntar(historial + [{"role": "user", "content": texto}], extra):
            respuesta += pedazo
            for frase in frases.agregar(pedazo):
                primera = primera or time.monotonic()
                locutor.frases.put(frase)
    except Exception as e:
        print("[ollama] %s" % e)
        if not respuesta:
            respuesta = "Perdón, tuve un problema para pensar la respuesta."
            frases.agregar(respuesta)
    for frase in frases.terminar():
        primera = primera or time.monotonic()
        locutor.frases.put(frase)
    print("Cubo: %s  (1a frase en %.1f s)" % (respuesta.strip(), (primera or time.monotonic()) - t))
    for tipo, valor in frases.etiquetas:
        accion = comandos.de_etiqueta(tipo, valor)
        if accion is not None:
            ejecutar(accion, motor)
        else:
            print("      [cubo] etiqueta ignorada: %s %s" % (tipo, valor or ""))
    return respuesta.strip()


def principal():
    p = argparse.ArgumentParser(description="Asistente de voz con el cubo LED")
    p.add_argument("--escribir", action="store_true",
                   help="escribir las preguntas en vez de hablar (sin microfono ni Whisper)")
    p.add_argument("--sin-cubo", action="store_true", help="probar sin la Pico conectada")
    args = p.parse_args()

    print("Asistente de voz v%s. Cargando..." % VERSION)
    whisper = detector = None
    if not args.escribir:
        from faster_whisper import WhisperModel
        whisper = WhisperModel(config.WHISPER_MODELO, device="cpu", compute_type="int8")
        if config.ACTIVACION == "palabra":
            import openwakeword
            from openwakeword.model import Model
            openwakeword.utils.download_models()
            detector = Model(wakeword_models=[config.PALABRA_ACTIVACION],
                             inference_framework="onnx")
    voz = Voz(config.VOZ_PIPER)
    motor = MotorAnimaciones()

    if args.sin_cubo:
        cubo = abrir_cubo("silencio" if args.escribir else "consola")
    else:
        cubo = abrir_cubo(config.PUERTO_CUBO)
    rep = Reproductor(config.PARLANTE, config.AJUSTE_SINCRONIA_S)
    mic = None if args.escribir else Microfono(config.MICROFONO)
    anim = Animador(cubo, rep, mic, modo=config.ANIMACION_HABLANDO,
                    fps=config.CUADROS_POR_SEGUNDO, motor=motor,
                    voz_sobre_animacion=getattr(config, "VOZ_SOBRE_ANIMACION", True))
    locutor = Locutor(voz, rep)
    anim.start()
    locutor.start()

    anim.estado = "pensando"            # precargar el modelo para que la 1a respuesta no tarde
    try:
        for _ in preguntar([{"role": "user", "content": "Hola"}]):
            pass
    except Exception as e:
        print("No pude hablar con Ollama en %s: %s" % (config.OLLAMA_URL, e))
        raise SystemExit(1)

    historial = []
    if args.escribir:
        print("Listo. Escribe tu pregunta o un comando (por ejemplo: pon la lluvia).")
    elif detector:
        print("Listo. Di '%s' y luego tu pregunta." % config.PALABRA_ACTIVACION)
    else:
        print("Listo. Habla cuando quieras (por ejemplo: pon la lluvia).")
    try:
        while True:
            anim.estado = "reposo"
            if args.escribir:
                texto = input("\nTu:   ").strip()
            else:
                if detector:
                    esperar_palabra(mic, detector)
                    anim.estado = "escuchando"
                    audio = escuchar_pregunta(mic, anim, espera_max_s=6)
                else:
                    audio = escuchar_pregunta(mic, anim)
                if audio is None:
                    continue
                anim.estado = "pensando"
                texto = entender(whisper, audio)
            if len(texto) < 2:
                continue

            anim.estado = "pensando"
            respuesta = responder(texto, historial, motor, locutor)
            historial += [{"role": "user", "content": texto},
                          {"role": "assistant", "content": respuesta}]
            historial = historial[-config.MAX_HISTORIAL:]

            locutor.frases.join()       # espera a que todas las frases tengan audio
            rep.esperar()
            if mic is not None:
                time.sleep(0.3)         # deja apagar el eco de la sala
                mic.vaciar()
    except (KeyboardInterrupt, EOFError):
        print()
    finally:
        motor.detener()
        anim.detener()
        rep.cerrar()
        if mic is not None:
            mic.cerrar()
        cubo.cerrar()


if __name__ == "__main__":
    principal()
