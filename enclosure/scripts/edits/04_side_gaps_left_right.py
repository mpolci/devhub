"""Edit 04: separate left and right side gaps; the PCB moves toward the left wall.

The extra room is needed only on the right, where the sloped front USB recess must end
before the corner rounding. Every expression using SideGap is rebound: the ones on the
right side (they add PcbWidth) to SideGapRight, the others to SideGapLeft.

Globals from fcrun.py: App, doc, Params, fclib.
"""

from fclib import add_param

add_param(Params, "SideGapLeft", "App::PropertyLength", "Shell", 0.2,
          "Gap between the PCB left edge and the inner wall face")
add_param(Params, "SideGapRight", "App::PropertyLength", "Shell", 2.5,
          "Gap between the PCB right edge and the inner wall face (front USB recess before the corner)")
doc.recompute()

import re

# Expressions of the VarSet itself refer to SideGap without the "Params." prefix.
OLD = re.compile(r"\b((?:Params\.)?)SideGap\b")
rebound = []
for o in doc.Objects:
    for path, expr in list(o.ExpressionEngine):
        if not OLD.search(expr):
            continue
        side = "SideGapRight" if "PcbWidth" in expr else "SideGapLeft"
        o.setExpression(path, OLD.sub(lambda m: m.group(1) + side, expr))
        rebound.append(f"{o.Name}{path}")
doc.recompute()
left = [f"{o.Name}{p}" for o in doc.Objects for p, e in o.ExpressionEngine if OLD.search(e)]
if left:
    raise RuntimeError(f"SideGap still referenced by {left}")
Params.removeProperty("SideGap")
print("rebound:", rebound)
