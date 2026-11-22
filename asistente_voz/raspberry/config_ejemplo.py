# Plantilla de configuracion. Copiala como config.py y ajustala ahi:
# config.py no se sube al repositorio (esta en .gitignore).

# ---- Cubo -------------------------------------------------------------------
PUERTO_CUBO = None          # None = buscar la Pico sola (en la Pi suele ser /dev/ttyACM0)
CUADROS_POR_SEGUNDO = 40
ANIMACION_HABLANDO = "ondas"    # "ondas", "esfera" o "espectro"
# Segundos que se adelanta (+) o atrasa (-) el cubo respecto a la voz.
# Subelo si las luces van antes que el sonido (parlantes Bluetooth: ~0.2)
AJUSTE_SINCRONIA_S = 0.0

# Animaciones con la voz (v2)
COMANDOS_DIRECTOS = True    # "pon la lluvia", "para", "escribe HOLA" sin pasar por el modelo
ANUNCIAR_DEMO = True        # en el modo demo, dice el nombre de cada animacion antes (v4.1)
VOZ_SOBRE_ANIMACION = True  # al escuchar/pensar/hablar se ve la voz, y luego vuelve la animacion.
                            # False: la animacion elegida sigue aunque el asistente hable

# Niveles de brillo (v3). Necesitan el firmware nuevo en la Pico (./cargar_pico.sh)
BRILLO = 1.0                # brillo al arrancar, 0.25..1 (se cambia con la voz)
ESTELA_MS = 150             # en las animaciones, lo que se apaga se desvanece en este tiempo.
                            # 0 = sin estela
GAMMA_BRILLO = 2.2          # corrige el brillo para que los niveles se vean parejos

# ---- Audio ------------------------------------------------------------------
# None = el dispositivo predeterminado. Si no, el numero o parte del nombre
# que muestra "python -m sounddevice"
MICROFONO = None
PARLANTE = None

# ---- Escuchar ---------------------------------------------------------------
# "frase": duerme hasta oir FRASE_ACTIVACION ("hola cubo") y luego queda atento (v4).
# "voz": escucha siempre y responde a cualquier frase.
# "palabra": espera la palabra de activacion de openWakeWord (en ingles, hay que instalarlo).
ACTIVACION = "frase"
FRASE_ACTIVACION = "hola cubo"
VENTANA_CONVERSACION_S = 20     # despues de responder, sigue atento este tiempo sin la frase
RESPUESTA_ACTIVACION = "¿Sí?"   # lo que dice si solo le dices "hola cubo" ("" = solo luz)
PALABRA_ACTIVACION = "hey_jarvis"   # modelos que trae openWakeWord (en ingles)
UMBRAL_VOZ = 0.02           # volumen (RMS) desde el que se considera que alguien habla.
                            # Medirlo con: python medir_microfono.py
SILENCIO_FIN_S = 0.7        # silencio que marca el fin de la pregunta
MAX_PREGUNTA_S = 15

# ---- Voz a texto (faster-whisper) ------------------------------------------
WHISPER_MODELO = "base"     # "tiny" (rapido), "base" o "small" (preciso, lento)
IDIOMA = "es"

# ---- Modelo de lenguaje (Ollama) -------------------------------------------
OLLAMA_URL = "http://localhost:11434"
MODELO = "qwen2.5:1.5b"     # probar tambien "gemma3:1b" (rapido) o "llama3.2:3b" (mejor)
INSTRUCCIONES = (
    "Eres la voz de un cubo LED de 9x9x9 que se ilumina cuando hablas. "
    "Responde siempre en español, en una a tres frases cortas y naturales, "
    "como en una conversación. No uses listas, emojis, asteriscos ni formato."
)
MAX_HISTORIAL = 0           # respuestas anteriores que se le pasan al modelo (v5.9; antes 2).
                            # Con 0 solo va la pregunta anterior, para entender "¿y cuánto valen?".

# ---- Lo que sabe el cubo (v5) ----------------------------------------------
# Documentos en ../conocimiento/ (la tienda, Cali, preguntas frecuentes). Si esa
# carpeta no existe se usa ../conocimiento_ejemplo/. Ver ../conocimiento_ejemplo/LEEME.md
USAR_CONOCIMIENTO = True
MODELO_EMBEDDINGS = "bge-m3"    # para buscar en los documentos (entiende espanol)
FRAGMENTOS = 2                  # secciones que se le pasan al modelo en cada pregunta
UMBRAL_FRAGMENTO = 0.35         # parecido minimo para pasarle una seccion
MARGEN_FRAGMENTO = 0.15         # v5.6: la 2a seccion solo si puntua casi igual que la mejor
UMBRAL_SEGUIMIENTO = 0.5        # v5.10: si la pregunta sola puntua menos, se busca con la anterior
                                # (las que empiezan con "y" siempre)
UMBRAL_PREGUNTA_EXACTA = 0.88   # parecido para contestar tal cual una pregunta frecuente
NUM_CTX = 4096                  # ventana de contexto del modelo, en tokens
# v5.4, para responder mas rapido:
BUSCAR_PRODUCTOS = False        # True = busca cada producto con su precio (mas lento);
                                # False = solo el resumen por categoria
MAX_PALABRAS_RESPUESTA = 100    # tokens maximos de cada respuesta del modelo
TEMPERATURA = 0.5               # mas baja = respuestas mas seguras, inventa menos (v5.5)
# Lo que dice mientras el modelo piensa, para que no haya un silencio largo (v5.5).
# [] = no decir nada
FRASES_ESPERA = ["Déjame ver.", "Mmm, a ver.", "Un momento.", "Buena pregunta."]
# Palabras que Whisper debe escribir bien cuando esta atento (nombres de productos y lugares)
PISTA_WHISPER = ("Conversación en una tienda de artesanías en Cali: mochila wayúu, sombrero "
                 "vueltiao, champús, lulada, Cristo Rey, San Antonio, Petronio Álvarez.")

# ---- Texto a voz (Piper) ---------------------------------------------------
VOZ_PIPER = "voces/es_MX-claude-high.onnx"   # relativo a esta carpeta
