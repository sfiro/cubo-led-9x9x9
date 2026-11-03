# Asistente de voz con el cubo LED

Un asistente de voz que funciona **completamente local** en una Raspberry Pi 5, sin nube.
El cubo de 9×9×9 es su cara: se ilumina al ritmo de la voz cuando el asistente habla, y
muestra otra animación mientras escucha y mientras piensa.

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
| `raspberry/asistente.py` | El asistente completo (fase 3) |
| `raspberry/reproducir.py` | Fase 1: el cubo sigue un archivo WAV |
| `raspberry/decir.py` | Fase 2: el cubo dice con voz lo que escribas |
| `raspberry/prueba_usb.py` | Prueba la conexión: piso por piso y velocidad |
| `raspberry/medir_microfono.py` | Muestra el volumen del micrófono para ajustar `UMBRAL_VOZ` |
| `raspberry/config_ejemplo.py` | Plantilla de configuración. Cópiala como `config.py`, que queda fuera del repo |
| `raspberry/asistente.service` | Servicio para que arranque solo al encender la Pi |
| `raspberry/instalar.sh` | Instala todo en la Pi 5 (paquetes, Python, voz, Ollama y modelo, permisos) |
| `raspberry/cargar_pico.sh` | Respalda lo que tiene la Pico y le carga el firmware (`--restaurar` lo devuelve) |
| `raspberry/diagnostico.py` | Prueba cada parte por separado sin que nadie hable, y termina con el cubo hablando |
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
Si la Pi ya tiene Raspberry Pi OS funcionando, tres comandos hacen casi todo. Primero clona el
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
Después ajusta el audio (paso 3.3) y prueba las fases. Para que lo haga el agente OpenClaw,
sigue [OPENCLAW.md](OPENCLAW.md).

Los pasos de abajo explican lo mismo, uno por uno.

### 3.1 Sistema
1. Con [Raspberry Pi Imager](https://www.raspberrypi.com/software/) graba
   **Raspberry Pi OS Lite (64-bit)** en la microSD. En la configuración del Imager pon:
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

### 3.7 Palabra de activación (opcional)
Con `ACTIVACION = "voz"`, el asistente responde a cualquier frase que escuche. Para que espere
una palabra:
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

El servicio usa el audio de ALSA directamente, que es lo que hay en Raspberry Pi OS Lite. Con
la versión de escritorio el audio pasa por PipeWire y el servicio puede no encontrarlo.

## Problemas comunes

| Síntoma | Qué revisar |
|---|---|
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
- Interrumpir al asistente mientras habla (necesita cancelación de eco).
- Niveles de brillo en el cubo, para que las ondas se desvanezcan suavemente.
- Que el modelo pueda elegir animaciones o mostrar texto ("muéstrame la hora").
- Una palabra de activación entrenada en español.
