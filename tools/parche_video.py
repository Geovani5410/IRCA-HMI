# Ejecutar desde la raiz del repo:  python3 tools/parche_video.py
# Adapta frontend/index.html a la nueva estructura y conecta video + servidor:
#   - los .stl se cargan desde assets/
#   - el video de la camara (MediaMTX/WebRTC) se muestra en un <iframe>
#   - el HMI se conecta solo a ws://<host>/ws del servidor y usa el estado del serial
import sys
f = 'frontend/index.html'
s = open(f, encoding='utf-8').read()

def rep(old, new):
    global s
    if s.count(old) != 1:
        print('NO ENCONTRADO:', old[:70]); sys.exit(1)
    s = s.replace(old, new)

# 1) STL desde assets/
rep("encodeURI('toyota corolla.STL')", "encodeURI('assets/toyota corolla.STL')")
rep("ld.load(n+'.stl',", "ld.load('assets/'+n+'.stl',")

# NOTA: solo se activa en http:// (servidor de la RPi). En https:// (GitHub Pages) el navegador
#       bloquea ws:// y http:// (contenido mixto), asi que alli queda en modo demo.
# 2) Video: por defecto http://<host>:8889/cam (MediaMTX en la RPi). ?cam=URL lo cambia, ?cam= lo apaga.
rep("""if(q.get('cam'))$('camBox').innerHTML=`<img src="${q.get('cam')}">`;""",
"""const CAM=q.get('cam')??(location.protocol==='http:'?`http://${location.hostname}:8889/cam`:null);
if(CAM)$('camBox').innerHTML=`<iframe src="${CAM}${CAM.includes('?')?'&':'?'}controls=false&muted=true&autoplay=true" allow="autoplay;fullscreen" style="width:100%;height:100%;border:0"></iframe>`;""")

# 3) WebSocket: por defecto ws://<host>/ws del server.py (?ws=URL lo cambia, ?demo lo desactiva)
rep("if(q.get('ws')){const conn=()=>{const w=new WebSocket(q.get('ws'));",
"""const WS=q.get('ws')??(location.protocol==='http:'&&!q.has('demo')?`ws://${location.host}/ws`:null);
if(WS){const conn=()=>{const w=new WebSocket(WS);""")
rep("chip('chEsp','ESP32: conectado','ok');demo=false;log('rosbridge conectado')};",
    "chip('chEsp','ESP32: esperando','w');log('Servidor conectado')};")
rep("w.onmessage=e=>{const m=JSON.parse(e.data),n=+(m.topic||'').match(/brazo_(\\d)/)?.[1];",
"""w.onmessage=e=>{const m=JSON.parse(e.data);
    if(m.topic==='/estado'){const ok=!!m.msg?.serial;demo=!ok;chip('chEsp',ok?'ESP32: conectado':'ESP32: sin serial',ok?'ok':'bad');log(ok?'ESP32 conectada':'ESP32 sin conexión serial',ok?'':'w');return}
    const n=+(m.topic||'').match(/brazo_(\\d)/)?.[1];""")

open(f, 'w', encoding='utf-8').write(s)
print('Listo')
