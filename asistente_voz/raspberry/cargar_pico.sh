#!/usr/bin/env bash
# Copia el firmware del asistente a la Pico conectada por USB (con mpremote).
# Antes guarda en respaldo_pico/ los .py que tenga la Pico, para poder volver a
# las animaciones solas. El respaldo no se sobrescribe si ya existe.
#   ./cargar_pico.sh
#   ./cargar_pico.sh --restaurar      vuelve a poner el main.py del respaldo
set -euo pipefail
cd "$(dirname "$0")"
M=.venv/bin/mpremote
[ -x "$M" ] || { echo "Falta mpremote: corre primero ./instalar.sh"; exit 1; }

if [ "${1:-}" = "--restaurar" ]; then
    [ -f respaldo_pico/main.py ] || { echo "No hay respaldo_pico/main.py"; exit 1; }
    "$M" connect auto fs cp respaldo_pico/main.py :main.py + reset
    echo "Pico restaurada con el main.py del respaldo"
    exit 0
fi

echo "Archivos que tiene la Pico ahora:"
LISTA=$("$M" connect auto fs ls)
echo "$LISTA"
mkdir -p respaldo_pico
for f in $(echo "$LISTA" | awk '{print $2}' | grep '\.py$' || true); do
    if [ -e "respaldo_pico/$f" ]; then
        echo "respaldo_pico/$f ya existe: no se sobrescribe"
    else
        "$M" connect auto fs cp ":$f" "respaldo_pico/$f"
    fi
done

"$M" connect auto fs cp ../../pico_w/cubo.py :cubo.py + fs cp ../pico/main.py :main.py + reset
echo "Firmware cargado. En unos 3 s el cubo debe mostrar la esfera que respira."
