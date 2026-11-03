# Instalar con OpenClaw

Cómo pedirle a OpenClaw, instalado en la Raspberry Pi 5, que instale y
pruebe el asistente. El agente hace los mismos pasos del [README](README.md#3-instalar-la-raspberry-pi-5),
pero con tres scripts que lo dejan casi todo automático:

| Script | Qué hace |
|---|---|
| `raspberry/instalar.sh` | Paquetes del sistema, entorno de Python, `config.py`, voz de Piper, Ollama con el modelo y permisos de USB y audio |
| `raspberry/cargar_pico.sh` | Guarda un respaldo de lo que tiene la Pico y le carga el firmware del asistente |
| `raspberry/diagnostico.py` | Prueba cada parte por separado (Pico, audio, Piper, Whisper, Ollama) sin que nadie hable, y termina con el cubo hablando |

## Antes de empezar

1. Conecta a la Pi 5 por USB:
   - el micrófono;
   - el parlante;
   - la Pico, que ya está conectada al cubo. El cubo debe tener su fuente de 5 V encendida.
2. Cierra Thonny en cualquier computador que tenga abierta la Pico.
3. La Pico perderá el programa de la secuencia: `cargar_pico.sh` lo guarda en
   `raspberry/respaldo_pico/`, y `./cargar_pico.sh --restaurar` lo devuelve.
4. El agente necesita poder usar `sudo` para instalar paquetes y Ollama. Si no puede, corre tú
   `./instalar.sh` por SSH y luego dile que siga desde el paso 3.
5. La instalación descarga unos 3 GB (el modelo de lenguaje, Whisper y la voz) y tarda de 10 a
   20 minutos.

## Mensaje para OpenClaw

Copia y pega esto en el chat de OpenClaw:

```text
Quiero que instales y pruebes un asistente de voz local que usa un cubo LED como pantalla, en esta Raspberry Pi 5. Todo está en el repositorio público https://github.com/sfiro/cubo-led-9x9x9, en la carpeta asistente_voz/. Lee primero asistente_voz/README.md y asistente_voz/model.md.

Hardware conectado por USB a esta Pi: un micrófono, un parlante y una Raspberry Pi Pico que maneja el cubo.

Reglas:
- Trabaja solo dentro de ~/cubo-led-9x9x9. Lo único de afuera son los paquetes y permisos que instala instalar.sh.
- No hagas commits ni push, y no modifiques los archivos del repositorio. Lo único que puedes editar es asistente_voz/raspberry/config.py, que es local.
- No instales todavía el servicio de arranque automático (asistente.service).
- Si un paso falla, no improvises arreglos grandes: muéstrame el error completo y lo que propones hacer.
- Pregúntame antes de reiniciar la Pi.

Pasos:
1. Si ~/cubo-led-9x9x9 no existe, clónalo con git clone https://github.com/sfiro/cubo-led-9x9x9.git ~/cubo-led-9x9x9. Si ya existe, actualízalo con git pull.
2. cd ~/cubo-led-9x9x9/asistente_voz/raspberry y corre ./instalar.sh. Si al final avisa que agregó el usuario a los grupos dialout y audio, pregúntame si reinicio la Pi antes de seguir.
3. Identifica el micrófono y el parlante USB con: .venv/bin/python -m sounddevice, aplay -l y arecord -l. Si la Pi tiene PipeWire (versión con escritorio), sigue la sección "Audio con PipeWire" del README: deja MICROFONO y PARLANTE en None y revisa los predeterminados con wpctl status. Si no son el micrófono y el parlante USB, muéstrame la salida de wpctl status antes de cambiarlos, porque otro programa podría depender de ellos. Si la Pi no tiene PipeWire y no son los predeterminados, pon en config.py MICROFONO y PARLANTE con parte de su nombre o su número.
4. Prueba el parlante sin el cubo: .venv/bin/python decir.py --sin-cubo "Hola, estoy probando el parlante". Pregúntame si lo escuché.
5. Revisa que la Pico aparezca con ls /dev/ttyACM*. Luego corre ./cargar_pico.sh, que guarda un respaldo y carga el firmware. Pregúntame si el cubo muestra una esfera que crece y se encoge.
6. Corre .venv/bin/python diagnostico.py y muéstrame el resumen completo. Durante la prueba, el cubo enciende piso por piso y al final el parlante dice una frase mientras el cubo se ilumina. Si el diagnóstico avisa que el ruido supera UMBRAL_VOZ, pon en config.py UMBRAL_VOZ en unas 3 veces el promedio de ruido que midió (mínimo 0.01) y vuelve a correrlo.
7. Si todo sale OK, lanza el asistente en segundo plano: nohup .venv/bin/python -u asistente.py > asistente.log 2>&1 &. Espera a que asistente.log diga "Listo", avísame para que le hable, y muéstrame las líneas "Tu:" y "Cubo:" del log. Para detenerlo: pkill -f asistente.py.
8. Al final dame un resumen: qué instalaste, qué micrófono y parlante elegiste, qué cambiaste en config.py y los tiempos que imprimieron diagnostico.py y asistente.py.
```

## Mensaje para actualizar a una versión nueva

Cuando haya una versión nueva en GitHub (ver [versiones/](versiones/README.md)), pega esto en
OpenClaw:

```text
Hay una versión nueva del asistente de voz del cubo LED. En ~/cubo-led-9x9x9:
1. Detén el asistente si está corriendo (pkill -f asistente.py) y actualiza con git pull.
2. Lee asistente_voz/versiones/README.md y el documento de la versión nueva (por ejemplo, asistente_voz/versiones/v2.md), y dime en dos líneas qué trae.
3. Si ese documento dice que hay que reinstalar algo o volver a cargar la Pico, hazlo con instalar.sh o cargar_pico.sh.
4. cd asistente_voz/raspberry y corre .venv/bin/python diagnostico.py. Muéstrame el resumen completo.
5. Si todo sale OK, lanza el asistente en segundo plano como antes: nohup .venv/bin/python -u asistente.py > asistente.log 2>&1 &. Avísame para que lo pruebe, y muéstrame las líneas "Tu:", "Cubo:" y "[cubo]" del log.
Las mismas reglas de antes: no modifiques el repositorio, solo config.py, y pregúntame antes de reiniciar.
```

## Si algo falla

- El agente debería mostrarte el error. La tabla
  [Problemas comunes](README.md#problemas-comunes) del README cubre los más típicos.
- Para volver a las animaciones solas en la Pico:
  `cd ~/cubo-led-9x9x9/asistente_voz/raspberry && ./cargar_pico.sh --restaurar`.
- Para detener el asistente: `pkill -f asistente.py`. Después de 3 s, la Pico vuelve a mostrar
  la esfera de espera.
