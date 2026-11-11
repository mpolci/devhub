"""Build pcb/devhub-pcb.FCStd, the PCB reference linked by the enclosure (PcbRef).

The document is fully generated: rerun it after exporting a new STEP from KiCad.

    kicad-cli pcb export step --force --subst-models --user-origin 60.5x147.5mm \
        -o enclosure/pcb/devhub-pcb.step hardware/devhub/devhub.kicad_pcb
    FreeCAD enclosure/scripts/build_pcb_ref.py

STEP frame: X = u from the PCB left edge, Y = v from the front edge, Z = 0 on the PCB
bottom face. The enclosure link shifts it by -PcbThickness.

KiCad has no 3D model for K1 (Omron G5Q-1) and the Keystone 5000 test points: they are
replaced by boxes and cylinders with the datasheet envelope. The button caps (6 mm,
10 mm above the PCB top face) are not part of the PCB and are added the same way.
"""

import os

import FreeCAD as App
import Part

HERE = os.path.dirname(os.path.abspath(__file__))
PCB_DIR = os.path.join(os.path.dirname(HERE), "pcb")
STEP = os.path.join(PCB_DIR, "devhub-pcb.step")
OUT = os.path.join(PCB_DIR, "devhub-pcb.FCStd")
T = 1.6  # PCB top face in the STEP frame

V = App.Vector

PLACEHOLDERS = [
    # name, shape factory, colour
    # K1 body from the B.Fab outline (u 1.80..11.85, v 32.83..53.03), 15.8 mm max height.
    ("K1_G5Q", lambda: Part.makeBox(10.3, 20.3, 15.8, V(6.825 - 5.15, 42.93 - 10.15, -15.8)),
     (0.15, 0.15, 0.15)),
    # Keystone 5000-5004 miniature test points on the bottom face, envelope only.
    ("TP1_Keystone", lambda: Part.makeCylinder(1.6, 7.6, V(58.88, 97.34, -7.6)), (0.8, 0.1, 0.1)),
    ("TP10_Keystone", lambda: Part.makeCylinder(1.6, 7.6, V(22.20, 53.70, -7.6)), (0.1, 0.1, 0.1)),
]
for ref, x in (("SW1", 18.5), ("SW401", 46.0), ("SW301", 61.0), ("SW201", 76.0), ("SW101", 91.0)):
    PLACEHOLDERS.append((f"{ref}_Cap", (lambda x=x: Part.makeCylinder(3.0, 6.0, V(x, 14.5, T + 4.0))),
                         (0.1, 0.4, 0.8)))


def main():
    doc = App.newDocument("devhub_pcb")
    before = set(doc.Objects)
    if App.GuiUp:
        import ImportGui
        ImportGui.insert(STEP, doc.Name)
    else:
        import Import
        Import.insert(STEP, doc.Name)
    imported_roots = [o for o in doc.Objects if o not in before and not o.InList]

    root = doc.addObject("App::Part", "PCB")
    root.Label = "PCB"
    for o in imported_roots:
        root.addObject(o)
    group = doc.addObject("App::DocumentObjectGroup", "Placeholders")
    root.addObject(group)
    for name, make, colour in PLACEHOLDERS:
        f = doc.addObject("Part::Feature", name)
        f.Shape = make()
        group.addObject(f)
        if App.GuiUp:
            f.ViewObject.ShapeAppearance = [App.Material(DiffuseColor=colour)]
    doc.recompute()
    if os.path.exists(OUT):
        os.remove(OUT)
    doc.saveAs(OUT)


if App.GuiUp:
    import sys
    import traceback
    try:
        main()
        os._exit(0)
    except BaseException:
        traceback.print_exc()
        sys.stderr.flush()
        os._exit(1)
else:
    main()
