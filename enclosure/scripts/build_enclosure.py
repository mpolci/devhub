"""Create devhub-enclosure.FCStd from scratch.

Run once, only when the file does not exist yet: later changes go through fcedit.py,
which modifies the existing document and keeps the edits made by hand in FreeCAD.

    FreeCAD build_enclosure.py          (GUI, so that default styles are stored too)
    freecadcmd build_enclosure.py       (geometry only, for checks)
"""

import os
import sys

import FreeCAD as App

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import fclib  # noqa: E402
from fclib import SketchBuilder, new_sketch, pad, pocket, linear_pattern  # noqa: E402

ENCLOSURE_DIR = os.path.dirname(HERE)
OUT = os.environ.get("ENCLOSURE_OUT", os.path.join(ENCLOSURE_DIR, "devhub-enclosure.FCStd"))
PCB_DOC = os.path.join(ENCLOSURE_DIR, "pcb", "devhub-pcb.FCStd")

L, D, I = "App::PropertyLength", "App::PropertyDistance", "App::PropertyInteger"

# name, type, group, value, description
PARAMS = [
    # PCB and components (from hardware/devhub/devhub.kicad_pcb and the 3D models)
    ("PcbWidth", L, "PCB", 100.0, "PCB size along X (u)"),
    ("PcbDepth", L, "PCB", 100.0, "PCB size along Y (v), front edge at Y = 0"),
    ("PcbThickness", L, "PCB", 1.6, "PCB thickness; Z = 0 is the top face"),
    ("ComponentMaxDepth", L, "PCB", 15.8, "Tallest part below the PCB bottom face (relay K1, G5Q-1 15.8 max)"),
    ("LedHeight", L, "PCB", 1.1, "LED 0805 height above the PCB top face"),
    ("ButtonCapHeight", L, "PCB", 10.0, "Top of the button caps above the PCB top face"),
    ("UsbAxisBelowPcb", L, "PCB", 1.815, "USB-C receptacle axis below the PCB bottom face"),
    ("BananaSleeveDiameter", L, "PCB", 11.0, "Insulating sleeve diameter of the banana jacks (Cliff S16N-PC)"),
    ("BananaAxisBelowPcb", L, "PCB", 6.38, "Banana jack axis below the PCB bottom face"),
    ("BananaSleeveProtrusion", L, "PCB", 2.15, "Banana sleeve protrusion beyond the PCB front edge"),
    # Shell
    ("WallThickness", L, "Shell", 2.0, "Side, front and back wall thickness"),
    ("TopThickness", L, "Shell", 2.0, "Lid top thickness"),
    ("FrontBackGap", L, "Shell", 0.2, "Gap between PCB front/back edges and the inner wall faces"),
    ("SideGap", L, "Shell", 5.5, "Gap between PCB left/right edges and the inner wall faces (room for the lid bosses)"),
    ("OuterCornerRadius", L, "Shell", 4.0, "Outer vertical corner radius (must exceed WallThickness)"),
    ("BottomClearance", L, "Shell", 1.5, "Clearance between the tallest bottom-side part and the bottom plate"),
    ("PlateThickness", L, "Shell", 2.5, "Bottom plate thickness"),
    # Buttons
    ("ButtonProtrusion", D, "Buttons", 1.0, "Button caps above the lid top face (negative = recessed)"),
    ("ButtonHoleDiameter", L, "Buttons", 6.6, "Hole for the 6 mm button caps"),
    ("ButtonY", D, "Buttons", 14.5, "Button row from the PCB front edge"),
    ("SW1X", D, "Buttons", 18.5, "SW1, relay button"),
    ("SW401X", D, "Buttons", 46.0, "SW401, port 4 button"),
    ("SW301X", D, "Buttons", 61.0, "SW301, port 3 button"),
    ("SW201X", D, "Buttons", 76.0, "SW201, port 2 button"),
    ("SW101X", D, "Buttons", 91.0, "SW101, port 1 button"),
    # LEDs and light pipes
    ("LightPipeDiameter", L, "LightPipes", 3.0, "Light pipe diameter (transparent PETG)"),
    ("LightPipeGap", L, "LightPipes", 0.3, "Gap between LED top and light pipe bottom"),
    ("ShroudWall", L, "LightPipes", 1.0, "Opaque shroud wall around each light pipe"),
    ("LedRowY", D, "LightPipes", 2.0, "Front LED row from the PCB front edge"),
    ("D90X", D, "LightPipes", 18.5, "D90, relay LED"),
    ("D402X", D, "LightPipes", 46.0, "D402, port 4 LED"),
    ("D302X", D, "LightPipes", 61.0, "D302, port 3 LED"),
    ("D202X", D, "LightPipes", 76.0, "D202, port 2 LED"),
    ("D102X", D, "LightPipes", 91.0, "D102, port 1 LED"),
    ("D93X", D, "LightPipes", 10.0, "D93, host LED"),
    ("D93Y", D, "LightPipes", 90.0, "D93, host LED"),
    # PCB mounting (self-tapping screws from below the PCB into lid standoffs)
    ("StandoffDiameter", L, "Mounting", 6.0, "Lid standoffs holding the PCB"),
    ("PcbScrewPilotDiameter", L, "Mounting", 2.5, "Pilot hole for M3 self-tapping screws"),
    ("PcbScrewHoleDepth", L, "Mounting", 6.0, "Pilot hole depth in the standoffs"),
    ("H1X", D, "Mounting", 20.0, "H1"), ("H1Y", D, "Mounting", 44.2, "H1"),
    ("H2X", D, "Mounting", 95.5, "H2"), ("H2Y", D, "Mounting", 28.0, "H2"),
    ("H3X", D, "Mounting", 4.5, "H3"), ("H3Y", D, "Mounting", 95.5, "H3"),
    ("H4X", D, "Mounting", 95.5, "H4"), ("H4Y", D, "Mounting", 95.5, "H4"),
    # Bottom plate fastening (self-tapping screws from below into lid bosses beside the PCB)
    ("LidBossDiameter", L, "PlateScrews", 6.5, "Lid bosses for the bottom plate screws"),
    ("LidBossClearance", L, "PlateScrews", 0.5, "Gap between lid bosses and PCB side edges"),
    ("LidScrewPilotDiameter", L, "PlateScrews", 2.5, "Pilot hole for M3 self-tapping screws"),
    ("LidScrewHoleDepth", L, "PlateScrews", 10.0, "Pilot hole depth in the lid bosses"),
    ("PlateScrewClearanceDiameter", L, "PlateScrews", 3.4, "Clearance hole in the bottom plate"),
    ("LidBossFrontY", D, "PlateScrews", 10.0, "Front bosses Y"),
    ("LidBossBackY", D, "PlateScrews", 90.0, "Back bosses Y"),
    # Connector openings
    ("UsbOpeningWidth", L, "Openings", 12.6, "USB-C opening, sized for the plug overmold"),
    ("UsbOpeningHeight", L, "Openings", 6.8, "USB-C opening, sized for the plug overmold"),
    ("UsbOpeningRadius", L, "Openings", 1.5, "USB-C opening corner radius"),
    ("J401X", D, "Openings", 46.0, "J401, port 4 (front)"),
    ("J301X", D, "Openings", 61.0, "J301, port 3 (front)"),
    ("J201X", D, "Openings", 76.0, "J201, port 2 (front)"),
    ("J101X", D, "Openings", 91.0, "J101, port 1 (front)"),
    ("J1X", D, "Openings", 24.0, "J1, power input (back)"),
    ("J2X", D, "Openings", 82.0, "J2, upstream (back)"),
    ("BananaClearance", L, "Openings", 0.3, "Diametral clearance around the banana sleeves"),
    ("J5X", D, "Openings", 10.0, "J5, relay COM (black)"),
    ("J6X", D, "Openings", 27.0, "J6, relay NC (red)"),
    ("TongueClearance", L, "Openings", 0.15, "Side gap between plate tongues and lid U-slots"),
    ("TongueInnerLength", L, "Openings", 8.0, "Tongue length inside the inner wall face, under the sleeves"),
    # Ventilation
    ("VentSlotWidth", L, "Ventilation", 2.0, "Side wall vent slot width"),
    ("VentTopBelowPcb", L, "Ventilation", 3.0, "Slot top end below the PCB bottom face"),
    ("VentBottomAbovePlate", L, "Ventilation", 3.0, "Slot bottom end above the bottom plate"),
    ("VentFirstY", D, "Ventilation", 22.5, "First slot Y"),
    ("VentPitch", L, "Ventilation", 5.0, "Slot pitch along Y"),
    ("VentCount", I, "Ventilation", 12, "Slots per side"),
]

P = "Params."
DERIVED = [
    ("TopZ", D, f"{P}ButtonCapHeight - {P}ButtonProtrusion", "Lid top face"),
    ("CeilingZ", D, f"{P}TopZ - {P}TopThickness", "Lid inner ceiling"),
    ("PcbBottomZ", D, f"-{P}PcbThickness", "PCB bottom face"),
    ("BottomZ", D, f"-({P}PcbThickness + {P}ComponentMaxDepth + {P}BottomClearance)",
     "Lid bottom edge = bottom plate top face"),
    ("OuterX0", D, f"-({P}SideGap + {P}WallThickness)", "Outline"),
    ("OuterX1", D, f"{P}PcbWidth + {P}SideGap + {P}WallThickness", "Outline"),
    ("OuterY0", D, f"-({P}FrontBackGap + {P}WallThickness)", "Outline"),
    ("OuterY1", D, f"{P}PcbDepth + {P}FrontBackGap + {P}WallThickness", "Outline"),
    ("InnerCornerRadius", D, f"{P}OuterCornerRadius - {P}WallThickness", "Cavity corner radius"),
    ("UsbAxisZ", D, f"-({P}PcbThickness + {P}UsbAxisBelowPcb)", "USB-C axis"),
    ("BananaAxisZ", D, f"-({P}PcbThickness + {P}BananaAxisBelowPcb)", "Banana jack axis"),
    ("BananaSlotWidth", D, f"{P}BananaSleeveDiameter + {P}BananaClearance", "U-slot width"),
    ("TongueWidth", D, f"{P}BananaSlotWidth - 2 * {P}TongueClearance", "Plate tongue width"),
    ("LightPipeBottomZ", D, f"{P}LedHeight + {P}LightPipeGap", "Light pipe bottom"),
    ("LidBossX0", D, f"-({P}LidBossClearance + {P}LidBossDiameter / 2)", "Left lid bosses"),
    ("LidBossX1", D, f"{P}PcbWidth + {P}LidBossClearance + {P}LidBossDiameter / 2", "Right lid bosses"),
    ("VentTopZ", D, f"{P}PcbBottomZ - {P}VentTopBelowPcb - {P}VentSlotWidth / 2", "Vent slot upper arc centre"),
    ("VentBottomZ", D, f"{P}BottomZ + {P}VentBottomAbovePlate + {P}VentSlotWidth / 2", "Vent slot lower arc centre"),
]

BUTTONS = ["SW1", "SW401", "SW301", "SW201", "SW101"]
FRONT_LEDS = ["D90", "D402", "D302", "D202", "D102"]
HOLES = ["H1", "H2", "H3", "H4"]
FRONT_USB = ["J401", "J301", "J201", "J101"]
BACK_USB = ["J1", "J2"]
BANANAS = ["J5", "J6"]


def build(doc):
    vs = doc.addObject("App::VarSet", "Params")
    vs.Label = "Params"
    for name, t, group, value, desc in PARAMS:
        fclib.add_param(vs, name, t, group, value, desc)
    for name, t, expr, desc in DERIVED:
        fclib.add_param(vs, name, t, "Derived", None, desc, expr=expr)
    doc.recompute()

    # ------------------------------------------------------------------ Lid
    lid = doc.addObject("PartDesign::Body", "Lid")
    sb = lambda sk: SketchBuilder(sk, vs)  # noqa: E731

    sk = new_sketch(lid, "LidOutlineSketch", "XY", f"{P}BottomZ")
    sb(sk).rounded_rect(f"{P}OuterX0", f"{P}OuterY0", f"{P}OuterX1", f"{P}OuterY1",
                        f"{P}OuterCornerRadius", "outline")
    pad(lid, "LidBlock", sk, f"{P}TopZ - {P}BottomZ")

    sk = new_sketch(lid, "CavitySketch", "XY", f"{P}BottomZ")
    sb(sk).rounded_rect(f"-{P}SideGap", f"-{P}FrontBackGap", f"{P}PcbWidth + {P}SideGap",
                        f"{P}PcbDepth + {P}FrontBackGap", f"{P}InnerCornerRadius", "cavity")
    pocket(lid, "Cavity", sk, f"{P}CeilingZ - {P}BottomZ", reversed_=True)

    sk = new_sketch(lid, "StandoffSketch", "XY", f"{P}CeilingZ")
    b = sb(sk)
    for h in HOLES:
        b.circle(f"{P}{h}X", f"{P}{h}Y", f"{P}StandoffDiameter", h)
    pad(lid, "Standoffs", sk, f"{P}CeilingZ", reversed_=True)

    sk = new_sketch(lid, "StandoffHoleSketch", "XY", "0 mm")
    b = sb(sk)
    for h in HOLES:
        b.circle(f"{P}{h}X", f"{P}{h}Y", f"{P}PcbScrewPilotDiameter", h)
    pocket(lid, "StandoffHoles", sk, f"{P}PcbScrewHoleDepth", reversed_=True)

    sk = new_sketch(lid, "LidBossSketch", "XY", f"{P}BottomZ")
    b = sb(sk)
    for n, x in (("L", "LidBossX0"), ("R", "LidBossX1")):
        for m, y in (("F", "LidBossFrontY"), ("B", "LidBossBackY")):
            b.circle(f"{P}{x}", f"{P}{y}", f"{P}LidBossDiameter", f"boss{n}{m}")
    pad(lid, "LidBosses", sk, f"{P}CeilingZ - {P}BottomZ")

    sk = new_sketch(lid, "LidBossHoleSketch", "XY", f"{P}BottomZ")
    b = sb(sk)
    for n, x in (("L", "LidBossX0"), ("R", "LidBossX1")):
        for m, y in (("F", "LidBossFrontY"), ("B", "LidBossBackY")):
            b.circle(f"{P}{x}", f"{P}{y}", f"{P}LidScrewPilotDiameter", f"hole{n}{m}")
    pocket(lid, "LidBossHoles", sk, f"{P}LidScrewHoleDepth", reversed_=True)

    sk = new_sketch(lid, "ButtonHoleSketch", "XY", f"{P}TopZ")
    b = sb(sk)
    for s in BUTTONS:
        b.circle(f"{P}{s}X", f"{P}ButtonY", f"{P}ButtonHoleDiameter", s)
    pocket(lid, "ButtonHoles", sk, through_all=True)

    sk = new_sketch(lid, "ShroudSketch", "XY", f"{P}CeilingZ")
    b = sb(sk)
    shroud_d = f"{P}LightPipeDiameter + 2 * {P}ShroudWall"
    for led in FRONT_LEDS:
        b.circle(f"{P}{led}X", f"{P}LedRowY", shroud_d, led)
    b.circle(f"{P}D93X", f"{P}D93Y", shroud_d, "D93")
    pad(lid, "Shrouds", sk, f"{P}CeilingZ - {P}LightPipeBottomZ", reversed_=True)

    sk = new_sketch(lid, "LightPipeHoleSketch", "XY", f"{P}TopZ")
    b = sb(sk)
    for led in FRONT_LEDS:
        b.circle(f"{P}{led}X", f"{P}LedRowY", f"{P}LightPipeDiameter", led)
    b.circle(f"{P}D93X", f"{P}D93Y", f"{P}LightPipeDiameter", "D93")
    pocket(lid, "LightPipeHoles", sk, through_all=True)

    # Front wall: XZ plane, normal -Y, so +offset moves toward -Y.
    sk = new_sketch(lid, "FrontUsbSketch", "XZ", f"{P}FrontBackGap + {P}WallThickness / 2")
    b = sb(sk)
    for j in FRONT_USB:
        b.rounded_rect(f"{P}{j}X - {P}UsbOpeningWidth / 2", f"{P}UsbAxisZ - {P}UsbOpeningHeight / 2",
                       f"{P}{j}X + {P}UsbOpeningWidth / 2", f"{P}UsbAxisZ + {P}UsbOpeningHeight / 2",
                       f"{P}UsbOpeningRadius", j)
    pocket(lid, "FrontUsbOpenings", sk, f"{P}WallThickness + 2 mm", midplane=True)

    sk = new_sketch(lid, "BackUsbSketch", "XZ",
                    f"-({P}PcbDepth + {P}FrontBackGap + {P}WallThickness / 2)")
    b = sb(sk)
    for j in BACK_USB:
        b.rounded_rect(f"{P}{j}X - {P}UsbOpeningWidth / 2", f"{P}UsbAxisZ - {P}UsbOpeningHeight / 2",
                       f"{P}{j}X + {P}UsbOpeningWidth / 2", f"{P}UsbAxisZ + {P}UsbOpeningHeight / 2",
                       f"{P}UsbOpeningRadius", j)
    pocket(lid, "BackUsbOpenings", sk, f"{P}WallThickness + 2 mm", midplane=True)

    sk = new_sketch(lid, "BananaSlotSketch", "XZ", f"{P}FrontBackGap + {P}WallThickness / 2")
    b = sb(sk)
    for j in BANANAS:
        b.slot_v(f"{P}{j}X", f"{P}BananaAxisZ", f"{P}BottomZ - {P}BananaSlotWidth",
                 f"{P}BananaSlotWidth", j)
    pocket(lid, "BananaSlots", sk, f"{P}WallThickness + 2 mm", midplane=True)

    # Side walls: one slot through both walls, repeated along Y.
    sk = new_sketch(lid, "VentSketch", "YZ", f"{P}PcbWidth / 2")
    sb(sk).slot_v(f"{P}VentFirstY", f"{P}VentTopZ", f"{P}VentBottomZ", f"{P}VentSlotWidth", "vent")
    vent = pocket(lid, "VentSlot", sk, through_all=True, midplane=True)
    linear_pattern(lid, "VentSlots", vent, (sk, ["H_Axis"]), f"{P}VentPitch", f"{P}VentCount")

    # ------------------------------------------------------------------ Bottom plate
    plate = doc.addObject("PartDesign::Body", "BottomPlate")

    sk = new_sketch(plate, "PlateOutlineSketch", "XY", f"{P}BottomZ - {P}PlateThickness")
    sb(sk).rounded_rect(f"{P}OuterX0", f"{P}OuterY0", f"{P}OuterX1", f"{P}OuterY1",
                        f"{P}OuterCornerRadius", "outline")
    pad(plate, "Plate", sk, f"{P}PlateThickness")

    # Tongues closing the banana U-slots from below; sketch on the outer front face.
    sk = new_sketch(plate, "TongueSketch", "XZ", f"{P}FrontBackGap + {P}WallThickness")
    b = sb(sk)
    for j in BANANAS:
        b.rect(f"{P}{j}X - {P}TongueWidth / 2", f"{P}BottomZ",
               f"{P}{j}X + {P}TongueWidth / 2", f"{P}BananaAxisZ", j)
    pad(plate, "Tongues", sk, f"{P}FrontBackGap + {P}WallThickness + {P}TongueInnerLength",
        reversed_=True)

    sk = new_sketch(plate, "CradleSketch", "XZ", f"{P}FrontBackGap + {P}WallThickness")
    b = sb(sk)
    for j in BANANAS:
        b.circle(f"{P}{j}X", f"{P}BananaAxisZ", f"{P}BananaSlotWidth", j)
    pocket(plate, "Cradles", sk, through_all=True, midplane=True)

    sk = new_sketch(plate, "PlateScrewSketch", "XY", f"{P}BottomZ")
    b = sb(sk)
    for n, x in (("L", "LidBossX0"), ("R", "LidBossX1")):
        for m, y in (("F", "LidBossFrontY"), ("B", "LidBossBackY")):
            b.circle(f"{P}{x}", f"{P}{y}", f"{P}PlateScrewClearanceDiameter", f"hole{n}{m}")
    pocket(plate, "PlateScrewHoles", sk, through_all=True)

    # ------------------------------------------------------------------ Light pipes
    pipes = doc.addObject("PartDesign::Body", "LightPipes")
    sk = new_sketch(pipes, "LightPipeSketch", "XY", f"{P}TopZ")
    b = sb(sk)
    for led in FRONT_LEDS:
        b.circle(f"{P}{led}X", f"{P}LedRowY", f"{P}LightPipeDiameter", led)
    b.circle(f"{P}D93X", f"{P}D93Y", f"{P}LightPipeDiameter", "D93")
    pad(pipes, "Pipes", sk, f"{P}TopZ - {P}LightPipeBottomZ", reversed_=True)

    doc.recompute()
    fclib.show_only_tips(doc)


def add_pcb_ref(doc):
    """Link to the PCB reference document; the owner document must be saved first."""
    if not os.path.exists(PCB_DOC):
        print(f"{PCB_DOC} missing: PcbRef not created")
        return
    pcb_doc = App.openDocument(PCB_DOC, hidden=True) if App.GuiUp else App.openDocument(PCB_DOC)
    link = doc.addObject("App::Link", "PcbRef")
    link.Label = "PcbRef"
    link.LinkedObject = pcb_doc.getObject("PCB")
    link.setExpression(".Placement.Base.z", f"-{P}PcbThickness")
    doc.recompute()


def set_styles(doc):
    import FreeCADGui as Gui  # noqa: F401
    shell = (0.20, 0.21, 0.23)
    for name in ("Lid", "BottomPlate"):
        vo = doc.getObject(name).ViewObject
        vo.ShapeAppearance = [App.Material(DiffuseColor=shell)]
    vo = doc.getObject("LightPipes").ViewObject
    vo.ShapeAppearance = [App.Material(DiffuseColor=(0.92, 0.95, 1.0))]
    vo.Transparency = 40


def main():
    if os.path.exists(OUT):
        raise SystemExit(f"{OUT} already exists: use fcedit.py to modify it")
    doc = App.newDocument("devhub_enclosure")
    build(doc)
    bad = [o.Name for o in doc.Objects if "Invalid" in o.State or "Error" in o.State]
    if bad:
        raise SystemExit(f"recompute errors: {bad}")
    doc.saveAs(OUT)
    add_pcb_ref(doc)
    if App.GuiUp:
        set_styles(doc)
    doc.save()


if App.GuiUp:
    # Run from the GUI: always quit, reporting any failure on stderr and in the exit code.
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
