"""Edit 02: sloped sides on the USB-C recesses instead of square steps.

The recess pockets get a taper; their sketches grow by the same amount at the outer face,
so the recess floor keeps UsbRecessWidth x UsbRecessHeight (room for the plug overmold).

Globals from fcrun.py: App, doc, Params, fclib.
"""

from fclib import add_param, set_constraint_expr

P = "Params."

add_param(Params, "UsbRecessDraftAngle", "App::PropertyAngle", "Openings", 45.0,
          "Slope of the USB recess sides from the wall normal (0 = square step)")
add_param(Params, "UsbRecessGrow", "App::PropertyDistance", "Derived", None,
          "USB recess outline growth at the outer face",
          expr=f"{P}UsbRecessDepth * tan({P}UsbRecessDraftAngle)")
Params.setDocumentationOfProperty("UsbRecessWidth", "Recess floor width per port, centred on the port (front: from first to last port)")
Params.setDocumentationOfProperty("UsbRecessHeight", "Recess floor height, centred on the USB-C axis")
Params.setDocumentationOfProperty("UsbRecessRadius", "Recess floor corner radius")
doc.recompute()

G = f"{P}UsbRecessGrow"
ports = {"front": ("J401", "J101")}
sk = doc.getObject("FrontUsbRecessSketch")
for name, (first, last) in ports.items():
    set_constraint_expr(sk, f"{name}_x0", f"{P}{first}X - {P}UsbRecessWidth / 2 - {G}")
    set_constraint_expr(sk, f"{name}_x1", f"{P}{last}X + {P}UsbRecessWidth / 2 + {G}")
    set_constraint_expr(sk, f"{name}_y0", f"{P}UsbAxisZ - {P}UsbRecessHeight / 2 - {G}")
    set_constraint_expr(sk, f"{name}_y1", f"{P}UsbAxisZ + {P}UsbRecessHeight / 2 + {G}")
    set_constraint_expr(sk, f"{name}_r", f"{P}UsbRecessRadius + {G}")
sk = doc.getObject("BackUsbRecessSketch")
for j in ("J1", "J2"):
    set_constraint_expr(sk, f"{j}_x0", f"{P}{j}X - {P}UsbRecessWidth / 2 - {G}")
    set_constraint_expr(sk, f"{j}_x1", f"{P}{j}X + {P}UsbRecessWidth / 2 + {G}")
    set_constraint_expr(sk, f"{j}_y0", f"{P}UsbAxisZ - {P}UsbRecessHeight / 2 - {G}")
    set_constraint_expr(sk, f"{j}_y1", f"{P}UsbAxisZ + {P}UsbRecessHeight / 2 + {G}")
    set_constraint_expr(sk, f"{j}_r", f"{P}UsbRecessRadius + {G}")

# Negative taper: the pocket narrows with depth (checked on the button chamfers).
for n in ("FrontUsbRecess", "BackUsbRecesses"):
    doc.getObject(n).setExpression("TaperAngle", f"-{P}UsbRecessDraftAngle")
