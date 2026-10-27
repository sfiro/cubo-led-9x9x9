# Simulador 3D del cubo LED

Una página web que ejecuta **el mismo `pico_w/animaciones.py`** de la Pico W y muestra el cubo
de 9×9×9 en 3D. Sirve para probar animaciones antes de cargarlas en el cubo real.

## Qué tiene
- **Visor 3D**: los 729 LEDs con columnas y pisos. Se puede girar y hacer zoom con el mouse o el
  dedo, y hay vistas fijas (3D, frente, lado, arriba). Al pasar el mouse sobre un LED se ven sus
  coordenadas `c.led(z, y, x)`.
- **Animaciones**: un botón por cada una de `LISTA`, más el modo demo.
- **Reproducción**: pausa, avance cuadro a cuadro, velocidad de 0.1× a 8× y opción de repetir.
  Usa un reloj virtual: en pausa, o a otra velocidad, las duraciones de las animaciones
  (`_durante`) siguen siendo las correctas.
- **Datos**: tiempo de la animación, cuadros por segundo, LEDs encendidos, tiempo de cálculo
  de cada cuadro y LEDs dibujados fuera del cubo. Estos últimos se ignoran, y suelen indicar
  un error en la animación.
- **Editor**: escribe una función `async def nombre(c)` y ejecútala con el botón o con
  Ctrl/Cmd+Enter. El código se guarda en el navegador. Los errores salen en la consola con la
  línea donde ocurrieron.
- **Protección contra cuelgues**: Python corre en un *Web Worker*. Si una animación entra en un
  bucle sin `await`, la página avisa y ofrece reiniciar Python.

## Cómo funciona
```
index.html + app.js  ── Three.js: escena, controles, editor
      │  ▲ mensajes (cuadros, salida, latido)
      ▼  │
worker.js ── MicroPython 1.27 (WebAssembly)
      ├─ /lib/animaciones.py  ← ../pico_w/animaciones.py (sin cambios)
      ├─ /lib/fuente.py       ← ../pico_w/fuente.py
      ├─ /lib/cubo.py         ← cubo_sim.py (misma interfaz que pico_w/cubo.py)
      └─ /lib/sim_runner.py   ← reemplaza time/asyncio por un reloj virtual
```
- `cubo_sim.py` tiene los mismos métodos que el `Cubo` real (`led`, `leer`, `apagar`,
  `encender`, `plano`, `mostrar`). `mostrar()` manda el cuadro a la página en vez de enviarlo a
  los 74HC595.
- `sim_runner.py` reemplaza los módulos `time` y `asyncio` que usan las animaciones por
  versiones con reloj virtual. Por eso funcionan la pausa, el avance cuadro a cuadro y la
  velocidad sin modificar `animaciones.py`.

## Limitaciones
- El navegador es mucho más rápido que la Pico. Si "Cálculo del cuadro" pasa de unos pocos
  milisegundos, en el cubo real esa animación irá más lenta.
- No simula problemas eléctricos: la señal de 3.3 V, LEDs que brillan sin deber o diferencias
  de brillo entre pisos.

## Usarlo en tu computador
Hay que servirlo por HTTP, porque abriendo el archivo directamente el navegador no deja cargar
`../pico_w/`. Desde la carpeta raíz del repo:
```bash
python3 -m http.server 8765
```
Luego abre <http://localhost:8765/simulador/>. Necesita internet la primera vez, para descargar
MicroPython y Three.js desde cdn.jsdelivr.net.

## Publicarlo en GitHub Pages
En el repositorio de GitHub ve a **Settings → Pages**, elige **Deploy from a branch**, rama
`main` y carpeta `/ (root)`, y guarda. Queda disponible en
`https://<usuario>.github.io/cubo-led-9x9x9/`, que redirige al simulador.
