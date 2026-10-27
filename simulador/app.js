// Simulador del cubo LED 9x9x9: visor 3D (Three.js) + controles + editor.
// Python corre en worker.js; aqui solo se dibuja lo que manda.

import * as THREE from "three";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";
import { EffectComposer } from "three/addons/postprocessing/EffectComposer.js";
import { RenderPass } from "three/addons/postprocessing/RenderPass.js";
import { UnrealBloomPass } from "three/addons/postprocessing/UnrealBloomPass.js";
import { OutputPass } from "three/addons/postprocessing/OutputPass.js";

const N = 9;
const TOTAL = N * N * N;
const VELOCIDADES = [0.1, 0.25, 0.5, 1, 2, 4, 8];

const $ = (id) => document.getElementById(id);

// ---------------------------------------------------------------- escena 3D

const lienzo = $("lienzo");
const renderer = new THREE.WebGLRenderer({ canvas: lienzo, antialias: true });
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
renderer.setClearColor(0x0b0d12);

const escena = new THREE.Scene();
escena.fog = new THREE.Fog(0x0b0d12, 22, 40);
const camara = new THREE.PerspectiveCamera(42, 1, 0.1, 100);
camara.position.set(11.5, 8.5, 13.5);

const controles = new OrbitControls(camara, lienzo);
controles.enableDamping = true;
controles.minDistance = 7;
controles.maxDistance = 32;

// LED (z, y, x) -> posicion en la escena: x a la derecha, z (piso) hacia arriba, y hacia el fondo.
const centro = (N - 1) / 2;
const posicion = (z, y, x) => new THREE.Vector3(x - centro, z - centro, centro - y);

const geoLed = new THREE.SphereGeometry(0.15, 16, 12);
const matLed = new THREE.MeshBasicMaterial({ color: 0xffffff });
const leds = new THREE.InstancedMesh(geoLed, matLed, TOTAL);
const matriz = new THREE.Matrix4();
const escalaOn = new THREE.Vector3(1.25, 1.25, 1.25);
const escalaOff = new THREE.Vector3(1, 1, 1);
const sinRotacion = new THREE.Quaternion();
const colorOff = new THREE.Color(0x232833);
let colorOn = new THREE.Color($("color").value);
const encendidos = new Uint8Array(TOTAL);

for (let z = 0; z < N; z++)
  for (let y = 0; y < N; y++)
    for (let x = 0; x < N; x++) {
      const i = z * 81 + y * 9 + x;
      matriz.compose(posicion(z, y, x), sinRotacion, escalaOff);
      leds.setMatrixAt(i, matriz);
      leds.setColorAt(i, colorOff);
    }
escena.add(leds);

// Alambres: columnas verticales (anodos) y filas de cada piso (catodos).
const alambres = (() => {
  const puntos = [];
  const a = -centro, b = centro;
  for (let y = 0; y < N; y++)
    for (let x = 0; x < N; x++) {
      puntos.push(x - centro, a - 0.6, centro - y, x - centro, b, centro - y);
    }
  for (let z = 0; z < N; z++)
    for (let y = 0; y < N; y++) {
      puntos.push(a, z - centro, centro - y, b, z - centro, centro - y);
    }
  const geo = new THREE.BufferGeometry();
  geo.setAttribute("position", new THREE.Float32BufferAttribute(puntos, 3));
  return new THREE.LineSegments(geo, new THREE.LineBasicMaterial({ color: 0x343b4b, transparent: true, opacity: 0.55 }));
})();
escena.add(alambres);

// Base
const base = new THREE.Mesh(new THREE.BoxGeometry(N + 1.2, 0.3, N + 1.2),
  new THREE.MeshBasicMaterial({ color: 0x151922 }));
base.position.y = -centro - 0.85;
escena.add(base);

// Ejes desde el LED (0,0,0)
const origen = posicion(0, 0, 0).add(new THREE.Vector3(-0.9, -0.7, 0.9));
for (const [dir, color] of [[[1, 0, 0], 0xff6b6b], [[0, 0, -1], 0x4d8dff], [[0, 1, 0], 0x5ad19a]]) {
  escena.add(new THREE.ArrowHelper(new THREE.Vector3(...dir), origen, 2.2, color, 0.4, 0.22));
}

// Resplandor
const composer = new EffectComposer(renderer);
composer.addPass(new RenderPass(escena, camara));
const bloom = new UnrealBloomPass(new THREE.Vector2(1, 1), 1.1, 0.45, 0.2);
composer.addPass(bloom);
composer.addPass(new OutputPass());

function ajustarTamano() {
  const { clientWidth: w, clientHeight: h } = lienzo.parentElement;
  renderer.setSize(w, h, false);
  composer.setSize(w, h);
  camara.aspect = w / h;
  // Alejar la camara en pantallas angostas para que el cubo entre completo.
  const distancia = 20 * Math.max(1, 1.15 / camara.aspect);
  camara.position.setLength(distancia);
  camara.updateProjectionMatrix();
}
new ResizeObserver(ajustarTamano).observe(lienzo.parentElement);
ajustarTamano();

function dibujarCuadro(vox) {
  let total = 0;
  for (let i = 0; i < TOTAL; i++) {
    const on = vox.charCodeAt(2 * i + 1) !== 48 ? 1 : 0;     // "00" apagado, "01" encendido
    total += on;
    if (on !== encendidos[i]) {
      encendidos[i] = on;
      leds.getMatrixAt(i, matriz);
      const p = new THREE.Vector3().setFromMatrixPosition(matriz);
      matriz.compose(p, sinRotacion, on ? escalaOn : escalaOff);
      leds.setMatrixAt(i, matriz);
      leds.setColorAt(i, on ? colorOn : colorOff);
    }
  }
  leds.instanceMatrix.needsUpdate = true;
  leds.instanceColor.needsUpdate = true;
  return total;
}

function recolorear() {
  for (let i = 0; i < TOTAL; i++) leds.setColorAt(i, encendidos[i] ? colorOn : colorOff);
  leds.instanceColor.needsUpdate = true;
}

// Coordenadas del LED bajo el mouse
const rayo = new THREE.Raycaster();
const mouse = new THREE.Vector2();
const tip = $("tip");
lienzo.addEventListener("pointermove", (e) => {
  const r = lienzo.getBoundingClientRect();
  mouse.set(((e.clientX - r.left) / r.width) * 2 - 1, -((e.clientY - r.top) / r.height) * 2 + 1);
  rayo.setFromCamera(mouse, camara);
  const hit = rayo.intersectObject(leds)[0];
  if (hit && e.pointerType === "mouse") {
    const i = hit.instanceId;
    const z = Math.floor(i / 81), y = Math.floor((i % 81) / 9), x = i % 9;
    tip.textContent = `c.led(${z}, ${y}, ${x})  ${encendidos[i] ? "encendido" : "apagado"}`;
    tip.style.left = e.clientX - r.left + 14 + "px";
    tip.style.top = e.clientY - r.top + 14 + "px";
    tip.style.display = "block";
  } else {
    tip.style.display = "none";
  }
});
lienzo.addEventListener("pointerleave", () => (tip.style.display = "none"));

// Vistas fijas: frente mira el plano x-z (columnas y pisos), lado el plano y-z.
const VISTAS = {
  "3d": [11.5, 8.5, 13.5],
  frente: [0, 0, 1],
  lado: [1, 0, 0],
  arriba: [0, 1, 0.0001],
};
document.querySelectorAll("[data-vista]").forEach((b) => {
  b.onclick = () => {
    const d = camara.position.length();
    camara.position.set(...VISTAS[b.dataset.vista]).setLength(d);
    controles.target.set(0, 0, 0);
  };
});

function animar() {
  controles.update();
  if ($("brillo").checked) composer.render();
  else renderer.render(escena, camara);
  requestAnimationFrame(animar);
}
animar();

// ---------------------------------------------------------------- Python

const ui = {
  pausado: false,
  velocidad: 1,
  ultimo: null,           // { tipo: "anim", nombre } o { tipo: "editor", codigo, funcion }
  corriendo: false,
  ultimoLatido: performance.now(),
  cuadros: 0,
};

let worker = null;

function crearWorker() {
  worker = new Worker(new URL("worker.js", import.meta.url), { type: "module" });
  worker.onmessage = (ev) => recibir(ev.data);
  worker.onerror = (e) => {
    consola("Error en el worker: " + (e.message || "desconocido"), "error");
    ponerEstado("error", "Error al cargar");
  };
  ui.ultimoLatido = performance.now();
  enviarControl();
}

function enviarControl() {
  worker?.postMessage({ tipo: "control", pausado: ui.pausado, velocidad: ui.velocidad, repetir: $("repetir").checked });
}

function recibir(m) {
  switch (m.tipo) {
    case "listo":
      $("carga").style.display = "none";
      armarLista(m.animaciones);
      $("btn-ejecutar").disabled = false;
      ponerEstado("", "Listo — elige una animación");
      consola("MicroPython listo. Animaciones cargadas desde pico_w/animaciones.py", "info");
      if (ui.ultimo) relanzar();
      break;
    case "cuadro": {
      const total = dibujarCuadro(m.vox);
      ui.cuadros++;
      $("d-leds").textContent = total;
      $("d-fuera").textContent = m.fuera;
      $("caja-fuera").classList.toggle("aviso", m.fuera > 0);
      break;
    }
    case "calculo":
      $("d-calc").textContent = m.ms + " ms";
      $("caja-calc").classList.toggle("aviso", m.ms > 30);
      $("d-tiempo").textContent = (m.t / 1000).toFixed(1) + " s";
      break;
    case "salida":
      consola(m.texto, m.error ? "error" : "");
      break;
    case "terminado":
      ui.corriendo = false;
      ponerEstado("", "Terminó");
      break;
    case "fallo":
      ui.corriendo = false;
      ponerEstado("error", "Error — revisa la consola");
      break;
    case "error_inicio":
      ponerEstado("error", "No se pudo cargar MicroPython");
      $("carga").textContent = "No se pudo cargar MicroPython. Revisa la consola.";
      break;
    case "latido":
      ui.ultimoLatido = performance.now();
      $("alerta").style.display = "none";
      break;
  }
}

// Si Python deja de responder (bucle sin await), avisar.
setInterval(() => {
  if (ui.corriendo && performance.now() - ui.ultimoLatido > 3000) {
    $("alerta").style.display = "block";
    ponerEstado("error", "Python no responde");
  }
}, 1000);

setInterval(() => {
  $("d-fps").textContent = ui.cuadros;
  ui.cuadros = 0;
}, 1000);

$("btn-reiniciar-py").onclick = () => {
  worker.terminate();
  $("alerta").style.display = "none";
  ui.corriendo = false;
  consola("Python reiniciado.", "info");
  ponerEstado("", "Reiniciando MicroPython…");
  ui.ultimo = null;
  crearWorker();
};

// ---------------------------------------------------------------- controles

function armarLista(nombres) {
  const lista = $("lista");
  lista.querySelectorAll("[data-anim]:not([data-anim=demo])").forEach((b) => b.remove());
  for (const n of nombres) {
    const b = document.createElement("button");
    b.className = "btn";
    b.dataset.anim = n;
    b.textContent = n.replaceAll("_", " ");
    lista.appendChild(b);
  }
  lista.querySelectorAll("[data-anim]").forEach((b) => {
    b.disabled = false;
    b.onclick = () => lanzar({ tipo: "anim", nombre: b.dataset.anim });
  });
}

function marcarActivo() {
  document.querySelectorAll("[data-anim]").forEach((b) =>
    b.classList.toggle("activo", ui.ultimo?.tipo === "anim" && b.dataset.anim === ui.ultimo.nombre));
}

function lanzar(orden) {
  ui.ultimo = orden;
  ui.corriendo = true;
  $("alerta").style.display = "none";
  if (orden.tipo === "anim") worker.postMessage({ tipo: "elegir", nombre: orden.nombre });
  else worker.postMessage({ tipo: "editor", codigo: orden.codigo, funcion: orden.funcion });
  marcarActivo();
  ponerEstado(ui.pausado ? "pausa" : "ok",
    (ui.pausado ? "En pausa: " : "") + (orden.tipo === "anim" ? orden.nombre.replaceAll("_", " ") : "editor: " + orden.funcion));
}

function relanzar() {
  if (ui.ultimo) lanzar(ui.ultimo);
}

function ponerEstado(clase, texto) {
  $("punto").className = "punto " + clase;
  $("estado-texto").textContent = texto;
}

$("btn-pausa").onclick = () => {
  ui.pausado = !ui.pausado;
  $("btn-pausa").textContent = ui.pausado ? "▶ Seguir" : "⏸ Pausa";
  $("btn-paso").disabled = !ui.pausado;
  enviarControl();
  if (ui.corriendo) {
    const t = $("estado-texto").textContent.replace(/^En pausa: /, "");
    ponerEstado(ui.pausado ? "pausa" : "ok", (ui.pausado ? "En pausa: " : "") + t);
  }
};
$("btn-paso").onclick = () => worker.postMessage({ tipo: "paso" });
$("btn-reiniciar").onclick = relanzar;
$("btn-detener").onclick = () => {
  worker.postMessage({ tipo: "detener" });
  ui.corriendo = false;
  ponerEstado("", "Detenido");
};
$("velocidad").oninput = (e) => {
  ui.velocidad = VELOCIDADES[+e.target.value];
  $("velocidad-txt").textContent = ui.velocidad + "×";
  enviarControl();
};
$("repetir").onchange = enviarControl;
$("estructura").onchange = (e) => (alambres.visible = base.visible = e.target.checked);
$("color").oninput = (e) => {
  colorOn = new THREE.Color(e.target.value);
  recolorear();
};

// ---------------------------------------------------------------- editor

const EJEMPLO = `# Escribe tu animacion y presiona "Ejecutar".
# Puedes usar lo mismo que en pico_w/animaciones.py:
#   c.led(z, y, x, 1)   c.apagar()   c.encender()   c.plano(eje, k, 1)
#   await _paso(c, ms)  -> muestra el cuadro y espera ms milisegundos
import math
from animaciones import _paso, _r, _durante, N, M


async def espiral(c):
    # Un punto que sube girando y deja una estela
    for vuelta in range(3):
        c.apagar()
        for z in range(N):
            for k in range(12):
                a = (z * 12 + k) * math.pi / 6
                x = int(4.5 + 3.6 * math.cos(a))
                y = int(4.5 + 3.6 * math.sin(a))
                c.led(z, y, x, 1)
                await _paso(c, 20)


async def lluvia_inversa(c, duracion=6000):
    # Burbujas que suben desde el piso
    sigue = _durante(duracion)
    while sigue():
        burbujas = [(_r(), _r()) for _ in range(5)]
        for z in range(N):
            c.apagar()
            for y, x in burbujas:
                c.led(z, y, x, 1)
            await _paso(c, 40)
`;

const codigo = $("codigo");
const guardar = (clave, valor) => { try { localStorage.setItem(clave, valor); } catch (e) { /* sin almacenamiento */ } };
const leer = (clave) => { try { return localStorage.getItem(clave); } catch (e) { return null; } };

codigo.value = leer("cubo-editor") ?? EJEMPLO;

function actualizarFunciones() {
  const sel = $("funcion");
  const previa = sel.value || leer("cubo-funcion");
  const nombres = [...codigo.value.matchAll(/^async\s+def\s+(\w+)\s*\(/gm)].map((m) => m[1]);
  sel.innerHTML = "";
  for (const n of nombres) sel.add(new Option(n, n));
  if (nombres.includes(previa)) sel.value = previa;
  if (!nombres.length) sel.add(new Option("(define async def nombre(c))", ""));
}
actualizarFunciones();

let temporizador;
codigo.addEventListener("input", () => {
  clearTimeout(temporizador);
  temporizador = setTimeout(() => {
    guardar("cubo-editor", codigo.value);
    actualizarFunciones();
  }, 300);
});

// Tab inserta 4 espacios; Ctrl/Cmd+Enter ejecuta.
codigo.addEventListener("keydown", (e) => {
  if (e.key === "Tab" && !e.shiftKey) {
    e.preventDefault();
    const { selectionStart: a, selectionEnd: b } = codigo;
    codigo.setRangeText("    ", a, b, "end");
    codigo.dispatchEvent(new Event("input"));
  } else if (e.key === "Enter" && (e.metaKey || e.ctrlKey)) {
    e.preventDefault();
    ejecutarEditor();
  }
});

function ejecutarEditor() {
  const funcion = $("funcion").value;
  if (!funcion) {
    consola("El editor necesita al menos una función 'async def nombre(c):'.", "error");
    return;
  }
  guardar("cubo-editor", codigo.value);
  guardar("cubo-funcion", funcion);
  consola(`Ejecutando ${funcion}() del editor…`, "info");
  lanzar({ tipo: "editor", codigo: codigo.value, funcion });
}

$("btn-ejecutar").onclick = ejecutarEditor;
$("funcion").onchange = (e) => guardar("cubo-funcion", e.target.value);
$("btn-plantilla").onclick = () => {
  if (codigo.value !== EJEMPLO && !confirm("¿Reemplazar el código del editor por el ejemplo?")) return;
  codigo.value = EJEMPLO;
  codigo.dispatchEvent(new Event("input"));
};
$("btn-copiar").onclick = async () => {
  try {
    await navigator.clipboard.writeText(codigo.value);
    consola("Código copiado al portapapeles.", "info");
  } catch (e) {
    codigo.select();
    consola("No pude copiar automáticamente: el código quedó seleccionado, usa Ctrl/Cmd+C.", "error");
  }
};

// ---------------------------------------------------------------- consola

function consola(texto, clase = "") {
  const c = $("consola");
  const linea = document.createElement("div");
  if (clase) linea.className = clase;
  linea.textContent = texto;
  c.appendChild(linea);
  while (c.childElementCount > 400) c.firstChild.remove();
  c.scrollTop = c.scrollHeight;
}
$("btn-limpiar").onclick = () => ($("consola").textContent = "");

crearWorker();
