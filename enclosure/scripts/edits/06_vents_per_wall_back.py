"""Edit 06: vents cut wall by wall, more side slots, back vents between J1 and J2.

- The side vent pocket went through all along X: once the slots reached above the PCB
  (edit 05) it also cut the H1 and H2 standoffs. Each wall now has its own pocket, as
  long as the wall plus 0.5 mm on each side.
- Side slots: VentCount 14 per side, centred on the PCB depth.
- Back wall: BackVentCount slots centred between J1 and J2, near the LDO U1 and D2,
  same height and pitch as the side slots.

Globals from fcrun.py: App, doc, Params, fclib.
"""

from fclib import SketchBuilder, new_sketch, pocket, linear_pattern, add_param, remove_feature

P = "Params."
lid = doc.getObject("Lid")
cut = f"{P}WallThickness + 1 mm"

add_param(Params, "BackVentCount", "App::PropertyInteger", "Ventilation", 8,
          "Back wall slots, centred between J1 and J2")
add_param(Params, "BackVentFirstX", "App::PropertyDistance", "Derived", None,
          "First back slot X",
          expr=f"({P}J1X + {P}J2X) / 2 - ({P}BackVentCount - 1) * {P}VentPitch / 2")
Params.VentCount = 14
Params.setExpression("VentFirstY", f"{P}PcbDepth / 2 - ({P}VentCount - 1) * {P}VentPitch / 2")
Params.setDocumentationOfProperty("VentFirstY", "First side slot Y (centred on the PCB depth)")
doc.recompute()

# Left wall: the existing sketch moves from the PCB centre to the wall mid-plane.
remove_feature(doc, "VentSlots")
left_sk = doc.getObject("VentSketch")
left_sk.setExpression(".AttachmentOffset.Base.z", f"-({P}SideGapLeft + {P}WallThickness / 2)")
left = doc.getObject("VentSlot")
left.Type = "Length"
left.setExpression("Length", cut)

sk = new_sketch(lid, "VentRightSketch", "YZ",
                f"{P}PcbWidth + {P}SideGapRight + {P}WallThickness / 2")
SketchBuilder(sk, Params).slot_v(f"{P}VentFirstY", f"{P}VentTopZ", f"{P}VentBottomZ",
                                 f"{P}VentSlotWidth", "vent")
right = pocket(lid, "VentSlotRight", sk, cut, midplane=True)

sk = new_sketch(lid, "BackVentSketch", "XZ",
                f"-({P}PcbDepth + {P}FrontBackGap + {P}WallThickness / 2)")
SketchBuilder(sk, Params).slot_v(f"{P}BackVentFirstX", f"{P}VentTopZ", f"{P}VentBottomZ",
                                 f"{P}VentSlotWidth", "vent")
back = pocket(lid, "BackVentSlot", sk, cut, midplane=True)

linear_pattern(lid, "VentSlots", [left, right], (left_sk, ["H_Axis"]), f"{P}VentPitch", f"{P}VentCount")
linear_pattern(lid, "BackVentSlots", back, (sk, ["H_Axis"]), f"{P}VentPitch", f"{P}BackVentCount")
