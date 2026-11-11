"""Runs inside freecadcmd, called by fcedit.py: applies an edit script to a document.

Environment: FCEDIT_DOC (document), FCEDIT_SCRIPT (edit script), FCEDIT_REPORT (json
report written here), FCEDIT_SAVE ("1" to save).

The edit script runs with these globals: App, doc, Params (the VarSet), fclib.
"""

import json
import os
import sys
import traceback

import FreeCAD as App

HERE = os.path.dirname(os.path.abspath(os.environ["FCEDIT_RUNNER"]))
sys.path.insert(0, HERE)
import fclib  # noqa: E402

report = {"errors": [], "visibility": {}, "new": [], "removed": [], "saved": False}
out = os.environ["FCEDIT_REPORT"]


def bad_objects(doc):
    return [f"{o.Name}: {o.State}" for o in doc.Objects
            if "Invalid" in o.State or "Error" in o.State]


try:
    doc = App.openDocument(os.environ["FCEDIT_DOC"])
    doc.recompute()
    pre_errors = bad_objects(doc)
    names_before = {o.Name for o in doc.Objects}
    vis_before = {o.Name: o.Visibility for o in doc.Objects}
    tips_before = {b.Name: (b.Tip.Name if b.Tip else None)
                   for b in doc.findObjects("PartDesign::Body")}

    g = {"App": App, "doc": doc, "Params": doc.getObject("Params"), "fclib": fclib,
         "__name__": "__fcedit__"}
    script = os.environ["FCEDIT_SCRIPT"]
    exec(compile(open(script).read(), script, "exec"), g)
    doc.recompute()
    # A feature recomputed before one of its new dependencies may fail once; the GUI
    # would fix it with another recompute, so retry a couple of times before giving up.
    for _ in range(2):
        failing = [o for o in doc.Objects if "Invalid" in o.State or "Error" in o.State]
        if not failing:
            break
        for o in failing:
            o.touch()
        doc.recompute()

    errors = [e for e in bad_objects(doc) if e not in pre_errors]
    report["errors"] = errors
    report["pre_existing_errors"] = pre_errors

    # Mimic the GUI: in a body whose tip moved, show only the new tip; hide new sketches.
    for b in doc.findObjects("PartDesign::Body"):
        tip = b.Tip.Name if b.Tip else None
        if tips_before.get(b.Name, "<new>") != tip:
            for o in b.Group:
                if o.isDerivedFrom("PartDesign::Feature") or o.isDerivedFrom("Sketcher::SketchObject"):
                    o.Visibility = (o == b.Tip)
    names_after = {o.Name for o in doc.Objects}
    report["new"] = sorted(names_after - names_before)
    report["removed"] = sorted(names_before - names_after)
    for o in doc.Objects:
        if o.Name in vis_before and vis_before[o.Name] != o.Visibility:
            report["visibility"][o.Name] = o.Visibility
    # New PartDesign features take the view style of the feature they build on, as the
    # GUI commands do; fcedit.py copies the view provider accordingly.
    report["inherit"] = {}
    report["new_visibility"] = {}
    for name in report["new"]:
        o = doc.getObject(name)
        report["new_visibility"][name] = o.Visibility
        if o.isDerivedFrom("PartDesign::Feature") and not o.isDerivedFrom("PartDesign::Body"):
            base = getattr(o, "BaseFeature", None)
            if base is not None:
                report["inherit"][name] = base.Name

    if not errors and os.environ.get("FCEDIT_SAVE") == "1":
        doc.save()
        report["saved"] = True
except Exception:
    report["errors"].append(traceback.format_exc())

with open(out, "w") as f:
    json.dump(report, f, indent=1)
