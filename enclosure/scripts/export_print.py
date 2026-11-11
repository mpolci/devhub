"""Export the printable bodies as STL files, oriented for printing (run with freecadcmd).

    freecadcmd -c "exec(open('enclosure/scripts/export_print.py').read())"

- lid.stl and light-pipes.stl: the lid upside down, top face on the bed; the light pipes
  get the same transform, so they stay in their holes when Bambu Studio puts both files
  in one object as two parts.
- bottom-plate.stl: outer face on the bed, as modelled.
Each file is centred on X/Y = 0 with Z = 0 on the bed (lid and pipes share the lid's
centre). Output in enclosure/print/; the document is only read, never saved.
"""

import os

import FreeCAD as App
import Mesh
import MeshPart

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(
    globals().get("__file__", "enclosure/scripts/export_print.py"))))
DOC = os.environ.get("ENCLOSURE_DOC", os.path.join(ROOT, "devhub-enclosure.FCStd"))
OUT = os.environ.get("PRINT_DIR", os.path.join(ROOT, "print"))
LINEAR_DEFLECTION = 0.01  # mm
ANGULAR_DEFLECTION = 0.0873  # rad, 5 degrees


def to_mesh(shape, placement):
    s = shape.copy()
    s.Placement = placement.multiply(s.Placement)
    return MeshPart.meshFromShape(Shape=s, LinearDeflection=LINEAR_DEFLECTION,
                                  AngularDeflection=ANGULAR_DEFLECTION, Relative=False)


def bed_placement(shape, rotation):
    """Rotation, then a shift putting the bounding box centre at X/Y = 0 and its bottom at Z = 0."""
    s = shape.copy()
    s.Placement = App.Placement(App.Vector(), rotation).multiply(s.Placement)
    bb = s.BoundBox
    shift = App.Vector(-(bb.XMin + bb.XMax) / 2, -(bb.YMin + bb.YMax) / 2, -bb.ZMin)
    return App.Placement(shift, App.Rotation()).multiply(App.Placement(App.Vector(), rotation))


def main():
    os.makedirs(OUT, exist_ok=True)
    doc = App.openDocument(DOC)
    doc.recompute()
    lid, pipes, plate = (doc.getObject(n).Shape for n in ("Lid", "LightPipes", "BottomPlate"))
    for name, s in (("Lid", lid), ("LightPipes", pipes), ("BottomPlate", plate)):
        if not s.isValid():
            raise RuntimeError(f"{name} is not a valid solid")

    flip = App.Rotation(App.Vector(1, 0, 0), 180)  # top face down
    lid_pl = bed_placement(lid, flip)
    plate_pl = bed_placement(plate, App.Rotation())
    files = {
        "lid.stl": to_mesh(lid, lid_pl),
        "light-pipes.stl": to_mesh(pipes, lid_pl),
        "bottom-plate.stl": to_mesh(plate, plate_pl),
    }
    for name, m in files.items():
        if not m.isSolid() or m.hasNonManifolds():
            raise RuntimeError(f"{name}: mesh is not a closed manifold")
        m.write(os.path.join(OUT, name))
        bb = m.BoundBox
        print(f"{name}: {m.CountFacets} facets, X {bb.XMin:.2f}..{bb.XMax:.2f} "
              f"Y {bb.YMin:.2f}..{bb.YMax:.2f} Z {bb.ZMin:.2f}..{bb.ZMax:.2f}")
    App.closeDocument(doc.Name)


main()
