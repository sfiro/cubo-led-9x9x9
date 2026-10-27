# Pagina web minima para elegir la animacion desde el celular.
#   GET /              -> la pagina con los botones
#   GET /anim/<nombre> -> cambia la animacion ("demo", "apagar" o una de animaciones.LISTA)
#   GET /estado        -> {"actual": "..."}

import asyncio

import animaciones

_BOTONES = "".join(
    '<button onclick="ir(\'{0}\')" id="b-{0}">{1}</button>'.format(n, n.replace("_", " "))
    for n, _ in animaciones.LISTA)

_PAGINA = """<!doctype html>
<html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Cubo LED</title>
<style>
 body{font-family:system-ui,sans-serif;background:#111;color:#eee;margin:0;padding:16px}
 h1{font-size:1.4em;margin:0 0 4px} p{color:#999;margin:0 0 16px}
 .g{display:grid;grid-template-columns:repeat(auto-fill,minmax(130px,1fr));gap:8px}
 button{padding:14px 8px;font-size:1em;border:0;border-radius:10px;background:#2a2a2a;color:#eee}
 button.on{background:#1e88e5;color:#fff} .esp{background:#3a3a3a}
</style></head><body>
<h1>Cubo LED 9&times;9&times;9</h1><p id="e">Animaci&oacute;n: ...</p>
<div class="g">
 <button class="esp" onclick="ir('demo')" id="b-demo">demo (todas)</button>
 <button class="esp" onclick="ir('apagar')" id="b-apagar">apagar</button>
 %s
</div>
<script>
function marcar(n){document.querySelectorAll('button').forEach(b=>b.classList.toggle('on',b.id=='b-'+n));
 document.getElementById('e').textContent='Animación: '+n.replace('_',' ');}
function ir(n){fetch('/anim/'+n).then(()=>marcar(n));}
fetch('/estado').then(r=>r.json()).then(j=>marcar(j.actual));
</script></body></html>
""" % _BOTONES


async def _responder(escritor, estado, tipo, cuerpo):
    encabezado = "HTTP/1.0 {}\r\nContent-Type: {}\r\nConnection: close\r\n\r\n".format(estado, tipo)
    escritor.write(encabezado.encode())
    escritor.write(cuerpo.encode())
    await escritor.drain()


async def servir(control, puerto=80):
    async def atender(lector, escritor):
        try:
            linea = await lector.readline()
            while True:                         # descartar encabezados
                h = await lector.readline()
                if not h or h == b"\r\n":
                    break
            partes = linea.split()
            ruta = partes[1].decode() if len(partes) > 1 else "/"

            if ruta.startswith("/anim/"):
                if control.elegir(ruta[6:]):
                    await _responder(escritor, "200 OK", "text/plain", "ok")
                else:
                    await _responder(escritor, "404 Not Found", "text/plain", "no existe")
            elif ruta == "/estado":
                await _responder(escritor, "200 OK", "application/json",
                                 '{"actual": "%s"}' % control.actual)
            elif ruta == "/":
                await _responder(escritor, "200 OK", "text/html; charset=utf-8", _PAGINA)
            else:
                await _responder(escritor, "404 Not Found", "text/plain", "no existe")
        except Exception as e:
            print("web:", e)
        finally:
            escritor.close()
            await escritor.wait_closed()

    return await asyncio.start_server(atender, "0.0.0.0", puerto)
