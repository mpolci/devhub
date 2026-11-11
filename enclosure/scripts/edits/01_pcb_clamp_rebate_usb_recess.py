"""Edit 01, applied with fcedit.py to the document created by build_enclosure.py.

- Bottom plate screws through the PCB holes: hollow plate columns under H1-H4 clamp the
  PCB against the lid standoffs; screw heads sit in the columns, 3 mm below the PCB.
  The lid bosses beside the PCB go away and the side gap shrinks.
- Plate guided by a rebate in the lid walls; the columns set the height, with a seat gap
  between plate and walls.
- USB-C openings follow the receptacle shell; the wall around them is thinned so that its
  outer face is at most UsbRecessFaceDistance from the receptacle face: one recess for
  the four front ports, one per port on the back.
- Button caps no longer stick out: slightly recessed, with a 45 degree chamfer on the holes.

Globals from fcrun.py: App, doc, Params, fclib.
"""

from fclib import SketchBuilder, new_sketch, pad, pocket, add_param, remove_feature, \
    set_constraint_expr

P = "Params."
L, D = "App::PropertyLength", "App::PropertyDistance"
lid = doc.getObject("Lid")
plate = doc.getObject("BottomPlate")
HOLES = ["H1", "H2", "H3", "H4"]

# ---------------------------------------------------------------- old plate fastening
for n in ("LidBossHoles", "LidBossHoleSketch", "LidBosses", "LidBossSketch",
          "PlateScrewHoles", "PlateScrewSketch"):
    remove_feature(doc, n)
for n in ("LidBossX0", "LidBossX1", "LidBossDiameter", "LidBossClearance",
          "LidScrewPilotDiameter", "LidScrewHoleDepth", "PlateScrewClearanceDiameter",
          "LidBossFrontY", "LidBossBackY"):
    if n in Params.PropertiesList:
        Params.removeProperty(n)

# ---------------------------------------------------------------- parameters
Params.SideGap = 1.5
Params.setDocumentationOfProperty(
    "SideGap", "Gap between PCB left/right edges and the inner wall faces (room for the front USB recess at the corner)")
Params.ButtonProtrusion = -0.3
Params.UsbOpeningWidth = 9.3
Params.UsbOpeningHeight = 3.5
Params.UsbOpeningRadius = 1.6
Params.setDocumentationOfProperty("UsbOpeningWidth", "USB-C opening around the receptacle shell (8.94 mm)")
Params.setDocumentationOfProperty("UsbOpeningHeight", "USB-C opening around the receptacle shell (3.17 mm)")
Params.setDocumentationOfProperty("UsbOpeningRadius", "USB-C opening corner radius (at most half the height)")
Params.PcbScrewHoleDepth = 8.0
Params.setDocumentationOfProperty("PcbScrewHoleDepth", "Pilot hole depth in the lid standoffs, from the PCB top face")
Params.setDocumentationOfProperty("PcbScrewPilotDiameter", "Pilot hole in the lid standoffs for the M3 self-tapping screws")
Params.VentBottomAbovePlate = 4.0

for args in [
    ("ButtonChamfer", L, "Buttons", 0.8, "45 degree chamfer on the button holes, lid top face"),
    ("UsbRecessFaceDistance", L, "Openings", 1.2, "Max distance from the USB-C receptacle face to the outer wall surface"),
    ("UsbRecessWidth", L, "Openings", 14.0, "Recess width per port, centred on the port (front: from first to last port)"),
    ("UsbRecessHeight", L, "Openings", 8.0, "Recess height, centred on the USB-C axis"),
    ("UsbRecessRadius", L, "Openings", 1.5, "Recess corner radius"),
    ("ColumnDiameter", L, "Mounting", 9.0, "Plate columns under H1-H4"),
    ("ScrewClearanceDiameter", L, "Mounting", 3.4, "Screw hole through the plate columns"),
    ("ScrewHeadDiameter", L, "Mounting", 6.2, "Bore for the screw heads inside the columns"),
    ("ScrewHeadSeatBelowPcb", L, "Mounting", 3.0, "Screw head seat below the PCB bottom face (M3x10-12)"),
    ("PlateSeatGap", L, "Shell", 0.2, "Gap between plate and lid walls: the columns set the plate height"),
    ("RebateHeight", L, "Shell", 2.5, "Rebate in the lid walls guiding the plate"),
    ("RebateDepth", L, "Shell", 1.0, "Rebate depth into the wall thickness"),
    ("RebateClearance", L, "Shell", 0.15, "Side clearance between plate lip and rebate"),
]:
    add_param(Params, *args)
for name, t, expr, desc in [
    ("PlateTopZ", D, f"{P}BottomZ - {P}PlateSeatGap", "Bottom plate top face"),
    ("UsbRecessDepth", D, f"{P}FrontBackGap + {P}WallThickness - {P}UsbRecessFaceDistance", "USB recess depth into the wall"),
]:
    add_param(Params, name, t, "Derived", None, desc, expr=expr)
doc.recompute()
sb = lambda sk: SketchBuilder(sk, Params)  # noqa: E731

# ---------------------------------------------------------------- lid
sk = new_sketch(lid, "ButtonChamferSketch", "XY", f"{P}TopZ")
b = sb(sk)
for s in ["SW1", "SW401", "SW301", "SW201", "SW101"]:
    b.circle(f"{P}{s}X", f"{P}ButtonY", f"{P}ButtonHoleDiameter + 2 * {P}ButtonChamfer", s)
pocket(lid, "ButtonChamfers", sk, f"{P}ButtonChamfer", taper_expr="-45 deg")  # negative: narrows with depth

# Front: one recess over the four ports, cut inward from the outer face (XZ normal -Y).
sk = new_sketch(lid, "FrontUsbRecessSketch", "XZ", f"{P}FrontBackGap + {P}WallThickness")
sb(sk).rounded_rect(f"{P}J401X - {P}UsbRecessWidth / 2", f"{P}UsbAxisZ - {P}UsbRecessHeight / 2",
                    f"{P}J101X + {P}UsbRecessWidth / 2", f"{P}UsbAxisZ + {P}UsbRecessHeight / 2",
                    f"{P}UsbRecessRadius", "front")
pocket(lid, "FrontUsbRecess", sk, f"{P}UsbRecessDepth")

# Back: one recess per port; the inward direction is opposite, hence reversed.
sk = new_sketch(lid, "BackUsbRecessSketch", "XZ",
                f"-({P}PcbDepth + {P}FrontBackGap + {P}WallThickness)")
b = sb(sk)
for j in ["J1", "J2"]:
    b.rounded_rect(f"{P}{j}X - {P}UsbRecessWidth / 2", f"{P}UsbAxisZ - {P}UsbRecessHeight / 2",
                   f"{P}{j}X + {P}UsbRecessWidth / 2", f"{P}UsbAxisZ + {P}UsbRecessHeight / 2",
                   f"{P}UsbRecessRadius", j)
pocket(lid, "BackUsbRecesses", sk, f"{P}UsbRecessDepth", reversed_=True)

# Rebate: inner part of the wall removed at the bottom edge.
sk = new_sketch(lid, "RebateSketch", "XY", f"{P}BottomZ")
sb(sk).rounded_rect(f"-({P}SideGap + {P}RebateDepth)", f"-({P}FrontBackGap + {P}RebateDepth)",
                    f"{P}PcbWidth + {P}SideGap + {P}RebateDepth",
                    f"{P}PcbDepth + {P}FrontBackGap + {P}RebateDepth",
                    f"{P}InnerCornerRadius + {P}RebateDepth", "rebate")
pocket(lid, "Rebate", sk, f"{P}RebateHeight", reversed_=True)

# ---------------------------------------------------------------- bottom plate
plate_sk = doc.getObject("PlateOutlineSketch")
plate_sk.setExpression(".AttachmentOffset.Base.z", f"{P}PlateTopZ - {P}PlateThickness")
tongue_sk = doc.getObject("TongueSketch")
for j in ["J5", "J6"]:
    set_constraint_expr(tongue_sk, f"{j}_p0_y", f"{P}PlateTopZ")

# Lip inside the rebate: ring from the cavity outline to the rebate minus clearance.
sk = new_sketch(plate, "LipSketch", "XY", f"{P}PlateTopZ")
b = sb(sk)
b.rounded_rect(f"-({P}SideGap + {P}RebateDepth - {P}RebateClearance)",
               f"-({P}FrontBackGap + {P}RebateDepth - {P}RebateClearance)",
               f"{P}PcbWidth + {P}SideGap + {P}RebateDepth - {P}RebateClearance",
               f"{P}PcbDepth + {P}FrontBackGap + {P}RebateDepth - {P}RebateClearance",
               f"{P}InnerCornerRadius + {P}RebateDepth - {P}RebateClearance", "outer")
b.rounded_rect(f"-{P}SideGap", f"-{P}FrontBackGap", f"{P}PcbWidth + {P}SideGap",
               f"{P}PcbDepth + {P}FrontBackGap", f"{P}InnerCornerRadius", "inner")
pad(plate, "Lip", sk, f"{P}RebateHeight")

# Columns from the plate to the PCB bottom face, under H1-H4.
sk = new_sketch(plate, "ColumnSketch", "XY", f"{P}PlateTopZ")
b = sb(sk)
for h in HOLES:
    b.circle(f"{P}{h}X", f"{P}{h}Y", f"{P}ColumnDiameter", h)
pad(plate, "Columns", sk, f"{P}PcbBottomZ - {P}PlateTopZ")

sk = new_sketch(plate, "ScrewHoleSketch", "XY", f"{P}PcbBottomZ")
b = sb(sk)
for h in HOLES:
    b.circle(f"{P}{h}X", f"{P}{h}Y", f"{P}ScrewClearanceDiameter", h)
pocket(plate, "ScrewHoles", sk, through_all=True)

sk = new_sketch(plate, "ScrewHeadBoreSketch", "XY", f"{P}PlateTopZ - {P}PlateThickness")
b = sb(sk)
for h in HOLES:
    b.circle(f"{P}{h}X", f"{P}{h}Y", f"{P}ScrewHeadDiameter", h)
pocket(plate, "ScrewHeadBores", sk,
       f"{P}PcbBottomZ - {P}ScrewHeadSeatBelowPcb - ({P}PlateTopZ - {P}PlateThickness)",
       reversed_=True)
