import re, sys
f = 'index.html'
s = open(f, encoding='utf-8').read()

def rep(old, new):
    global s
    if s.count(old) != 1:
        print('NO ENCONTRADO:', old[:70]); sys.exit(1)
    s = s.replace(old, new)

# 1) Puertas ahora van ANTES del elevador (el carro ya no regresa)
rep(""" {n:'SISTEMA MECÁNICO',a:[],sec:3},
 {n:'PUERTAS',a:[5,6],sec:2,k:[H,[55,75,110],[55,60,125],[125,75,110],[125,60,125],H]},""",
""" {n:'PUERTAS',a:[5,6],sec:2,k:[H,[55,75,110],[55,60,125],[125,75,110],[125,60,125],H]},
 {n:'SISTEMA MECÁNICO',a:[],sec:3},""")

# 2) Posiciones del carro por etapa: arranca en los brazos 1 y 2 y solo avanza
rep("const POS=[0.06,SEC*.5,SEC*1.5,SEC*1.5,SEC*2.5,SEC*3.5,SEC*2.5];",
    "const POS=[SEC*.5,SEC*.5,SEC*1.5,SEC*1.5,SEC*2.5,SEC*2.5,SEC*3.5];")

# 3) Carro girado 180° y con entrada corta
rep("carro.add(giro);carro.position.set(-.2,.012,0);scene.add(carro);",
    "carro.add(giro);giro.rotation.y=Math.PI;carro.position.set(.11,.012,0);scene.add(carro);")

NEW = """function posCarro(){
  const from=idx===0?.11:POS[idx-1],to=POS[idx];
  const x=from+(to-from)*ease(Math.min(t/(D*.3),1));
  const y=idx===6?.05*ease(Math.min(Math.max((t/D-.3)/.4,0),1)):0;   // en el elevador sube y se queda arriba
  return [x,y];
}
"""
s, n = re.subn(r'function posCarro\(\)\{.*?\n\}\n', lambda m: NEW, s, count=1, flags=re.S)
if n != 1:
    print('NO ENCONTRADO: posCarro'); sys.exit(1)

open(f, 'w', encoding='utf-8').write(s)
print('Listo')
