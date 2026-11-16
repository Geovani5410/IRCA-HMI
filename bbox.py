# bbox.py
import trimesh, glob
for f in sorted(glob.glob('*.stl')):
    b = trimesh.load(f).bounds
    print(f"{f:28s} min={b[0].round(1)} max={b[1].round(1)} size={(b[1]-b[0]).round(1)}")

