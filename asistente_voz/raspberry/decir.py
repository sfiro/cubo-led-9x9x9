# Fase 2: el cubo "habla" con Piper lo que escribas.
#
#   python decir.py "Hola, soy el cubo"
#   python decir.py              (modo interactivo: una frase por linea)

import argparse
import time

try:
    import config
except ImportError:
    import config_ejemplo as config
from audio import Reproductor
from cubo_usb import abrir_cubo
from visual import MODOS_HABLANDO, Animador
from voz import Voz

p = argparse.ArgumentParser(description="El cubo dice un texto con Piper")
p.add_argument("texto", nargs="*")
p.add_argument("--modo", choices=list(MODOS_HABLANDO), default=config.ANIMACION_HABLANDO)
p.add_argument("--sin-cubo", action="store_true", help="probar sin la Pico conectada")
args = p.parse_args()

voz = Voz(config.VOZ_PIPER)
cubo = abrir_cubo("consola" if args.sin_cubo else config.PUERTO_CUBO)
rep = Reproductor(config.PARLANTE, config.AJUSTE_SINCRONIA_S)
anim = Animador(cubo, rep, modo=args.modo, fps=config.CUADROS_POR_SEGUNDO)
anim.start()


def decir(texto):
    anim.estado = "pensando"
    t = time.monotonic()
    audio = voz.sintetizar(texto)
    print("(voz lista en %.2f s)" % (time.monotonic() - t))
    anim.estado = "reposo"
    rep.agregar(audio, voz.sr)
    rep.esperar()


try:
    if args.texto:
        decir(" ".join(args.texto))
        time.sleep(0.5)
    else:
        print("Escribe una frase y Enter (Ctrl-C para salir)")
        while True:
            texto = input("> ").strip()
            if texto:
                decir(texto)
except (KeyboardInterrupt, EOFError):
    pass
finally:
    anim.detener()
    rep.cerrar()
    cubo.cerrar()
