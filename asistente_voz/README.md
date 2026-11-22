# Asistente de voz con el cubo LED

Un asistente de voz que funciona **completamente local** en una Raspberry Pi 5, sin nube.
El cubo de 9×9×9 es su cara: se ilumina al ritmo de la voz cuando el asistente habla, y
muestra otra animación mientras escucha y mientras piensa. Desde la v2 también se le pueden
pedir con la voz las 34 animaciones del cubo ("pon la lluvia", "¿qué hora es?", "para").

Desde la v3 el cubo tiene 16 niveles de brillo: bordes suaves, estela en las animaciones y
brillo por voz ("baja el brillo"). Desde la v4 duerme hasta que le dices **"hola cubo"**.

**Versión actual: 5.10**: el cubo de Cali Abstracto. Empieza una conversación nueva con cada cliente, le pasa al modelo solo lo que necesita y sigue el tema con preguntas como "¿y cuánto valen?" o "¿y de qué material son?" ([v5.10](versiones/v5.10.md)). Responde sobre la tienda de artesanías y sobre Cali con documentos que tú escribes (ver [versiones/v5.md](versiones/v5.md)). Lo que trae cada versión y cómo volver a una anterior está en
[versiones/](versiones/README.md).

```
 Micrófono ─► Raspberry Pi 5 ──────────────────────────────────► Parlante
              voz a texto → modelo de lenguaje → texto a voz
                                                      │ análisis del audio
                                                      ▼
                                  USB ─► Pico (PIO + DMA) ─► Cubo 9×9×9
```

La Pi piensa y la Pico solo refresca el cubo, con el mismo driver de [`pico_w/`](../pico_w/).
Así el cubo nunca parpadea, aunque la Pi esté ocupada con el modelo. Los detalles técnicos
están en [model.md](model.md).

| Estado | Qué muestra el cubo |
|---|---|
| En reposo | Una esfera hueca que "respira" |
| Escuchando | Un cilindro que crece con el volumen de tu voz |
| Pensando | Una doble hélice girando |
| Hablando | Ondas que salen del centro con cada sílaba (o una esfera, o un ecualizador) |
| Sin la Pi | La Pico muestra sola una esfera que respira |
| Con una animación elegida (v2) | La animación, en vez de la esfera de reposo |

## Qué se necesita

| Parte | Comentario |
|---|---|
| Raspberry Pi 5, **8 GB** o más | Con 4 GB solo caben los modelos más chicos |
| Disipador con ventilador (Active Cooler) | Sin él, la Pi baja su velocidad al calentarse con el modelo |
| Fuente oficial USB-C de 27 W | Aparte de la fuente del cubo |
| microSD de 32 GB o más (o un disco NVMe) | Los modelos ocupan unos 2 a 4 GB |
| Micrófono y parlante USB | Lo ideal es un **altavoz de conferencias USB** (micrófono + parlante con cancelación de eco). La Pi 5 no tiene salida de audio de 3.5 mm |
| Raspberry Pi Pico (o Pico W) | La misma que ya maneja el cubo |
| El cubo, con su fuente de 5 V y 2 A o más | Sin cambios |

## Conexión

```
Raspberry Pi 5 ──USB──► Pico ──GP2/GP3/GP4 + GND──► conector JP7 del cubo
      │
      └──USB──► micrófono y parlante
```

- La Pico se conecta a la Pi con un cable USB normal, que la alimenta y lleva los datos.
- De la Pico al cubo, igual que en [pico_w/LEEME.md](../pico_w/LEEME.md#conexión):
  GP2 → DATA (pin 14 de U1), GP3 → CLOCK (pin 11), GP4 → LATCH (pin 12) y **GND con GND**.
- No conectes la fuente del cubo a la Pi ni a la Pico: solo se comparte el GND.

## Archivos

| Archivo | Qué hace |
|---|---|
| `pico/main.py` | Firmware de la Pico: recibe los cuadros por USB y los muestra |
| `raspberry/cubo_usb.py` | Encuentra la Pico y le envía los cuadros |
| `raspberry/audio.py` | Analiza el volumen y las frecuencias, reproduce el audio sincronizado con el cubo y captura el micrófono |
| `raspberry/visual.py` | Las animaciones de cada estado y el hilo que manda 40 cuadros por segundo |
| `raspberry/voz.py` | Texto a voz con Piper, y la limpieza y división en frases del texto del modelo |
| `raspberry/asistente.py` | El asistente completo (fase 3). `--escribir` para probarlo escribiendo |
| `raspberry/animaciones_pi.py` | Corre en la Pi las 34 animaciones de `pico_w/animaciones.py` (v2) |
| `raspberry/comandos.py` | Comandos de voz para las animaciones y lo que el modelo sabe de ellas (v2) |
| `raspberry/activacion.py` | Reconoce "hola cubo" y lleva la cuenta de si está atento o dormido (v4) |
| `raspberry/productos.py` | Importa la base de productos (`productos.xlsx`) sin datos internos, y busca los productos que nombra un cliente (v5.1) |
| `raspberry/conocimiento.py` | Busca en los documentos lo que sirve para cada pregunta, y responde las preguntas frecuentes (v5) |
| `conocimiento/` | Lo que sabe el cubo de **Cali Abstracto**: la tienda, la guía de Cali, las preguntas frecuentes y la personalidad (v5.2). La base de productos no se sube |
| `conocimiento_ejemplo/` | Las mismas plantillas, vacías, para otra tienda (v5) |
| `raspberry/reproducir.py` | Fase 1: el cubo sigue un archivo WAV |
| `raspberry/decir.py` | Fase 2: el cubo dice con voz lo que escribas |
| `raspberry/prueba_usb.py` | Prueba la conexión: piso por piso y velocidad |
| `raspberry/medir_microfono.py` | Muestra el volumen del micrófono para ajustar `UMBRAL_VOZ` |
| `raspberry/config_ejemplo.py` | Plantilla de configuración. Cópiala como `config.py`, que queda fuera del repo |
| `raspberry/asistente.service` | Servicio para que arranque solo al encender la Pi (Raspberry Pi OS Lite) |
| `raspberry/asistente-usuario.service` | Lo mismo, para la Pi con escritorio (PipeWire) |
| `raspberry/instalar.sh` | Instala todo en la Pi 5 (paquetes, Python, voz, Ollama y modelo, permisos) |
| `raspberry/cargar_pico.sh` | Respalda lo que tiene la Pico y le carga el firmware (`--restaurar` lo devuelve) |
| `raspberry/diagnostico.py` | Prueba cada parte por separado sin que nadie hable, y termina con el cubo hablando |
| `versiones/` | Un documento por versión: qué cambió, cómo se usa y cómo se probó |
| `OPENCLAW.md` | Mensaje para que el agente OpenClaw de la Pi lo instale y lo pruebe |

## 1. Preparar la Pico

1. Instala MicroPython en la Pico, si no lo tiene: la misma versión de
   [pico_w/LEEME.md](../pico_w/LEEME.md#instalación) sirve.
2. Con Thonny, copia a la Pico estos dos archivos:
   - [`../pico_w/cubo.py`](../pico_w/cubo.py) (el driver);
   - [`pico/main.py`](pico/main.py) (reemplaza al `main.py` de las animaciones).
3. Si cambiaste los pines, copia también tu `config.py`: este `main.py` lee de ahí `PIN_DATOS`,
   `PIN_RELOJ`, `PIN_LATCH` y `FREQ_PIO`. Sin él usa GP2, GP3 y GP4.
4. Desconecta y vuelve a conectar el USB. A los 3 segundos el cubo debe mostrar la esfera que
   respira: eso indica que la Pico está esperando cuadros.
5. **Cierra Thonny** antes de usar la Pi o la Mac: si Thonny tiene el puerto abierto, nadie más
   puede usarlo.

Para volver a las animaciones solas, copia otra vez el `main.py` de `pico_w/`.

## 2. Probar desde la Mac (fases 1 y 2, opcional)

Las dos primeras fases funcionan en cualquier computador con Python 3, así que se pueden probar
antes de tener la Pi 5. Conecta la Pico a la Mac por USB.

```bash
cd asistente_voz/raspberry
```
```bash
python3 -m venv .venv
```
```bash
source .venv/bin/activate
```
```bash
pip install numpy sounddevice pyserial
```
```bash
cp config_ejemplo.py config.py
```
```bash
python prueba_usb.py
```

Para la fase 1, convierte cualquier audio a WAV con la herramienta que trae macOS:
```bash
afconvert -f WAVE -d LEI16@22050 -c 1 ~/Music/cancion.m4a prueba.wav
```
```bash
python reproducir.py prueba.wav
```
Agrega `--modo espectro` para la música, o `--sin-cubo` para probar sin la Pico.

## 3. Instalar la Raspberry Pi 5

### Instalación rápida
Si la Pi ya tiene Raspberry Pi OS funcionando (Lite o con escritorio), tres comandos hacen
casi todo. Primero clona el
repo (paso 3.2) y luego, dentro de `asistente_voz/raspberry`:
```bash
./instalar.sh
```
```bash
./cargar_pico.sh
```
```bash
.venv/bin/python diagnostico.py
```
`instalar.sh` usa `sudo`, se puede repetir sin problema y no toca un `config.py` que ya exista.
Si detecta PipeWire (la versión con escritorio), instala también `pipewire-alsa`. Al final
muestra los dispositivos de audio que encontró. Después ajusta el audio (paso 3.3) y prueba las fases. Para que lo haga el agente OpenClaw,
sigue [OPENCLAW.md](OPENCLAW.md).

Los pasos de abajo explican lo mismo, uno por uno.

### 3.1 Sistema
1. Con [Raspberry Pi Imager](https://www.raspberrypi.com/software/) graba
   **Raspberry Pi OS Lite (64-bit)** en la microSD. La versión con escritorio también sirve,
   pero el audio pasa por PipeWire (ver [Audio con PipeWire](#audio-con-pipewire-versión-con-escritorio)).
   En la configuración del Imager pon:
   - nombre del equipo: `cubo`;
   - tu usuario y contraseña;
   - tu red WiFi;
   - SSH activado.
2. Pon la microSD en la Pi, conecta el micrófono, el parlante y la Pico, y enciéndela.
3. Desde la Mac entra por SSH (usa el usuario que creaste):
   ```bash
   ssh pi@cubo.local
   ```
4. Actualiza e instala lo necesario:
   ```bash
   sudo apt update && sudo apt full-upgrade -y
   ```
   ```bash
   sudo apt install -y git python3-venv python3-dev libportaudio2 alsa-utils
   ```
5. Da permiso a tu usuario para usar el puerto USB de la Pico y el audio (en Raspberry Pi OS
   suele venir así, pero no sobra), y reinicia:
   ```bash
   sudo usermod -aG dialout,audio $USER && sudo reboot
   ```

### 3.2 Programa del asistente
```bash
git clone https://github.com/sfiro/cubo-led-9x9x9.git
```
```bash
cd cubo-led-9x9x9/asistente_voz/raspberry
```
```bash
python3 -m venv .venv
```
```bash
source .venv/bin/activate
```
```bash
pip install -r requirements.txt
```
```bash
cp config_ejemplo.py config.py
```

Cada vez que entres por SSH, activa el entorno antes de correr los programas:
`cd ~/cubo-led-9x9x9/asistente_voz/raspberry && source .venv/bin/activate`.

### 3.3 Audio
1. Mira qué dispositivos encontró la Pi:
   ```bash
   python -m sounddevice
   ```
2. Si el micrófono o el parlante no son los predeterminados, pon en `config.py` el número o
   parte del nombre en `MICROFONO` y `PARLANTE` (por ejemplo, `PARLANTE = "USB"`).
3. Graba 5 segundos y escúchalos, para comprobar los dos:
   ```bash
   arecord -f S16_LE -r 22050 -c 1 -d 5 prueba.wav
   ```
   ```bash
   aplay prueba.wav
   ```
4. Ajusta el umbral de voz: corre el medidor, quédate callado unos segundos y luego habla
   normal. Pon en `UMBRAL_VOZ` un valor entre los dos (por ejemplo, el doble del silencio).
   ```bash
   python medir_microfono.py
   ```

#### Audio con PipeWire (versión con escritorio)
En la versión con escritorio, PipeWire tiene tomados el micrófono y el parlante. Por eso
`aplay -D hw:...` da `Device or resource busy`, y sin el puente `pipewire-alsa`,
`python -m sounddevice` no muestra nada. Con el puente, `python -m sounddevice` muestra solo
`pipewire` y `default`, y es PipeWire el que decide qué micrófono y qué parlante usar:
1. Deja `MICROFONO = None` y `PARLANTE = None` en `config.py`.
2. Mira los dispositivos. Los predeterminados tienen un `*`:
   ```bash
   wpctl status
   ```
3. Si los predeterminados no son tu micrófono y tu parlante USB, elígelos con el número que
   aparece en `Sinks` (parlante) y en `Sources` (micrófono). Por ejemplo:
   ```bash
   wpctl set-default 108
   ```
   ```bash
   wpctl set-default 57
   ```
   PipeWire recuerda la elección aunque reinicies la Pi. Los números pueden cambiar en cada
   arranque, así que revísalos con `wpctl status` antes de repetirlo.
4. Si otro programa usa la cancelación de eco de PipeWire (nodos `echo-cancel-sink` y
   `echo-cancel-source`) y la deja como predeterminada, el asistente también la usará. Funciona,
   pero cambia el volumen del micrófono: vuelve a medir `UMBRAL_VOZ`.
5. Si entras por SSH o lo corre un agente y no aparece ningún dispositivo, falta la variable de
   la sesión:
   ```bash
   export XDG_RUNTIME_DIR=/run/user/$(id -u)
   ```
6. No dejes corriendo al mismo tiempo otro asistente de voz que use el mismo micrófono: los dos
   responderían.

### 3.4 Fase 1: el cubo sigue un audio
```bash
python prueba_usb.py
```
```bash
python reproducir.py prueba.wav
```
Debe encender piso por piso, luego todo el cubo, y después seguir tu voz grabada. Si las luces
van antes o después que el sonido, ajusta `AJUSTE_SINCRONIA_S` en `config.py`.

### 3.5 Fase 2: el cubo habla
Descarga una voz en español de [Piper](https://huggingface.co/rhasspy/piper-voices/tree/main/es)
(cada voz son dos archivos, el `.onnx` y su `.onnx.json`):
```bash
mkdir -p voces
```
```bash
curl -L -o voces/es_MX-claude-high.onnx https://huggingface.co/rhasspy/piper-voices/resolve/main/es/es_MX/claude/high/es_MX-claude-high.onnx
```
```bash
curl -L -o voces/es_MX-claude-high.onnx.json https://huggingface.co/rhasspy/piper-voices/resolve/main/es/es_MX/claude/high/es_MX-claude-high.onnx.json
```
```bash
python decir.py "Hola, soy el cubo de luz"
```
Hay otras voces (de España, Argentina y México) en la misma página. Si cambias de voz, cambia
`VOZ_PIPER` en `config.py`. Las voces `medium` generan el audio más rápido que las `high`.

### 3.6 Fase 3: el asistente completo
1. Instala [Ollama](https://ollama.com) y descarga el modelo:
   ```bash
   curl -fsSL https://ollama.com/install.sh | sh
   ```
   ```bash
   ollama pull qwen2.5:1.5b
   ```
2. Pruébalo solo:
   ```bash
   ollama run qwen2.5:1.5b "Hola, ¿quién eres?"
   ```
3. Arranca el asistente. La primera vez descarga el modelo de Whisper, así que tarda un poco más.
   ```bash
   python asistente.py
   ```
4. Cuando diga `Listo`, háblale. En la consola verás lo que entendió (`Tu:`), lo que respondió
   (`Cubo:`) y cuánto tardó cada etapa.

Para cambiar de modelo, descárgalo con `ollama pull` y ponlo en `MODELO` en `config.py`:

| Modelo | Comentario |
|---|---|
| `gemma3:1b` | El más rápido, respuestas más simples |
| `qwen2.5:1.5b` | Buen equilibrio (el predeterminado) |
| `llama3.2:3b`, `qwen2.5:3b` | Mejores respuestas, tarda más en empezar a hablar |

Los modelos cambian rápido: cualquiera de 1 a 4 mil millones de parámetros que esté en Ollama
sirve. Si un modelo "piensa en voz alta" (`<think>`), el asistente no lee esa parte.

### Controlar el cubo con la voz (v2)
Mientras conversas, puedes pedirle animaciones:

| Dices | Qué pasa |
|---|---|
| "Pon la lluvia", "muéstrame los fuegos artificiales", o solo "Matrix" | Pone esa animación. Se repite hasta que pidas otra |
| "Escribe feliz cumpleaños" | El texto corre por las caras del cubo |
| "¿Qué hora es?" | Dice la hora y la muestra |
| "Modo demo" | Recorre las 34 animaciones y dice el nombre de cada una antes (v4.1) |
| "Para", "detente", "apaga las luces" | Quita la animación |
| "Pon algo relajante" | El modelo elige una (las olas, por ejemplo) |

Lo demás es conversación normal. Cuando el asistente escucha o habla, el cubo muestra la voz y
después vuelve a la animación. La lista completa de comandos y de nombres está en
[versiones/v2.md](versiones/v2.md).

Desde la v3 también se puede cambiar el brillo: "baja el brillo", "sube el brillo", "brillo al
máximo", "pon el brillo al 40 %". Ver [versiones/v3.md](versiones/v3.md).

Para probar los comandos y el modelo sin hablar:
```bash
python asistente.py --escribir
```

### 3.7 "Hola cubo" y otras formas de activarlo
Con `ACTIVACION = "frase"` (desde la v4) duerme hasta oír **"hola cubo"** y luego queda atento
20 s para conversar. "Adiós cubo" lo vuelve a dormir. Los detalles están en
[versiones/v4.md](versiones/v4.md). La frase se cambia en `FRASE_ACTIVACION`.

Con `ACTIVACION = "voz"`, el asistente responde a cualquier frase que escuche.

Con `ACTIVACION = "palabra"` usa openWakeWord, un detector especializado, más liviano pero en
inglés:
```bash
pip install openwakeword
```
Luego pon en `config.py` `ACTIVACION = "palabra"`. Los modelos que trae openWakeWord están en
inglés (`hey_jarvis`, `alexa`, `hey_mycroft`). Se puede entrenar uno propio, en español,
siguiendo la documentación de openWakeWord.

### 3.8 Arranque automático
1. Edita `asistente.service` y cambia `pi` por tu usuario (en `User` y en las dos rutas).
2. Instálalo:
   ```bash
   sudo cp asistente.service /etc/systemd/system/
   ```
   ```bash
   sudo systemctl enable --now asistente
   ```
3. Para ver lo que imprime:
   ```bash
   journalctl -u asistente -f
   ```

Ese servicio usa el audio de ALSA directamente, que es lo que hay en Raspberry Pi OS Lite.

**Con escritorio (PipeWire)**, el asistente tiene que correr dentro de la sesión del usuario,
que es donde está PipeWire. Para eso está `asistente-usuario.service`, que no hay que editar
si el repo está en `~/cubo-led-9x9x9`:
```bash
mkdir -p ~/.config/systemd/user && cp asistente-usuario.service ~/.config/systemd/user/asistente.service
```
```bash
systemctl --user daemon-reload && systemctl --user enable --now asistente
```
Para que arranque al encender la Pi aunque nadie inicie sesión:
```bash
sudo loginctl enable-linger $USER
```
Para ver lo que imprime:
```bash
journalctl --user -u asistente -f
```

## Problemas comunes

| Síntoma | Qué revisar |
|---|---|
| `python -m sounddevice` no muestra nada | La Pi tiene PipeWire y falta el puente: `sudo apt install pipewire-alsa`. Ver [Audio con PipeWire](#audio-con-pipewire-versión-con-escritorio) |
| `aplay` da `Device or resource busy` | PipeWire tiene tomado el dispositivo. Es normal con escritorio: usa `aplay -D default` |
| Usa otro micrófono o parlante | Con PipeWire, elige los predeterminados con `wpctl set-default` |
| `No encontre la Pico por USB` | El cable USB tiene que ser de datos, no solo de carga. Revisa `ls /dev/ttyACM*` |
| `La Pico ... no responde` | Cierra Thonny, revisa que en la Pico estén `cubo.py` y el `main.py` de `asistente_voz/pico`, y reconéctala |
| La esfera de espera no aparece al conectar la Pico | Revisa el cableado y el driver con `pico_w/diagnostico.py` |
| El asistente se responde a sí mismo | El micrófono oye el parlante. Aleja los dos, baja el volumen o usa un altavoz de conferencias con cancelación de eco |
| No reacciona cuando le hablas | Baja `UMBRAL_VOZ` (mídelo con `medir_microfono.py`) |
| Corta las preguntas por la mitad | Sube `SILENCIO_FIN_S` |
| Entiende mal | Usa `WHISPER_MODELO = "small"` (más lento) y habla más cerca del micrófono |
| Tarda mucho en contestar | Un modelo más chico, una voz `medium`, o `WHISPER_MODELO = "tiny"`. Revisa que la Pi no esté caliente: `vcgencmd measure_temp` |
| Piper se cierra con `Error processing file '.../phontab'` | La ruta de la carpeta es demasiado larga para espeak-ng (unos 160 caracteres). Clona el repo en una ruta corta, como `~/cubo-led-9x9x9` |
| Las luces no van con la voz | Ajusta `AJUSTE_SINCRONIA_S` (positivo si las luces van antes que el sonido) |

## Ideas para seguir
- Control de velocidad y duración de las animaciones ("más rápido", "por 5 minutos").
- Interrumpir al asistente mientras habla (necesita cancelación de eco).
- Un detector de "hola cubo" entrenado (openWakeWord), para no depender de Whisper estando
  dormido.
