"""Helpers for building parametric PartDesign features driven by the Params VarSet.

Every sketch dimension is a constraint bound to an expression, so the model follows the
VarSet when a parameter changes. All helpers only add objects; none of them deletes or
recreates existing ones.

Coordinate frame of the enclosure document:
  X = u, from the left edge of the PCB (seen from above, front side at the bottom)
  Y = v, from the front edge of the PCB (the side with the USB-C ports and banana jacks)
  Z = 0 on the top face of the PCB, positive upward
"""

import FreeCAD as App
import Part
import Sketcher

V = App.Vector
ROOT = -1  # sketch root point (origin) geometry index


# --------------------------------------------------------------------------- params

def add_param(vs, name, ptype, group, value, doc, expr=None):
    """Add a property to the VarSet if missing; never touches an existing one."""
    if name in vs.PropertiesList:
        return False
    vs.addProperty(ptype, name, group, doc)
    if expr is None:
        setattr(vs, name, value)
    else:
        vs.setExpression(name, expr)
    return True


def ev(obj, expr):
    """Evaluate an expression in the context of obj, returning a float in mm."""
    q = obj.evalExpression(expr)
    return float(q.Value) if hasattr(q, "Value") else float(q)


# --------------------------------------------------------------------------- sketches

def new_sketch(body, name, plane, offset_expr="0 mm", label=None):
    """Sketch attached to an origin plane of the body, shifted along its normal.

    plane: 'XY' (x->X, y->Y, normal +Z), 'XZ' (x->X, y->Z, normal -Y),
           'YZ' (x->Y, y->Z, normal +X).
    """
    sk = body.newObject("Sketcher::SketchObject", name)
    sk.Label = label or name
    feat = {"XY": "XY_Plane", "XZ": "XZ_Plane", "YZ": "YZ_Plane"}[plane]
    origin_plane = [f for f in body.Origin.OriginFeatures if f.Role == feat][0]
    sk.AttachmentSupport = [(origin_plane, "")]
    sk.MapMode = "FlatFace"
    sk.setExpression(".AttachmentOffset.Base.z", offset_expr)
    return sk


class SketchBuilder:
    """Adds fully constrained geometry whose dimensions are expressions."""

    def __init__(self, sketch, ctx):
        self.sk = sketch
        self.ctx = ctx  # object used to evaluate expressions for initial geometry

    def v(self, expr):
        return ev(self.ctx, expr)

    def _con(self, c, expr=None, name=None):
        i = self.sk.addConstraint(c)
        if name:
            self.sk.renameConstraint(i, name)
        if expr is not None:
            self.sk.setExpression(f"Constraints[{i}]", expr)
        return i

    def _place_point(self, geo, pos, x_expr, y_expr, name=None):
        x, y = self.v(x_expr), self.v(y_expr)
        self._con(Sketcher.Constraint("DistanceX", ROOT, 1, geo, pos, x), x_expr,
                  f"{name}_x" if name else None)
        self._con(Sketcher.Constraint("DistanceY", ROOT, 1, geo, pos, y), y_expr,
                  f"{name}_y" if name else None)

    def circle(self, cx, cy, d, name=None):
        g = self.sk.addGeometry(Part.Circle(V(self.v(cx), self.v(cy), 0), V(0, 0, 1),
                                            self.v(d) / 2), False)
        self._place_point(g, 3, cx, cy, name)
        self._con(Sketcher.Constraint("Diameter", g, self.v(d)), d,
                  f"{name}_d" if name else None)
        return g

    def rect(self, x0, y0, x1, y1, name=None):
        a, b, c, d = self.v(x0), self.v(y0), self.v(x1), self.v(y1)
        pts = [V(a, b, 0), V(c, b, 0), V(c, d, 0), V(a, d, 0)]
        g = [self.sk.addGeometry(Part.LineSegment(pts[i], pts[(i + 1) % 4]), False)
             for i in range(4)]
        for i in range(4):
            self._con(Sketcher.Constraint("Coincident", g[i], 2, g[(i + 1) % 4], 1))
        self._con(Sketcher.Constraint("Horizontal", g[0]))
        self._con(Sketcher.Constraint("Horizontal", g[2]))
        self._con(Sketcher.Constraint("Vertical", g[1]))
        self._con(Sketcher.Constraint("Vertical", g[3]))
        self._place_point(g[0], 1, x0, y0, f"{name}_p0" if name else None)
        self._place_point(g[2], 1, x1, y1, f"{name}_p1" if name else None)
        return g

    def rounded_rect(self, x0, y0, x1, y1, r, name=None):
        a, b, c, d, rr = (self.v(e) for e in (x0, y0, x1, y1, r))
        L = [
            Part.LineSegment(V(a + rr, b, 0), V(c - rr, b, 0)),  # bottom
            Part.LineSegment(V(c, b + rr, 0), V(c, d - rr, 0)),  # right
            Part.LineSegment(V(c - rr, d, 0), V(a + rr, d, 0)),  # top
            Part.LineSegment(V(a, d - rr, 0), V(a, b + rr, 0)),  # left
        ]
        import math
        A = [
            Part.ArcOfCircle(Part.Circle(V(c - rr, b + rr, 0), V(0, 0, 1), rr), -math.pi / 2, 0),
            Part.ArcOfCircle(Part.Circle(V(c - rr, d - rr, 0), V(0, 0, 1), rr), 0, math.pi / 2),
            Part.ArcOfCircle(Part.Circle(V(a + rr, d - rr, 0), V(0, 0, 1), rr), math.pi / 2, math.pi),
            Part.ArcOfCircle(Part.Circle(V(a + rr, b + rr, 0), V(0, 0, 1), rr), math.pi, 1.5 * math.pi),
        ]
        gl = [self.sk.addGeometry(x, False) for x in L]
        ga = [self.sk.addGeometry(x, False) for x in A]
        for i in range(4):
            self._con(Sketcher.Constraint("Tangent", gl[i], 2, ga[i], 1))
            self._con(Sketcher.Constraint("Tangent", ga[i], 2, gl[(i + 1) % 4], 1))
        self._con(Sketcher.Constraint("Horizontal", gl[0]))
        self._con(Sketcher.Constraint("Horizontal", gl[2]))
        self._con(Sketcher.Constraint("Vertical", gl[1]))
        self._con(Sketcher.Constraint("Vertical", gl[3]))
        for i in range(3):
            self._con(Sketcher.Constraint("Equal", ga[i], ga[i + 1]))
        self._con(Sketcher.Constraint("Radius", ga[0], rr), r, f"{name}_r" if name else None)
        n = (lambda s: f"{name}_{s}") if name else (lambda s: None)
        self._con(Sketcher.Constraint("DistanceX", ROOT, 1, gl[3], 1, a), x0, n("x0"))
        self._con(Sketcher.Constraint("DistanceY", ROOT, 1, gl[0], 1, b), y0, n("y0"))
        self._con(Sketcher.Constraint("DistanceX", ROOT, 1, gl[1], 1, c), x1, n("x1"))
        self._con(Sketcher.Constraint("DistanceY", ROOT, 1, gl[2], 1, d), y1, n("y1"))
        return gl + ga

    def slot_v(self, cx, y_top, y_bottom, w, name=None):
        """Vertical stadium: arc centres at (cx, y_top) and (cx, y_bottom), width w."""
        import math
        x, yt, yb, ww = (self.v(e) for e in (cx, y_top, y_bottom, w))
        r = ww / 2
        gr = self.sk.addGeometry(Part.LineSegment(V(x + r, yb, 0), V(x + r, yt, 0)), False)
        ga0 = self.sk.addGeometry(Part.ArcOfCircle(Part.Circle(V(x, yt, 0), V(0, 0, 1), r), 0, math.pi), False)
        gl = self.sk.addGeometry(Part.LineSegment(V(x - r, yt, 0), V(x - r, yb, 0)), False)
        ga1 = self.sk.addGeometry(Part.ArcOfCircle(Part.Circle(V(x, yb, 0), V(0, 0, 1), r), math.pi, 2 * math.pi), False)
        self._con(Sketcher.Constraint("Tangent", gr, 2, ga0, 1))
        self._con(Sketcher.Constraint("Tangent", ga0, 2, gl, 1))
        self._con(Sketcher.Constraint("Tangent", gl, 2, ga1, 1))
        self._con(Sketcher.Constraint("Tangent", ga1, 2, gr, 1))
        self._con(Sketcher.Constraint("Vertical", gr))
        self._con(Sketcher.Constraint("Equal", ga0, ga1))
        self._con(Sketcher.Constraint("Diameter", ga0, ww), w, f"{name}_w" if name else None)
        self._place_point(ga0, 3, cx, y_top, f"{name}_top" if name else None)
        self._con(Sketcher.Constraint("DistanceY", ga1, 3, ga0, 3, yt - yb),
                  f"({y_top}) - ({y_bottom})", f"{name}_len" if name else None)
        return [gr, ga0, gl, ga1]


# --------------------------------------------------------------------------- features

def pad(body, name, sketch, length_expr, reversed_=False, midplane=False, label=None):
    f = body.newObject("PartDesign::Pad", name)
    f.Label = label or name
    f.Profile = sketch
    f.Type = "Length"
    f.setExpression("Length", length_expr)
    f.Reversed = reversed_
    if midplane:
        f.SideType = "Symmetric"
    f.Refine = True
    return f


def pocket(body, name, sketch, length_expr=None, reversed_=False, midplane=False,
           through_all=False, taper_expr=None, label=None):
    f = body.newObject("PartDesign::Pocket", name)
    f.Label = label or name
    f.Profile = sketch
    if through_all:
        f.Type = "ThroughAll"
    else:
        f.Type = "Length"
        f.setExpression("Length", length_expr)
    f.Reversed = reversed_
    if midplane:
        f.SideType = "Symmetric"
    if taper_expr is not None:
        f.setExpression("TaperAngle", taper_expr)
    f.Refine = True
    return f


def linear_pattern(body, name, original, direction, offset_expr, count_expr, label=None):
    f = body.newObject("PartDesign::LinearPattern", name)
    f.Label = label or name
    f.Originals = list(original) if isinstance(original, (list, tuple)) else [original]
    f.Direction = direction
    f.Mode = "Spacing"
    f.setExpression("Offset", offset_expr)
    f.setExpression("Occurrences", count_expr)
    f.Refine = True
    body.Tip = f  # newObject does not move the tip to transformed features
    return f


def show_only_tips(doc):
    """Body features hidden except the tip, sketches hidden (as the GUI does)."""
    for body in doc.findObjects("PartDesign::Body"):
        for o in body.Group:
            if o.isDerivedFrom("PartDesign::Feature") or o.isDerivedFrom("Sketcher::SketchObject"):
                o.Visibility = (o == body.Tip)


def remove_feature(doc, name):
    """Remove a body feature or sketch, relinking the next feature to its base."""
    o = doc.getObject(name)
    if o is None:
        return False
    body = o.getParentGeoFeatureGroup()
    if body is not None and body.isDerivedFrom("PartDesign::Body"):
        body.removeObject(o)
    doc.removeObject(name)
    return True


def set_constraint_expr(sketch, constraint_name, expr):
    """Rebind a named sketch constraint to a new expression."""
    names = [c.Name for c in sketch.Constraints]
    sketch.setExpression(f"Constraints[{names.index(constraint_name)}]", expr)
