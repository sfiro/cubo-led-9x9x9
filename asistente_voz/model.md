# Modelo del sistema: asistente de voz con el cubo LED

Cómo funciona el asistente por dentro. Es la referencia para cambiar el protocolo, las
animaciones o el flujo de la conversación sin romper la sincronía entre la voz y el cubo. Lo
que pasa dentro del cubo (registros, mapa de bits, multiplexado) está en el
[model.md](../model.md) del proyecto principal.

## 1. Reparto del trabajo

```
┌──────────────────────── Raspberry Pi 5 (Linux, Python 3) ────────────────────────┐
│                                                                                  │
│  Microfono ─► escuchar_pregunta ─► faster-whisper ─► Ollama ─► Frases ─► Locutor │
│  (hilo de     (detecta voz y       (voz a texto)     (stream)  (corta    (Piper) │
│   audio)       silencio)                                        frases)     │    │
│                                                                             ▼    │
│                    Animador ◄── estado_audio() ◄── Reproductor ──► parlante      │
│                    (40 cuadros/s)                  (cola de audio + análisis)    │
│                         │                                                        │
└─────────────────────────┼────────────────────────────────────────────────────────┘
                          │ USB serie: una línea de texto por cuadro
                          ▼
              Pico: main.py ─► cubo.py (PIO + DMA) ─► 74HC595 ─► LEDs
```

- **La Pi** hace todo lo que necesita potencia: reconocer la voz, generar la respuesta,
  sintetizar la voz, analizar el audio y calcular cada cuadro del cubo.
- **La Pico** solo refresca el cubo. Recibe un cuadro, lo copia a `vox` y llama a
  `mostrar()`. El refresco de los 9 pisos lo siguen haciendo el PIO y el DMA, así que el
  cubo no parpadea aunque Linux haga pausas.
- Se eligió esto en vez de manejar los 74HC595 desde los GPIO de la Pi porque Linux no es de
  tiempo real: cualquier pausa del sistema dejaría un piso encendido de más y se vería como
  un parpadeo.

## 2. Hardware

| Conexión | Señales | Notas |
|---|---|---|
| Pi 5 → Pico | USB (CDC serie, `/dev/ttyACM0`) | Alimenta a la Pico. Al ser USB, no hay problema de niveles de 3.3 V |
| Pico → cubo | GP2 DATA, GP3 CLOCK, GP4 LATCH, GND | Igual que en `pico_w/` |
| Pi 5 → audio | USB | La Pi 5 no tiene salida de 3.5 mm. En Lite, PortAudio usa ALSA directo; con escritorio pasa por PipeWire (`pipewire-alsa`) y PipeWire elige los dispositivos |

El cubo tiene su propia fuente de 5 V. La Pi, la Pico y el cubo comparten el GND a través del
USB y del cable de GND entre la Pico y el cubo.

## 3. Protocolo USB

Líneas de texto ASCII terminadas en `\n`, a cualquier velocidad (es USB, el baud rate no
importa):

| Línea | Significado | Respuesta de la Pico |
|---|---|---|
| `F` + 184 dígitos hexadecimales | Un cuadro: 92 bytes = 736 bits, se usan los primeros 729 | Ninguna |
| `A` | Apagar todo y no mostrar la animación de espera | Ninguna |
| `G` + 730 dígitos hexadecimales (v3) | Un cuadro con brillo: 365 bytes, 4 bits por LED (niveles 0..15) | Ninguna |
| `?` | Pregunta quién es | `CUBO9 <versión> <bits>` (el firmware de la v1 responde solo `CUBO9 1`) |

**Orden de los bits**: el bit `b` (0 = el menos significativo) del byte `k` es el LED
`i = 8k + b`, con `i = z*81 + y*9 + x`, las mismas coordenadas de la sección 3 del
[model.md principal](../model.md#3-coordenadas). En la Pi es
`np.packbits(vox.reshape(-1), bitorder="little")` con `vox[z, y, x]`. En la Pico lo deshace
`_desempaquetar` (viper).

**Por qué texto y no bytes crudos**: la consola USB de MicroPython interpreta el byte `0x03`
como Ctrl-C y detiene el programa. Un cuadro binario lo tendría a menudo. En hexadecimal nunca
aparece, así que Thonny puede seguir deteniendo el programa como siempre. El costo es el doble
de bytes: 186 por cuadro, unos 7.4 KB/s a 40 cuadros por segundo, muy poco para el USB.

**Detección**: `cubo_usb.buscar_puerto()` busca un puerto con el fabricante USB `0x2E8A`
(Raspberry Pi). Luego envía `?` y espera `CUBO9` hasta 1.5 s. Si la Pico está en la consola
de MicroPython y no en este `main.py`, esa respuesta no llega y se avisa.

**Espera**: si pasan 3 s sin cuadros, la Pico muestra por su cuenta una esfera que respira.
Así se nota si la Pi se apagó o el programa se detuvo. Después de `A` no la muestra.

## 4. Hilos en la Pi

| Hilo | Qué hace | Comunicación |
|---|---|---|
| Principal | Escuchar, transcribir, preguntar a Ollama, cortar frases | Pone frases en `Locutor.frases` y cambia `Animador.estado` |
| `Locutor` | Convierte cada frase en audio con Piper | Llama a `Reproductor.agregar()` |
| Audio de salida (PortAudio) | `Reproductor._llenar`: saca el siguiente bloque de 20 ms de la cola, lo analiza y anota cuándo va a sonar | Escribe en `Reproductor._eventos` |
| Audio de entrada (PortAudio) | `Microfono._recibir`: guarda el volumen y pone bloques de 30 ms en una cola | `Microfono.rms` y `Microfono.cola` |
| `Animador` | 40 veces por segundo calcula el cuadro según el estado y lo envía | Lee `Reproductor.estado_audio()` y `Microfono.rms` |

Mientras el modelo sigue escribiendo, el `Locutor` ya está sintetizando la primera frase y el
reproductor puede estar sonando. Así el asistente empieza a hablar sin esperar la respuesta
completa.

## 5. Flujo de una conversación

1. **Reposo**: el hilo principal lee bloques del micrófono. Con `ACTIVACION = "palabra"`,
   antes espera a que openWakeWord dé más de 0.5 con trozos de 80 ms a 16 kHz. Con
   `ACTIVACION = "frase"` (v4), mientras duerme transcribe cada frase con la guía
   "Hola Cubo." y solo sigue si `activacion.buscar_frase` la encuentra. Después queda atento
   `VENTANA_CONVERSACION_S` (ver [versiones/v4.md](versiones/v4.md#cómo-funciona)).
2. **Escuchando**: cuando el volumen (RMS) pasa `UMBRAL_VOZ`, empieza a grabar e incluye los
   0.3 s anteriores, para no cortar la primera sílaba. Termina tras `SILENCIO_FIN_S` de silencio
   o `MAX_PREGUNTA_S`. Si hubo menos de 0.4 s de voz, lo descarta como ruido.
3. **Pensando**: remuestrea a 16 kHz, transcribe con faster-whisper (`int8`, `beam_size=1`,
   con filtro de voz) y envía el historial a Ollama (`/api/chat` con `stream`). La conversación
   guarda los últimos `MAX_HISTORIAL` mensajes.
4. **Frases**: `voz.Frases` junta los pedazos y corta cuando hay un signo de fin de frase seguido
   de un espacio, con un mínimo de 12 caracteres. Un "3.5" no corta, porque no hay espacio
   después del punto. Quita el formato (`*`, `#`, etc.), los emojis y los bloques `<think>`.
5. **Hablando**: cada frase pasa por Piper y va a la cola del reproductor. Mientras haya audio
   sonando, el `Animador` muestra la animación de hablar, sin que el hilo principal tenga que
   avisar.
6. **Fin del turno**: espera a que se sinteticen todas las frases y a que termine el audio,
   espera 0.3 s más para que se apague el eco de la sala, y vacía la cola del micrófono. Así el
   asistente no se escucha a sí mismo (es *half duplex*: no se le puede interrumpir).

## 6. Sincronía entre la voz y el cubo

La clave es que **el cubo no escucha el parlante**: el reproductor tiene en la mano cada bloque
antes de que suene.

```
_llenar (hilo de audio), cada 20 ms:
    bloque = siguientes N muestras de la cola (o silencio)
    cuando = ahora + latencia de salida + AJUSTE_SINCRONIA_S
    _eventos.append((cuando, nivel, bandas))

Animador, cada 25 ms:
    nivel, bandas = último evento con cuando <= ahora
```

- `latencia de salida` es la que informa PortAudio (`OutputStream.latency`), es decir, el
  tiempo entre entregar un bloque y que salga por el parlante.
- `AJUSTE_SINCRONIA_S` corrige lo que PortAudio no sabe: parlantes Bluetooth, el procesamiento
  del altavoz, o el retardo del USB y del refresco del cubo (unos pocos ms).
- Se usa `time.monotonic()` en vez de los tiempos de PortAudio, porque algunos controladores de
  ALSA los informan en cero.

## 7. Análisis del audio

`audio.Analizador`, para cada bloque de 20 ms:
- **Nivel**: `20·log10(RMS)` en dBFS, llevado a 0..1 entre −50 dBFS (silencio) y −10 dBFS (voz
  fuerte). Sube rápido (factor 0.6) y baja lento (0.2), como un vúmetro.
- **Bandas**: FFT con ventana de Hann. Los bins entre 120 Hz y 5 kHz se agrupan en 9 bandas
  espaciadas logarítmicamente, una por columna `x` del cubo. Se usa `log1p` y se normaliza con
  el máximo reciente, que decae un 0.5 % por bloque. Se multiplica por el nivel, para que en
  silencio el ecualizador caiga a cero.

Con 20 ms a 22 050 Hz, cada bin de la FFT mide unos 50 Hz: la banda más grave tiene pocos bins,
pero alcanza para la voz.

## 8. Animaciones

Todas son funciones de numpy sobre grillas precalculadas (`DIST` al centro, `RADIO_H` al eje
vertical y `ANGULO`). Devuelven un arreglo `(9, 9, 9)` de booleanos `[z, y, x]` y cuestan menos
de 0.1 ms por cuadro.

| Estado | Función | Idea |
|---|---|---|
| `reposo` | `reposo(t)` | Cáscara esférica de radio 2.2 ± 0.9, con un período de 3 s |
| `escuchando` | `escuchando(t, nivel)` | Cilindro de radio y altura proporcionales al volumen del micrófono |
| `pensando` | `pensando(t)` | Doble hélice de radio 3 que gira a 3.5 rad/s |
| `hablando` | `Ondas` | Núcleo que late con el volumen, y cáscaras de radio 1.8, 3.4 y 5.0 que muestran el volumen de 3, 6 y 9 cuadros atrás |
| `hablando` | `esfera` | Una cáscara de radio 0.6 + 3.6 × nivel |
| `hablando` | `espectro` | Cada columna `x` es una banda: altura y profundidad según su energía |

Para agregar una animación de hablar, escribe una función `f(t, nivel, bandas)` en `visual.py`
y agrégala a `MODOS_HABLANDO`. Si necesita memoria entre cuadros, que sea una clase con
`__call__`, como `Ondas`.

**Corriente**: con el volumen al máximo, `Ondas` enciende hasta unos 290 LEDs y `esfera` unos
280. Como el cubo enciende un piso a la vez, el peor caso por piso sigue siendo 81 LEDs, igual
que con `encender()`.

## 9. Tiempos aproximados en una Pi 5

Son estimaciones, sin medir todavía en este proyecto. El asistente imprime los tiempos reales en
cada turno.

| Etapa | Tiempo |
|---|---|
| Detectar el fin de la pregunta | `SILENCIO_FIN_S` (0.9 s) |
| Whisper `base` con una pregunta de 3 a 5 s | 1 a 2 s (`tiny`: unos 0.5 s) |
| Modelo de 1.5B hasta la primera frase | 0.5 a 1.5 s |
| Piper, primera frase | 0.2 a 0.6 s (más con voces `high`) |
| **Desde que te callas hasta que empieza a hablar** | **unos 3 a 4 s** |

## 10. Control de las animaciones con la voz (v2)

```
texto ─► comandos.interpretar ──sí──► Accion ─► MotorAnimaciones.iniciar / detener
            │ no                          ▲
            ▼                             │ comandos.de_etiqueta
         Ollama (instrucciones + lista) ─► Frases: quita [ANIMACION: x] y la guarda
```

- **MotorAnimaciones** (`animaciones_pi.py`) carga `pico_w/animaciones.py` por su ruta, sin
  agregar `pico_w/` a `sys.path` (allí hay otro `config.py`, el de la Pico). Le cambia `time` y
  `asyncio` por versiones con `ticks_ms()` y `sleep_ms()`, que MicroPython tiene y Python no,
  como hace el simulador. Las corre en un bucle asyncio en su propio hilo. Elegir otra
  animación cancela la tarea, que es lo mismo que hace la página web de la Pico.
- **CuboVirtual**: misma interfaz que `pico_w/cubo.py`. Su `mostrar()` guarda una copia del
  cuadro como arreglo de numpy, que el `Animador` toma 40 veces por segundo.
- **Prioridad en el Animador**:
  1. si `VOZ_SOBRE_ANIMACION` es `False` y hay animación, la animación;
  2. si el reproductor está sonando, la animación de hablar;
  3. escuchando o pensando;
  4. la animación elegida, o si no hay, el reposo.
- **Comandos directos** (`comandos.interpretar`): el texto se normaliza (minúsculas, sin tildes
  ni signos) y se reconoce en este orden:
  1. parar, solo si la frase entera es la orden ("para", "apaga las luces"), para no confundir
     "¿para qué sirve…?";
  2. escribir texto;
  3. la hora;
  4. verbo más nombre de animación, o solo el nombre. Se buscan primero los nombres más largos,
     para que "esfera móvil" le gane a "esfera".
- **Etiquetas del modelo**: `instrucciones_cubo()` agrega a las instrucciones la lista de
  animaciones, la que se está mostrando, y la fecha y la hora. `voz.Frases` quita las
  etiquetas completas, y un `[` sin cerrar lo guarda hasta que llegue el resto, para que una
  etiqueta cortada en dos pedazos no se lea en voz alta. Una etiqueta con un nombre que no
  existe se ignora y se avisa en la consola.
- **Historial**: también los comandos directos se guardan con su etiqueta ("Listo, pongo la
  lluvia. [ANIMACION: lluvia]"). Así el modelo ve ejemplos del formato y sabe qué se está
  mostrando.

## 11. Decisiones y límites

- **Half duplex**: el micrófono se ignora mientras el asistente habla. Para poder interrumpirlo
  hace falta cancelación de eco (un altavoz de conferencias, o el módulo AEC de
  PipeWire/WebRTC).
- **Niveles de brillo (v3)**: 16 niveles con BAM en el driver de la Pico, a 123 Hz. El cálculo
  de las pasadas, la gamma, la estela y el protocolo `G` están en
  [versiones/v3.md](versiones/v3.md#cómo-funciona-bam). Si la Pico tiene el firmware de la v1,
  la Pi le envía cuadros `F` y los bordes suaves se cortan a la mitad, con las mismas formas de
  la v2.
- **Un solo cliente**: el puerto serie lo abre un programa a la vez. `reproducir.py`, `decir.py`
  y `asistente.py` no pueden correr juntos, ni con Thonny abierto.
- **Configuración**: `config.py` es local y no se sube al repo. Si falta, se usa
  `config_ejemplo.py`, igual que en `pico_w/`.
