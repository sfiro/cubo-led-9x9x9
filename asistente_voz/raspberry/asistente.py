# Fase 3: asistente de voz completo, todo local en la Raspberry Pi 5.
#
#   microfono -> faster-whisper -> Ollama -> Piper -> parlante
#                                              \-> cubo (al ritmo de la voz)
#
# v2: ademas se pueden elegir las animaciones con la voz ("pon la lluvia",
# "para", "escribe HOLA"), sin dejar de conversar con el modelo. Ver
# ../versiones/v2.md.
# v3: niveles de brillo en el cubo (bordes suaves, estela en las animaciones)
# y brillo por voz ("baja el brillo"). Ver ../versiones/v3.md.
# v4: se despierta al oir "hola cubo" y queda atento un rato. Ver ../versiones/v4.md.
# v4.1: en el modo demo dice el nombre de cada animacion antes de mostrarla.
# v5: responde con la informacion de ../conocimiento/ (la tienda y Cali): busca
# los fragmentos que sirven para cada pregunta (RAG) y contesta al instante las
# preguntas frecuentes. Ver ../versiones/v5.md.
# v5.1: busca en la base de productos (productos.xlsx) los que nombra el cliente,
# con su precio real. Ver ../versiones/v5.1.md.
#
# Un turno a la vez: mientras el cubo habla, el microfono se ignora para que
# no se escuche a si mismo. Ctrl-C para salir.
#
#   python asistente.py               hablando por el microfono
#   python asistente.py --escribir    escribiendo las preguntas (para probar)
#   python asistente.py --sin-cubo    sin la Pico conectada

import argparse
import json
import os
import random
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
import conocimiento
import productos
from activacion import Vigilia, buscar_frase, es_despedida
from animaciones_pi import MotorAnimaciones
from audio import Microfono, Reproductor, remuestrear
from cubo_usb import abrir_cubo
from visual import Animador
from voz import Frases, Voz, limpiar

VERSION = "5.2"
# v5: los fija principal(). INSTRUCCIONES reemplaza a config.INSTRUCCIONES si hay
# personalidad.md, y SABER busca en los documentos.
INSTRUCCIONES = None
SABER = None
CATALOGO = None             # v5.1: productos.Catalogo
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


def escuchar_pregunta(mic, anim, espera_max_s=None, mostrar=True, rep=None):
    # Espera a que alguien hable y graba hasta un silencio. Devuelve el audio a
    # 16 kHz (o None si no hubo voz, o fue solo un ruido corto) y el momento en
    # que empezo la voz. mostrar=False: el cubo no reacciona (asistente dormido).
    # rep (v4.1): mientras el parlante suena (por ejemplo, el nombre de una
    # animacion del modo demo) lo que entra se descarta, para no escucharse a si mismo.
    mic.vaciar()
    bloque_s = 0.03
    previo = []                     # 0.3 s antes de detectar la voz, para no cortar el inicio
    grabado = []
    silencio = 0.0
    esperado = 0.0
    while True:
        x = mic.cola.get()
        if rep is not None and rep.ocupado():
            previo, grabado, silencio = [], [], 0.0
            if mostrar and anim.estado == "escuchando":
                anim.estado = "reposo"
            continue
        hay_voz = float(np.sqrt(np.mean(x * x))) > config.UMBRAL_VOZ
        if not grabado:
            previo = (previo + [x])[-10:]
            esperado += bloque_s
            if hay_voz:
                grabado = previo
                inicio = time.monotonic()
                if mostrar:
                    anim.estado = "escuchando"
            elif espera_max_s and esperado > espera_max_s:
                return None, None
            continue
        grabado.append(x)
        silencio = 0.0 if hay_voz else silencio + len(x) / mic.sr
        duracion = sum(len(g) for g in grabado) / mic.sr
        if silencio >= config.SILENCIO_FIN_S or duracion >= config.MAX_PREGUNTA_S:
            break
    if duracion - silencio < 0.4:
        return None, inicio
    return remuestrear(np.concatenate(grabado), mic.sr, SR_WHISPER), inicio


# ---- pensar -----------------------------------------------------------------


def preguntar(historial, extra=""):
    # Pide la respuesta a Ollama y la entrega en pedazos a medida que se genera.
    # extra: lo que se agrega a las instrucciones (la lista de animaciones)
    cuerpo = {
        "model": config.MODELO,
        "messages": [{"role": "system", "content": (INSTRUCCIONES or config.INSTRUCCIONES) + extra}]
                    + historial,
        "stream": True,
        "keep_alive": "30m",        # deja el modelo cargado entre preguntas
        # Ollama corta sin avisar lo que no cabe en la ventana, empezando por las
        # instrucciones: con los documentos se fija en 8192 tokens
        "options": {"num_ctx": getattr(config, "NUM_CTX", 8192)},
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


def ejecutar(accion, motor, anim):
    if accion.tipo == "parar":
        motor.detener()
        print("      [cubo] animacion detenida")
    elif accion.tipo == "brillo":
        anim.brillo = comandos.nuevo_brillo(anim.brillo, accion.valor)
        print("      [cubo] brillo al %d %%" % comandos.porcentaje(anim.brillo))
        if getattr(anim.cubo, "bits", 1) == 1:
            print("      AVISO: la Pico no tiene niveles de brillo. Carga el firmware nuevo "
                  "con ./cargar_pico.sh")
    else:
        motor.iniciar(accion.nombre, accion.mensaje)
        print("      [cubo] %s%s" % (accion.nombre,
                                   " '%s'" % accion.mensaje.strip() if accion.mensaje else ""))


def entender(whisper, audio, guia=None):
    # Devuelve (texto, segundos que tardo). guia (v4): texto que Whisper toma como
    # contexto previo, para que escriba bien la frase de activacion.
    t = time.monotonic()
    segmentos, _ = whisper.transcribe(audio, language=config.IDIOMA, beam_size=1,
                                      vad_filter=True, initial_prompt=guia)
    return " ".join(s.text.strip() for s in segmentos).strip(), time.monotonic() - t


def responder(texto, historial, motor, locutor, anim):
    # Comando directo o pregunta al modelo. Devuelve la respuesta, con sus
    # etiquetas, para guardarla en el historial.
    accion = comandos.interpretar(texto) if getattr(config, "COMANDOS_DIRECTOS", True) else None
    if accion is not None:
        print("Cubo: %s  (comando directo)" % accion.respuesta)
        ejecutar(accion, motor, anim)
        locutor.frases.put(accion.respuesta)
        return accion.respuesta + " " + accion.etiqueta()

    mensaje = texto
    if SABER is not None:
        try:
            exacta = SABER.pregunta_exacta(texto)
            if exacta is not None:
                respuesta, forma, puntaje = exacta
                print("Cubo: %s  (pregunta frecuente: '%s' %.2f)" % (respuesta, forma, puntaje))
                locutor.frases.put(limpiar(respuesta))
                return respuesta
            contexto, fuentes = SABER.contexto(texto)
        except Exception as e:          # Ollama ocupado o caido: se responde sin documentos
            print("[conocimiento] %s" % e)
            contexto, fuentes = "", []
        print("      [docs] %s" % (", ".join(fuentes) or "nada que sirva"))
        if CATALOGO is not None:
            encontrados, resumen = CATALOGO.buscar(texto)
            if encontrados:
                print("      [productos] %s" % resumen)
                contexto = (encontrados + "\n" + contexto).strip()
        if contexto:
            # Los documentos van con la pregunta y no en las instrucciones: asi las
            # instrucciones no cambian y Ollama reusa lo que ya leyo de ellas
            mensaje = "Información que puede servir:\n%s\n\nPregunta: %s" % (contexto, texto)

    t = time.monotonic()
    frases = Frases()
    respuesta = ""
    primera = None
    extra = comandos.instrucciones_cubo(motor.actual, motor.mensaje, anim.brillo)
    try:
        for pedazo in preguntar(historial + [{"role": "user", "content": mensaje}], extra):
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
            ejecutar(accion, motor, anim)
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

    global INSTRUCCIONES, SABER, CATALOGO
    carpeta = conocimiento.carpeta_conocimiento(getattr(config, "CARPETA_CONOCIMIENTO", None))
    if getattr(config, "USAR_CONOCIMIENTO", True) and carpeta:
        # v5.1: si hay una base de productos nueva, se importa antes de leer los documentos,
        # porque genera catalogo.md
        try:
            mensaje = productos.preparar(carpeta)
            if mensaje:
                print(mensaje)
            if os.path.exists(os.path.join(carpeta, "productos.json")):
                CATALOGO = productos.Catalogo(carpeta)
                print("Productos: %d en la base" % len(CATALOGO.productos))
        except Exception as e:
            print("AVISO: no pude leer la base de productos: %s" % e)
        INSTRUCCIONES = (conocimiento.cargar_personalidad(carpeta) or config.INSTRUCCIONES) \
            + conocimiento.REGLAS
        SABER = conocimiento.Conocimiento(
            carpeta, config.OLLAMA_URL, getattr(config, "MODELO_EMBEDDINGS", "bge-m3"),
            getattr(config, "FRAGMENTOS", 3), getattr(config, "UMBRAL_FRAGMENTO", 0.35),
            getattr(config, "UMBRAL_PREGUNTA_EXACTA", 0.88))
        print("Conocimiento: %s, %d secciones, %d preguntas frecuentes%s" % (
            os.path.relpath(carpeta, conocimiento.AQUI), len(SABER.fragmentos),
            len(SABER.preguntas), ", %d parrafos sin completar" % SABER.pendientes
            if SABER.pendientes else ""))
        if SABER.aviso:
            print("AVISO: " + SABER.aviso)

    if args.sin_cubo:
        cubo = abrir_cubo("silencio" if args.escribir else "consola")
    else:
        cubo = abrir_cubo(config.PUERTO_CUBO, getattr(config, "GAMMA_BRILLO", 2.2))
        print("Pico en %s, con %s" % (cubo.puerto, "16 niveles de brillo" if cubo.bits == 4 else
                                      "%d niveles de brillo" % (1 << cubo.bits) if cubo.bits > 1 else
                                      "encendido/apagado (firmware viejo: corre ./cargar_pico.sh)"))
    rep = Reproductor(config.PARLANTE, config.AJUSTE_SINCRONIA_S)
    mic = None if args.escribir else Microfono(config.MICROFONO)
    anim = Animador(cubo, rep, mic, modo=config.ANIMACION_HABLANDO,
                    fps=config.CUADROS_POR_SEGUNDO, motor=motor,
                    voz_sobre_animacion=getattr(config, "VOZ_SOBRE_ANIMACION", True),
                    brillo=getattr(config, "BRILLO", 1.0), estela_ms=getattr(config, "ESTELA_MS", 150))
    locutor = Locutor(voz, rep)

    turno = threading.Lock()        # para que un anuncio no se meta en medio de una respuesta

    def anunciar(nombre):
        # Modo demo: dice el nombre de la animacion y espera a que termine de sonar
        texto = "Ahora, %s." % comandos.nombre_hablado(nombre)
        with turno:
            print("      [demo] %s" % texto)
            locutor.frases.put(texto)
            locutor.frases.join()
            rep.esperar()

    if getattr(config, "ANUNCIAR_DEMO", True):
        motor.al_empezar = anunciar
    anim.start()
    locutor.start()

    anim.estado = "pensando"            # precargar el modelo para que la 1a respuesta no tarde
    try:
        for _ in preguntar([{"role": "user", "content": "Hola"}]):
            pass
    except Exception as e:
        print("No pude hablar con Ollama en %s: %s" % (config.OLLAMA_URL, e))
        raise SystemExit(1)

    # v4: con ACTIVACION = "frase" duerme hasta oir FRASE_ACTIVACION ("hola cubo")
    con_frase = config.ACTIVACION == "frase"
    frase = getattr(config, "FRASE_ACTIVACION", "hola cubo")
    guia = frase.title() + "."                      # "Hola Cubo."
    pista = getattr(config, "PISTA_WHISPER", None)  # v5: palabras del negocio para Whisper
    vigilia = Vigilia(getattr(config, "VENTANA_CONVERSACION_S", 20))
    aviso = getattr(config, "RESPUESTA_ACTIVACION", "¿Sí?")
    audio_aviso = voz.sintetizar(aviso) if con_frase and aviso else None

    historial = []
    if con_frase:
        print('Listo. Di "%s" para despertarlo%s.' % (frase, " (o escríbelo)" if args.escribir else ""))
    elif args.escribir:
        print("Listo. Escribe tu pregunta o un comando (por ejemplo: pon la lluvia).")
    elif detector:
        print("Listo. Di '%s' y luego tu pregunta." % config.PALABRA_ACTIVACION)
    else:
        print("Listo. Habla cuando quieras (por ejemplo: pon la lluvia).")
    try:
        while True:
            atento = not con_frase or vigilia.atento()
            anim.atento = atento
            anim.estado = "reposo"
            if args.escribir:
                texto = input("\nTu:   ").strip()
            else:
                if detector:
                    esperar_palabra(mic, detector)
                    anim.estado = "escuchando"
                    audio, inicio = escuchar_pregunta(mic, anim, espera_max_s=6, rep=rep)
                else:
                    audio, inicio = escuchar_pregunta(mic, anim, mostrar=atento, rep=rep)
                if audio is None:
                    continue
                # Cuenta si estaba atento cuando EMPEZO a hablar, aunque la ventana
                # se haya cerrado mientras hablaba
                atento = not con_frase or vigilia.atento(inicio)
                if atento:
                    anim.estado = "pensando"
                texto, segundos = entender(whisper, audio, pista if atento else guia)
            if len(texto) < 2:
                continue

            if not atento:
                encontrada, resto = buscar_frase(texto, frase)
                if not encontrada:
                    print("      [dormido] %s" % texto)
                    continue
                vigilia.despertar()
                anim.atento = True
                if not args.escribir:
                    print("\nTu:   %s  (%.1f s)" % (texto, segundos))
                print("      [cubo] despierto: te escucho %d s" % vigilia.ventana_s)
                if len(resto) < 2:              # solo "hola cubo": contesta y espera la pregunta
                    if audio_aviso is not None:
                        print("Cubo: %s" % aviso)
                        rep.agregar(audio_aviso, voz.sr)
                        rep.esperar()
                    if mic is not None:
                        time.sleep(0.2)
                        mic.vaciar()
                    vigilia.despertar()
                    continue
                texto = resto                   # "hola cubo, pon la lluvia"
            elif not args.escribir:
                print("\nTu:   %s  (%.1f s)" % (texto, segundos))

            if con_frase and es_despedida(texto):
                adios = random.choice(["¡Chao!", "Hasta luego.", "Aquí estaré."])
                print("Cubo: %s  (se duerme)" % adios)
                locutor.frases.put(adios)
                locutor.frases.join()
                rep.esperar()
                vigilia.dormir()
                continue

            anim.estado = "pensando"
            with turno:
                respuesta = responder(texto, historial, motor, locutor, anim)
                locutor.frases.join()   # espera a que todas las frases tengan audio
                rep.esperar()
            historial += [{"role": "user", "content": texto},
                          {"role": "assistant", "content": respuesta}]
            historial = historial[-config.MAX_HISTORIAL:]

            if mic is not None:
                time.sleep(0.3)         # deja apagar el eco de la sala
                mic.vaciar()
            vigilia.despertar()         # la ventana cuenta desde que termino de hablar
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
