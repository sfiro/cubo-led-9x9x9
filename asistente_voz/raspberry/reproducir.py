# Fase 1: reproduce un WAV y el cubo se ilumina al ritmo del sonido.
# Funciona tambien en la Mac (con el cubo por USB o con --sin-cubo).
#
#   python reproducir.py voz.wav
#   python reproducir.py musica.wav --modo espectro
#   python reproducir.py voz.wav --sin-cubo

import argparse
import time

try:
    import config
except ImportError:
    import config_ejemplo as config
from audio import Reproductor, leer_wav
from cubo_usb import abrir_cubo
from visual import MODOS_HABLANDO, Animador

p = argparse.ArgumentParser(description="Reproduce un WAV con el cubo LED")
p.add_argument("archivo", help="WAV de 16 bits")
p.add_argument("--modo", choices=list(MODOS_HABLANDO), default=config.ANIMACION_HABLANDO)
p.add_argument("--sin-cubo", action="store_true", help="probar sin la Pico conectada")
args = p.parse_args()

muestras, sr = leer_wav(args.archivo)
cubo = abrir_cubo("consola" if args.sin_cubo else config.PUERTO_CUBO)
rep = Reproductor(config.PARLANTE, config.AJUSTE_SINCRONIA_S)
anim = Animador(cubo, rep, modo=args.modo, fps=config.CUADROS_POR_SEGUNDO)
anim.start()
print("Cubo en %s | reproduciendo %s (%.1f s) con la animacion '%s'"
      % (cubo.puerto, args.archivo, len(muestras) / sr, args.modo))
try:
    time.sleep(0.5)
    rep.agregar(muestras, sr)
    rep.esperar()
    time.sleep(1)
except KeyboardInterrupt:
    pass
finally:
    anim.detener()
    rep.cerrar()
    cubo.cerrar()
