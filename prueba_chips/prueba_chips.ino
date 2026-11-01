// Prueba chip por chip de los 12 registros 74HC595 del cubo 9x9x9.
// Arduino MEGA o UNO. Monitor Serie a 9600 baudios, con "Nueva linea".
//
// Conexion:
//   MEGA: pin 51 -> DATA, pin 52 -> CLOCK, pin 2 -> LATCH  (igual que el sketch original)
//   UNO:  pin 11 -> DATA, pin 13 -> CLOCK, pin 2 -> LATCH
//   pin 8 -> SONDA: un cable que se apoya en el pin 9 (Q7') del chip a probar
//   GND del Arduino -> GND de la fuente del cubo
//
// Cadena: DATA entra al pin 14 de U1, el pin 9 (Q7') de U1 va al pin 14 de U2,
// y asi hasta U12. Se envia primero U12 y al final U1, MSB primero.
//   U1..U10 y U11 bit 0 = anodos (81 columnas)
//   U11 bits 1-7 = pisos 0-6, U12 bits 0-1 = pisos 7-8
//
// Comandos (escribirlos en el Monitor Serie y Enter):
//   s     SONDA: mide hasta que chip llega bien el corrimiento. Apoya el cable
//         del pin 8 en el pin 9 de U1 y escribe s; despues en U2, U3... U12.
//         El bit debe llegar tras 8, 16, 24... pulsos y el patron sin errores.
//   v N   VISUAL: enciende una por una las 8 salidas del chip N (1 a 12).
//         Salida de anodo: se enciende una columna vertical de 9 LEDs.
//         Salida de piso: se enciende un piso completo.
//   l N   LENTO: enciende Q0 del chip N enviando bit a bit, con latch tras
//         cada pulso, para ver el dato avanzar por la cadena hasta su lugar.
//   f N   FIJA: deja encendidas las 8 salidas del chip N para medirlas.
//   f N B solo la salida QB (0-7) del chip N. Queda fija hasta otro comando.
//   x     intercambia CLOCK y LATCH por software (por si los cables estan cruzados).
//   t     VISUAL de los 12 chips seguidos.
//   0     apaga todo.

#if defined(__AVR_ATmega2560__)
#define DATA_PIN  51
#define CLOCK_PIN 52
#else
#define DATA_PIN  11
#define CLOCK_PIN 13
#endif
#define LATCH_PIN 2
#define SONDA_PIN 8

#define PASO_MS 700           // tiempo de cada salida en la prueba visual

byte u[12];                   // u[0] = U1 ... u[11] = U12
int pinReloj = CLOCK_PIN;     // el comando x los intercambia
int pinLatch = LATCH_PIN;
unsigned int pulsos = 0;      // pulsos de reloj del ultimo envio

// noinline: sin esto el compilador mete todo dentro de loop() y falla el enlace
// con "relocation truncated to fit: R_AVR_7_PCREL" (bug conocido de avr-gcc con LTO)
void pruebaSonda() __attribute__((noinline));
void pruebaVisual(int chip) __attribute__((noinline));
void pruebaLenta(int chip) __attribute__((noinline));
void limpiarCadena() __attribute__((noinline));
void imprimirSecuencia() __attribute__((noinline));
void imprimirPulso(bool v, int chip, int b) __attribute__((noinline));
void salidaFija(int chip, int bit) __attribute__((noinline));

// ---- envio lento a mano (sin SPI) ------------------------------------------

void pulsoReloj() {
  pulsos++;
  delayMicroseconds(5);
  digitalWrite(pinReloj, HIGH);          // el 595 lee DATA en este flanco
  delayMicroseconds(5);
  digitalWrite(pinReloj, LOW);
}

void enviarBit(bool b) {
  digitalWrite(DATA_PIN, b);
  pulsoReloj();
}

void latch() {
  digitalWrite(pinLatch, HIGH);          // pasa el registro a las salidas
  delayMicroseconds(5);
  digitalWrite(pinLatch, LOW);
}

void enviar() {
  pulsos = 0;
  for (int c = 11; c >= 0; c--) {         // U12 primero, U1 al final
    for (int b = 7; b >= 0; b--) enviarBit(bitRead(u[c], b));
  }
  latch();
}

// Imprime los 12 bytes en el orden en que se envian (U12 primero, U1 al final),
// cada uno con el bit 7 (Q7) a la izquierda y el bit 0 (Q0) a la derecha.
void imprimirSecuencia() {
  Serial.print("   || pulsos=");
  Serial.print(pulsos);
  Serial.print(":");
  for (int c = 11; c >= 0; c--) {
    Serial.print(" U");
    Serial.print(c + 1);
    Serial.print("=");
    for (int b = 7; b >= 0; b--) Serial.print(bitRead(u[c], b));
  }
  Serial.println();
}

void apagarTodo() {
  for (int i = 0; i < 12; i++) u[i] = 0;
  enviar();
}

// Antes de cada comando: recorre toda la cadena con 96 ceros y hace latch,
// asi no queda ningun dato del comando anterior. Si despues de esto sigue
// algo encendido, ese chip (o el anterior) no esta desplazando o no hace latch.
void limpiarCadena() {
  for (int i = 0; i < 12; i++) u[i] = 0;
  pulsos = 0;
  for (int i = 0; i < 128; i++) enviarBit(0);   // 128 ceros: sobra aunque hubiera mas chips
  latch();
  Serial.print("Limpieza: 128 ceros y latch");
  imprimirSecuencia();
  delay(500);
}

// ---- sonda en Q7' -------------------------------------------------------------

// Patron de 32 bits con unos seguidos, ceros seguidos y alternados, para ver
// salidas pegadas en 0 o en 1 y pulsos de reloj dobles.
const byte PATRON[4] = {0xF0, 0xCC, 0xAA, 0x5A};

bool bitPatron(int i) {
  return bitRead(PATRON[i / 8], 7 - (i % 8));
}

void pruebaSonda() {
  Serial.println();
  Serial.println("== SONDA ==");

  // 1) llenar toda la cadena con ceros
  for (int i = 0; i < 120; i++) enviarBit(0);
  if (digitalRead(SONDA_PIN)) {
    Serial.println("La sonda marca 1 despues de llenar con ceros.");
    Serial.println("-> Esa salida Q7' esta pegada en 1, el chip anterior manda 1 siempre,");
    Serial.println("   o la sonda no esta en un pin 9 (revisa el cable del pin 8).");
    apagarTodo();
    return;
  }

  // 2) meter un solo 1 y contar los pulsos hasta que aparece en la sonda
  enviarBit(1);
  int pulsos = 1;
  digitalWrite(DATA_PIN, LOW);
  while (!digitalRead(SONDA_PIN) && pulsos < 120) {
    pulsoReloj();
    pulsos++;
  }
  if (pulsos >= 120) {
    Serial.println("El 1 nunca llego a la sonda (despues de 120 pulsos).");
    Serial.println("-> El corrimiento se corta ANTES de este chip, o en este chip.");
    Serial.println("   Prueba la sonda en el chip anterior para ubicar el corte.");
    Serial.println("   Revisa en el chip: pin 16 = 5 V, pin 8 = GND, pin 10 (MR) = 5 V,");
    Serial.println("   pin 11 (CLOCK) y pin 14 (DATA, desde el pin 9 del chip anterior).");
    apagarTodo();
    return;
  }

  Serial.print("El 1 llego despues de ");
  Serial.print(pulsos);
  Serial.println(" pulsos de reloj.");
  if (pulsos % 8 != 0) {
    Serial.println("-> NO es multiplo de 8: algun chip cuenta pulsos de mas o de menos.");
    Serial.println("   Suele ser ruido en CLOCK (rebotes): cable largo, GND debil, o un");
    Serial.println("   chip danado. Prueba la sonda en los chips anteriores.");
  } else {
    Serial.print("-> La sonda esta en el pin 9 de U");
    Serial.print(pulsos / 8);
    Serial.println(". El 1 recorrio bien todos los chips hasta aqui.");
  }

  // 3) patron de 32 bits: cada bit debe salir en la sonda 'pulsos' pulsos despues
  for (int i = 0; i < 120; i++) enviarBit(0);
  int errores = 0;
  for (int j = 1; j < pulsos + 32; j++) {
    enviarBit(j - 1 < 32 ? bitPatron(j - 1) : 0);
    if (j >= pulsos) {
      bool esperado = bitPatron(j - pulsos);
      if (digitalRead(SONDA_PIN) != esperado) errores++;
    }
  }
  Serial.print("Patron de 32 bits: ");
  if (errores == 0) {
    Serial.println("llego PERFECTO.");
    Serial.println("-> Los chips hasta aqui desplazan bien. Mueve la sonda al siguiente.");
  } else {
    Serial.print(errores);
    Serial.println(" bits equivocados.");
    Serial.println("-> El corrimiento falla a ratos: soldadura floja, chip danado o ruido.");
  }
  apagarTodo();
}

// ---- prueba visual --------------------------------------------------------------

bool esAnodo(int chip, int b) {           // chip de 1 a 12
  return chip <= 10 || (chip == 11 && b == 0);
}

void pruebaVisual(int chip) {
  Serial.println();
  Serial.print("== VISUAL U");
  Serial.print(chip);
  Serial.println(" ==");
  for (int b = 0; b < 8; b++) {
    if (esAnodo(chip, b)) {
      // todos los pisos encendidos y una sola columna
      for (int i = 0; i < 10; i++) u[i] = 0;
      u[10] = 0xFE;                       // pisos 0-6
      u[11] = 0x03;                       // pisos 7-8
      bitSet(u[chip - 1], b);
      int p = (chip - 1) * 8 + b;
      Serial.print("  U"); Serial.print(chip);
      Serial.print(" bit "); Serial.print(b);
      Serial.print(" (Q"); Serial.print(b);
      Serial.print(") -> columna "); Serial.print(p);
      Serial.print(": fila "); Serial.print(p / 9);
      Serial.print(", columna "); Serial.print(p % 9);
      Serial.print(" (9 LEDs en vertical)");
    } else {
      // todos los anodos encendidos y un solo piso
      for (int i = 0; i < 10; i++) u[i] = 0xFF;
      u[10] = 0x01;
      u[11] = 0x00;
      bitSet(u[chip - 1], b);
      int piso = (chip == 11) ? b - 1 : b + 7;
      Serial.print("  U"); Serial.print(chip);
      Serial.print(" bit "); Serial.print(b);
      Serial.print(" (Q"); Serial.print(b);
      if (piso <= 8) {
        Serial.print(") -> PISO "); Serial.print(piso);
        Serial.print(" completo");
      } else {
        Serial.print(") -> sin uso, deberia estar APAGADO");
      }
    }
    enviar();
    imprimirSecuencia();
    delay(PASO_MS);
  }
  apagarTodo();
}

// ---- modo lento -----------------------------------------------------------------

void imprimirPulso(bool v, int chip, int b) {
  Serial.print("  pulso ");
  Serial.print(pulsos);
  Serial.print(": DATA=");
  Serial.print(v);
  Serial.print("  (destino final U");
  Serial.print(chip);
  Serial.print(" Q");
  Serial.print(b);
  Serial.println(")");
}

// Enciende la salida Q0 del chip N, pero enviando bit por bit con LATCH despues
// de CADA pulso: en el cubo se ve como el dato avanza por la cadena hasta su lugar.
// Cada linea dice que bit se envio y en que registro debe terminar.
void pruebaLenta(int chip) {
  Serial.println();
  Serial.print("== LENTO: Q0 de U");
  Serial.print(chip);
  Serial.println(", latch despues de cada pulso ==");
  for (int i = 0; i < 12; i++) u[i] = 0;
  if (esAnodo(chip, 0)) {
    u[10] = 0xFE;                         // pisos encendidos
    u[11] = 0x03;
  } else {
    for (int i = 0; i < 10; i++) u[i] = 0xFF;   // anodos encendidos
    u[10] = 0x01;
  }
  bitSet(u[chip - 1], 0);

  pulsos = 0;
  for (int c = 11; c >= 0; c--) {
    for (int b = 7; b >= 0; b--) {
      bool v = bitRead(u[c], b);
      enviarBit(v);
      latch();
      imprimirPulso(v, c + 1, b);
      delay(150);
    }
  }
  Serial.print("Fin: debe quedar encendida solo la salida Q0 de U");
  Serial.print(chip);
  imprimirSecuencia();
}

// ---- salida fija para medir ----------------------------------------------------------

const byte PIN_Q[8] = {15, 1, 2, 3, 4, 5, 6, 7};   // pin del 74HC595 de Q0..Q7

// Deja encendida (fija) la salida Q'bit' del chip N, o sus 8 salidas si bit < 0,
// hasta el siguiente comando. Sirve para medir con el multimetro.
void salidaFija(int chip, int bit) {
  byte mascara = (bit < 0) ? 0xFF : (1 << bit);
  for (int i = 0; i < 12; i++) u[i] = 0;
  if (chip <= 10) {
    u[10] = 0xFE;                         // pisos encendidos
    u[11] = 0x03;
  } else {
    for (int i = 0; i < 10; i++) u[i] = 0xFF;   // anodos encendidos
    u[10] = 0x01;
  }
  u[chip - 1] |= mascara;
  enviar();
  Serial.print("Fija: U");
  Serial.print(chip);
  imprimirSecuencia();
  Serial.print("  Mide en U");
  Serial.print(chip);
  Serial.println(" contra GND (pin 8). Deben marcar 5 V:");
  for (int b = 0; b < 8; b++) {
    if (!bitRead(mascara, b)) continue;
    Serial.print("    Q");
    Serial.print(b);
    Serial.print(" = pin ");
    Serial.println(PIN_Q[b]);
  }
  Serial.println("  y las demas salidas 0 V. Pin 9 (Q7') = 5 V solo si Q7 esta en 1.");
}

// ---- programa -------------------------------------------------------------------

void menu() {
  Serial.println();
  Serial.println("Comandos: s = sonda | v N = visual del chip N (1-12) | l N = lento | f N [B] = salida fija | x = cruzar CLOCK/LATCH | t = todos | 0 = apagar");
}

void setup() {
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(SONDA_PIN, INPUT);
  Serial.begin(9600);
  apagarTodo();
  Serial.println("Prueba de los 74HC595 del cubo 9x9x9");
  Serial.print("CLOCK = pin ");
  Serial.print(pinReloj);
  Serial.print(", LATCH = pin ");
  Serial.println(pinLatch);
  menu();
}

void loop() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toLowerCase();
  if (cmd.length() == 0) return;

  Serial.println();
  limpiarCadena();
  if (cmd == "s") {
    pruebaSonda();
  } else if (cmd == "t") {
    for (int c = 1; c <= 12; c++) pruebaVisual(c);
  } else if (cmd == "0") {
    apagarTodo();
    Serial.println("Apagado");
  } else if (cmd == "x") {
    int t = pinReloj; pinReloj = pinLatch; pinLatch = t;
    digitalWrite(pinReloj, LOW);
    digitalWrite(pinLatch, LOW);
    limpiarCadena();
    Serial.print("Intercambiados: CLOCK = pin ");
    Serial.print(pinReloj);
    Serial.print(", LATCH = pin ");
    Serial.println(pinLatch);
  } else if (cmd.charAt(0) == 'f') {
    int chip = 0, bit = -1;
    sscanf(cmd.c_str() + 1, "%d %d", &chip, &bit);
    if (chip >= 1 && chip <= 12 && bit <= 7) salidaFija(chip, bit);
    else Serial.println("Uso: f N (8 salidas del chip N) o f N B (solo la salida QB)");
  } else if (cmd.charAt(0) == 'l') {
    int chip = cmd.substring(1).toInt();
    if (chip >= 1 && chip <= 12) pruebaLenta(chip);
    else Serial.println("Chip invalido: usa l 1 ... l 12");
  } else if (cmd.charAt(0) == 'v') {
    int chip = cmd.substring(1).toInt();
    if (chip >= 1 && chip <= 12) pruebaVisual(chip);
    else Serial.println("Chip invalido: usa v 1 ... v 12");
  } else {
    Serial.println("Comando desconocido");
  }
  menu();
}
