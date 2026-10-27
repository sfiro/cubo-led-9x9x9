// Web Worker: corre MicroPython (WebAssembly) con las animaciones reales de
// pico_w/animaciones.py. Si una animacion se cuelga, la pagina sigue viva y
// puede reiniciar este worker.

import { loadMicroPython } from "https://cdn.jsdelivr.net/npm/@micropython/micropython-webassembly-pyscript@1.27.0/micropython.mjs";

// Archivos que se copian al sistema de archivos de MicroPython (/lib).
const ARCHIVOS = {
  "animaciones.py": "../pico_w/animaciones.py",
  "fuente.py": "../pico_w/fuente.py",
  "cubo.py": "cubo_sim.py",
  "sim_runner.py": "sim_runner.py",
};

// Estado de los controles; la pagina lo actualiza con mensajes.
const estado = { pausado: false, velocidad: 1, repetir: true, pasos: 0, arg: "" };

let mp = null;
let cola = Promise.resolve();

const enviar = (m) => postMessage(m);

// Modulo "sim" que ve Python.
const sim = {
  mostrar: (vox, fuera) => enviar({ tipo: "cuadro", vox, fuera }),
  calculo: (ms, t) => enviar({ tipo: "calculo", ms, t }),
  pausado: () => estado.pausado,
  velocidad: () => estado.velocidad,
  repetir: () => estado.repetir,
  tomar_paso: () => {
    if (estado.pasos > 0) { estado.pasos--; return true; }
    return false;
  },
  terminado: () => enviar({ tipo: "terminado" }),
  fallo: () => enviar({ tipo: "fallo" }),
  arg: () => estado.arg,
  listo: (nombres) => enviar({ tipo: "listo", animaciones: String(nombres).split(",") }),
};

async function iniciar() {
  mp = await loadMicroPython({
    stdout: (texto) => enviar({ tipo: "salida", texto }),
    stderr: (texto) => enviar({ tipo: "salida", texto, error: true }),
  });
  mp.registerJsModule("sim", sim);
  try { mp.FS.mkdir("/lib"); } catch (e) { /* ya existe */ }
  for (const [destino, origen] of Object.entries(ARCHIVOS)) {
    const r = await fetch(new URL(origen, import.meta.url), { cache: "no-cache" });
    if (!r.ok) throw new Error(`No pude cargar ${origen} (${r.status})`);
    mp.FS.writeFile("/lib/" + destino, await r.text());
  }
  await mp.runPythonAsync(
    "import sys\nif '/lib' not in sys.path: sys.path.append('/lib')\n" +
    "import sim\nfrom sim_runner import runner\nsim.listo(runner.nombres())");
}

// Ejecuta codigo Python en orden; el argumento viaja por sim.arg() para no
// tener que meter texto del usuario dentro del codigo.
function python(codigo, arg = "") {
  cola = cola.then(async () => {
    estado.arg = arg;
    try {
      await mp.runPythonAsync(codigo);
    } catch (e) {
      enviar({ tipo: "salida", texto: String(e.message || e), error: true });
      enviar({ tipo: "fallo" });
    }
  });
  return cola;
}

onmessage = (ev) => {
  const m = ev.data;
  switch (m.tipo) {
    case "control":
      Object.assign(estado, { pausado: m.pausado, velocidad: m.velocidad, repetir: m.repetir });
      break;
    case "paso":
      estado.pasos++;
      break;
    case "elegir":
      python("import sim\nrunner.elegir(sim.arg())", m.nombre);
      break;
    case "editor":
      mp.FS.writeFile("/lib/editor_anim.py", m.codigo);
      python("import sim\nrunner.editor(sim.arg())", m.funcion);
      break;
    case "detener":
      python("runner.detener()");
      break;
  }
};

// Latido para que la pagina sepa si Python se colgo (un bucle sin await
// bloquea el worker y deja de responder).
setInterval(() => enviar({ tipo: "latido" }), 500);

iniciar().catch((e) => {
  enviar({ tipo: "salida", texto: "Error al iniciar MicroPython: " + (e.message || e), error: true });
  enviar({ tipo: "error_inicio" });
});
