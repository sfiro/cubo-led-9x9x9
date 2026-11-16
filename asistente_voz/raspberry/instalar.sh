#!/usr/bin/env bash
# Instala todo lo que necesita el asistente en la Raspberry Pi 5.
# Se puede correr varias veces: lo que ya esta instalado no se repite.
#   ./instalar.sh
set -euo pipefail
cd "$(dirname "$0")"

echo "== 1/7 Paquetes del sistema =="
sudo apt-get update
sudo apt-get install -y git curl python3-venv python3-dev libportaudio2 alsa-utils
PIPEWIRE=no
if dpkg -s pipewire > /dev/null 2>&1; then
    # Version con escritorio: PipeWire tiene tomados el microfono y el parlante.
    # Sin este puente, PortAudio (sounddevice) no ve ningun dispositivo.
    PIPEWIRE=si
    echo "La Pi usa PipeWire: se instala pipewire-alsa (el puente de ALSA a PipeWire)"
    sudo apt-get install -y pipewire-alsa
fi

echo "== 2/7 Entorno de Python (.venv) =="
[ -d .venv ] || python3 -m venv .venv
.venv/bin/pip install --upgrade pip
.venv/bin/pip install -r requirements.txt mpremote

echo "== 3/7 Configuracion =="
if [ -f config.py ]; then
    echo "config.py ya existe: no se toca"
else
    cp config_ejemplo.py config.py
    echo "config.py creado desde config_ejemplo.py"
fi

echo "== 4/7 Voz de Piper =="
VOZ=$(.venv/bin/python -c "import config; print(config.VOZ_PIPER)")
if [ -f "$VOZ" ] && [ -f "$VOZ.json" ]; then
    echo "$VOZ ya esta"
else
    # es_MX-claude-high -> es/es_MX/claude/high/es_MX-claude-high
    BASE=$(basename "$VOZ" .onnx)
    IFS=- read -r LOCAL NOMBRE CALIDAD <<< "$BASE"
    URL="https://huggingface.co/rhasspy/piper-voices/resolve/main/${LOCAL%%_*}/$LOCAL/$NOMBRE/$CALIDAD/$BASE"
    mkdir -p "$(dirname "$VOZ")"
    curl -fL -o "$VOZ" "$URL.onnx"
    curl -fL -o "$VOZ.json" "$URL.onnx.json"
fi

echo "== 5/7 Ollama y el modelo =="
if ! command -v ollama > /dev/null; then
    curl -fsSL https://ollama.com/install.sh | sh
fi
MODELO=$(.venv/bin/python -c "import config; print(config.MODELO)")
ollama pull "$MODELO"
# v5: modelo para buscar en los documentos de ../conocimiento/
EMBEDDINGS=$(.venv/bin/python -c "import config; print(getattr(config, 'MODELO_EMBEDDINGS', 'bge-m3'))")
ollama pull "$EMBEDDINGS"
if [ ! -d ../conocimiento ]; then
    cp -r ../conocimiento_ejemplo ../conocimiento
    echo "../conocimiento creado desde la plantilla: llena negocio.md y revisa cali.md"
else
    # Plantillas nuevas de versiones posteriores (nombres_productos.md en la v5.1),
    # sin tocar los archivos que ya llenaste
    for f in ../conocimiento_ejemplo/*; do
        if [ ! -e "../conocimiento/$(basename "$f")" ]; then
            cp "$f" ../conocimiento/
            echo "Agregado a ../conocimiento: $(basename "$f")"
        fi
    done
fi

echo "== 6/7 Permisos para el USB y el audio =="
if id -nG | grep -qw dialout && id -nG | grep -qw audio; then
    echo "$USER ya esta en los grupos dialout y audio"
else
    sudo usermod -aG dialout,audio "$USER"
    echo "AVISO: se agrego $USER a dialout y audio. Cierra la sesion y vuelve a entrar"
    echo "(o reinicia la Pi) antes de usar la Pico y el audio."
fi

echo "== 7/7 Audio =="
# PortAudio necesita encontrar el PipeWire de la sesion (al entrar por SSH o desde un agente)
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
.venv/bin/python -m sounddevice || true
if [ "$PIPEWIRE" = si ]; then
    echo "Con PipeWire deben aparecer 'pipewire' y 'default'. El asistente usa el microfono y"
    echo "el parlante predeterminados de PipeWire: revisalos con 'wpctl status' (los del *)."
fi

echo
echo "Listo. Siguiente paso: ./cargar_pico.sh y luego .venv/bin/python diagnostico.py"
