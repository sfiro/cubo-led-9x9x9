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
VOZ_SOBRE_ANIMACION = True  # al escuchar/pensar/hablar se ve la voz, y luego vuelve la animacion.
                            # False: la animacion elegida sigue aunque el asistente hable

# ---- Audio ------------------------------------------------------------------
# None = el dispositivo predeterminado. Si no, el numero o parte del nombre
# que muestra "python -m sounddevice"
MICROFONO = None
PARLANTE = None

# ---- Escuchar ---------------------------------------------------------------
# "voz": escucha siempre y responde a cualquier frase.
# "palabra": espera la palabra de activacion (necesita openwakeword instalado).
ACTIVACION = "voz"
PALABRA_ACTIVACION = "hey_jarvis"   # modelos que trae openWakeWord (en ingles)
UMBRAL_VOZ = 0.02           # volumen (RMS) desde el que se considera que alguien habla.
                            # Medirlo con: python medir_microfono.py
SILENCIO_FIN_S = 0.9        # silencio que marca el fin de la pregunta
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
MAX_HISTORIAL = 6           # mensajes anteriores que el modelo recuerda

# ---- Texto a voz (Piper) ---------------------------------------------------
VOZ_PIPER = "voces/es_MX-claude-high.onnx"   # relativo a esta carpeta
