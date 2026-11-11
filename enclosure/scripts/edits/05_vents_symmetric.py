"""Edit 05: side vents across the PCB, symmetric on the closed box.

Both slot ends are VentEndMargin from the outer faces of the closed box (lid top face and
plate bottom face). The slots now reach above the PCB, so the hot layer under the board
and the chamber above it are both vented.

Globals from fcrun.py: App, doc, Params, fclib.
"""

from fclib import add_param

add_param(Params, "VentEndMargin", "App::PropertyLength", "Ventilation", 6.7,
          "Distance of both slot ends from the outer top and bottom faces of the closed box")
add_param(Params, "BoxBottomZ", "App::PropertyDistance", "Derived", None,
          "Outer bottom face of the closed box", expr="Params.PlateTopZ - Params.PlateThickness")
doc.recompute()

Params.setExpression("VentTopZ", "Params.TopZ - Params.VentEndMargin - Params.VentSlotWidth / 2")
Params.setExpression("VentBottomZ", "Params.BoxBottomZ + Params.VentEndMargin + Params.VentSlotWidth / 2")
Params.setDocumentationOfProperty("VentTopZ", "Vent slot upper arc centre")
Params.setDocumentationOfProperty("VentBottomZ", "Vent slot lower arc centre")
doc.recompute()
for n in ("VentTopBelowPcb", "VentBottomAbovePlate"):
    if n in Params.PropertiesList:
        Params.removeProperty(n)
