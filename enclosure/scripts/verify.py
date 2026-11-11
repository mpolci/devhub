"""Geometry checks of the enclosure against the PCB reference (run with freecadcmd).

    freecadcmd -c "exec(open('enclosure/scripts/verify.py').read())"

Checks:
- every body is one valid solid (light pipes: one solid per LED);
- no overlap between lid, bottom plate and light pipes;
- no overlap between any printed part and the PCB with its components;
- assembly path: the PCB goes into the upturned lid along Z, so every PCB part swept
  downward from its final position must not hit the lid (banana sleeves through the
  open U-slots included);
- minimum clearance between each printed part and the PCB parts.
Writes the report to enclosure/verify-report.txt and prints it.
"""

import os

import FreeCAD as App
import Part

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(
    globals().get("__file__", "enclosure/scripts/verify.py"))))
DOC = os.environ.get("ENCLOSURE_DOC", os.path.join(ROOT, "devhub-enclosure.FCStd"))
TOL = 1e-3  # mm3

lines = []


def out(s=""):
    lines.append(s)


doc = App.openDocument(DOC)
doc.recompute()
lid, plate, pipes = (doc.getObject(n).Shape for n in ("Lid", "BottomPlate", "LightPipes"))
link = doc.getObject("PcbRef")
pcb_doc = link.LinkedObject.Document
shift = link.Placement

parts = []
for o in pcb_doc.Objects:
    if o.TypeId == "Part::Feature" and not o.Shape.isNull() and o.Shape.Volume > 1e-6:
        s = o.Shape.copy()
        s.Placement = shift.multiply(o.getGlobalPlacement())
        parent = o.getParentGeoFeatureGroup()
        parts.append(((parent.Label if parent else o.Label) + "/" + o.Label, s))

out("== solids")
for n, s in (("Lid", lid), ("BottomPlate", plate), ("LightPipes", pipes)):
    out(f"{n}: valid={s.isValid()} solids={len(s.Solids)} volume={s.Volume:.0f} mm3 "
        f"bbox X {s.BoundBox.XMin:.2f}..{s.BoundBox.XMax:.2f} Y {s.BoundBox.YMin:.2f}..{s.BoundBox.YMax:.2f} "
        f"Z {s.BoundBox.ZMin:.2f}..{s.BoundBox.ZMax:.2f}")

out("== feature integrity (no cut may reach these)")
import math  # noqa: E402
Pv = doc.getObject("Params")
ceiling, pilot_r = Pv.CeilingZ.Value, Pv.PcbScrewPilotDiameter.Value / 2
std_r = Pv.StandoffDiameter.Value / 2
expected = math.pi * std_r ** 2 * ceiling - math.pi * pilot_r ** 2 * Pv.PcbScrewHoleDepth.Value
for h in ("H1", "H2", "H3", "H4"):
    c = Part.makeCylinder(std_r, ceiling, App.Vector(getattr(Pv, h + "X").Value, getattr(Pv, h + "Y").Value, 0))
    v = c.common(lid).Volume
    out(f"{h} lid standoff: {v:.1f} of {expected:.1f} mm3 {'OK' if abs(v - expected) < 0.5 else 'DAMAGED'}")
col_r = Pv.ColumnDiameter.Value / 2
for h in ("H1", "H2", "H3", "H4"):
    x, y = getattr(Pv, h + "X").Value, getattr(Pv, h + "Y").Value
    z0, z1 = Pv.PlateTopZ.Value, Pv.PcbBottomZ.Value
    c = Part.makeCylinder(col_r, z1 - z0, App.Vector(x, y, z0))
    full = c.Volume
    v = c.common(plate).Volume
    out(f"{h} plate column: {v / full:.1%} filled (bores included)")

out("== printed parts overlap")
for (a, sa), (b, sb) in ((("Lid", lid), ("BottomPlate", plate)), (("Lid", lid), ("LightPipes", pipes)),
                         (("BottomPlate", plate), ("LightPipes", pipes))):
    v = sa.common(sb).Volume
    out(f"{a} x {b}: {v:.4f} mm3 {'OK' if v < TOL else 'OVERLAP'}")

out("== interference with PCB parts")
bad = 0
for name, s in parts:
    for pn, ps in (("Lid", lid), ("BottomPlate", plate), ("LightPipes", pipes)):
        if not s.BoundBox.intersect(ps.BoundBox):
            continue
        v = s.common(ps).Volume
        if v > TOL:
            bad += 1
            out(f"  {pn} x {name}: {v:.3f} mm3")
out(f"interferences: {bad}")

out("== assembly path (PCB inserted along Z into the upturned lid)")
# Inside the PCB outline, nothing of the lid may sit below the PCB top face.
P = doc.getObject("Params")
W, Dp = P.PcbWidth.Value, P.PcbDepth.Value
PCB_CORNER_RADIUS = 3.0  # D84
prism = Part.makeBox(W, Dp, 100, App.Vector(0, 0, -100))
prism = prism.makeFillet(PCB_CORNER_RADIUS,
                         [e for e in prism.Edges if abs(e.BoundBox.ZLength - 100) < 1e-6])
under = prism.common(lid).Volume
out(f"lid material under the PCB outline: {under:.4f} mm3 {'OK' if under < TOL else 'BLOCKS'}")
# Parts sticking out of the outline (banana sleeves) are swept down step by step.
blocked = 0
for name, s in parts:
    bb = s.BoundBox
    if bb.XMin > -0.01 and bb.YMin > -0.01 and bb.XMax < W + 0.01 and bb.YMax < Dp + 0.01:
        continue
    hit = 0.0
    for k in range(1, 81):
        t = s.copy()
        t.translate(App.Vector(0, 0, -0.5 * k))
        if t.BoundBox.intersect(lid.BoundBox):
            hit = max(hit, t.common(lid).Volume)
    out(f"  outside the outline: {name}: {'BLOCKED %.3f mm3' % hit if hit > TOL else 'free'}")
    blocked += hit > TOL
out(f"blocked parts: {blocked}")

out("== minimum clearances to PCB parts (closest five per printed part)")
for pn, ps in (("Lid", lid), ("BottomPlate", plate), ("LightPipes", pipes)):
    dists = []
    for name, s in parts:
        bb = s.BoundBox
        bb.enlarge(3)
        if bb.intersect(ps.BoundBox):
            dists.append((ps.distToShape(s)[0], name))
    for d, name in sorted(dists)[:5]:
        out(f"  {pn} - {name}: {d:.2f} mm")

report = "\n".join(lines)
print(report)
with open(os.path.join(ROOT, "verify-report.txt"), "w") as f:
    f.write(report + "\n")
