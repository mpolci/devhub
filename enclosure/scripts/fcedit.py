#!/usr/bin/env python3
"""Apply an edit script to an existing .FCStd with freecadcmd, keeping the GUI styles.

freecadcmd saves only Document.xml and the shapes: GuiDocument.xml (colours,
transparency, display modes, camera) would be lost. This tool saves it before the edit
and puts it back afterwards, with these adjustments:
- visibility changed by the edit script (or by a moved body tip) is written into it;
- view providers of removed objects are dropped;
- a new PartDesign feature gets a copy of the view provider of its base feature (colour,
  transparency), as the GUI commands do;
- other new objects get FreeCAD's default view properties when the file is opened.

Usage:
    python3 enclosure/scripts/fcedit.py [--dry-run] enclosure/devhub-enclosure.FCStd EDIT.py

The document must be closed in FreeCAD (or at least not saved while the edit runs: the
tool refuses to overwrite a file that changed in the meantime). A copy of the original
goes to enclosure/backup/ before every write.
"""

import argparse
import copy
import datetime
import json
import os
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
FREECADCMD = os.environ.get("FREECADCMD", "/Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd")
GUI_XML = "GuiDocument.xml"


def gui_files(xml_bytes):
    root = ET.fromstring(xml_bytes)
    return root, sorted({e.get("file") for e in root.iter() if e.get("file")})


def set_visibility(vp, value):
    for p in vp.iter("Property"):
        if p.get("name") == "Visibility":
            b = p.find("Bool")
            if b is not None:
                b.set("value", "true" if value else "false")


def inherit_styles(root, report, blobs):
    """Copy the view provider of the base feature to each new PartDesign feature."""
    data = root.find("ViewProviderData")
    if data is None:
        return
    vps = {vp.get("name"): vp for vp in data.findall("ViewProvider")}
    pending = dict(report.get("inherit", {}))
    # A new feature may build on another new one: resolve in dependency order.
    while pending:
        progress = False
        for new, src in list(pending.items()):
            if src in pending:
                continue
            del pending[new]
            progress = True
            if src not in vps or new in vps:
                continue
            vp = copy.deepcopy(vps[src])
            vp.set("name", new)
            for e in vp.iter():
                f = e.get("file")
                if f and f in blobs:
                    nf = f"{f}_{new}"
                    blobs[nf] = blobs[f]
                    e.set("file", nf)
            set_visibility(vp, report["new_visibility"].get(new, False))
            data.append(vp)
            vps[new] = vp
        if not progress:
            break


def patch_gui(root, report, names_in_doc):
    data = root.find("ViewProviderData")
    if data is not None:
        for vp in list(data.findall("ViewProvider")):
            name = vp.get("name")
            if name not in names_in_doc:
                data.remove(vp)
                continue
            if name in report["visibility"]:
                set_visibility(vp, report["visibility"][name])
        data.set("Count", str(len(data.findall("ViewProvider"))))


def doc_object_names(doc_xml):
    root = ET.fromstring(doc_xml)
    objs = root.find("Objects")
    return {o.get("name") for o in objs.findall("Object")} if objs is not None else set()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry-run", action="store_true", help="run and check, do not save")
    ap.add_argument("doc")
    ap.add_argument("script")
    a = ap.parse_args()

    doc = os.path.abspath(a.doc)
    script = os.path.abspath(a.script)
    mtime = os.stat(doc).st_mtime_ns

    with zipfile.ZipFile(doc) as z:
        old_names = set(z.namelist())
        gui_xml = z.read(GUI_XML) if GUI_XML in old_names else None
        thumb = z.read("thumbnails/Thumbnail.png") if "thumbnails/Thumbnail.png" in old_names else None
        gui_root, refs = gui_files(gui_xml) if gui_xml else (None, [])
        gui_blobs = {r: z.read(r) for r in refs if r in old_names}

    with tempfile.TemporaryDirectory() as tmp:
        # Sibling copy: relative links (PcbRef -> pcb/devhub-pcb.FCStd) keep resolving.
        work = os.path.join(os.path.dirname(doc), ".fcedit-work.FCStd")
        shutil.copy2(doc, work)
        try:
            run(a, doc, script, work, tmp, mtime, gui_root, refs, gui_blobs, thumb)
        finally:
            if os.path.exists(work):
                os.remove(work)


def run(a, doc, script, work, tmp, mtime, gui_root, refs, gui_blobs, thumb):
    rep = os.path.join(tmp, "report.json")
    env = dict(os.environ, FCEDIT_DOC=work, FCEDIT_SCRIPT=script, FCEDIT_REPORT=rep,
               FCEDIT_RUNNER=os.path.join(HERE, "fcrun.py"),
               FCEDIT_SAVE="0" if a.dry_run else "1")
    runner = os.path.join(HERE, "fcrun.py")
    proc = subprocess.run([FREECADCMD, "-c", f"exec(open({runner!r}).read())"], env=env,
                          capture_output=True, text=True)
    if not os.path.exists(rep):
        sys.stderr.write(proc.stdout[-4000:] + proc.stderr[-4000:])
        sys.exit("freecadcmd produced no report")
    report = json.load(open(rep))
    print(json.dumps({k: report[k] for k in ("new", "removed", "visibility")}, indent=1))
    if report.get("pre_existing_errors"):
        print("objects already in error before the edit:", report["pre_existing_errors"])
    if report["errors"]:
        print("EDIT FAILED, document not modified:")
        for e in report["errors"]:
            print(e)
        sys.exit(1)
    if a.dry_run:
        print("dry run OK, document not modified")
        return

    with zipfile.ZipFile(work) as z:
        app_order = z.namelist()
        entries = {n: z.read(n) for n in app_order}
    names = doc_object_names(entries["Document.xml"])
    gui_order = []
    if gui_root is not None:
        # Gui blobs keep their names unless an App file took the same name.
        for r in refs:
            if r in gui_blobs and r in entries:
                k = 1
                while f"{r}_gui{k}" in entries:
                    k += 1
                for e in gui_root.iter():
                    if e.get("file") == r:
                        e.set("file", f"{r}_gui{k}")
                gui_blobs[f"{r}_gui{k}"] = gui_blobs.pop(r)
        inherit_styles(gui_root, report, gui_blobs)
        patch_gui(gui_root, report, names)
        entries[GUI_XML] = ET.tostring(gui_root, encoding="utf-8", xml_declaration=True)
        # FreeCAD reads the archive sequentially, in the order the files are referenced:
        # App files, then GuiDocument.xml, then the Gui files in document order.
        gui_order = [GUI_XML]
        for e in gui_root.iter():
            f = e.get("file")
            if f and f in gui_blobs and f not in gui_order:
                gui_order.append(f)
                entries[f] = gui_blobs[f]
    if thumb is not None and "thumbnails/Thumbnail.png" not in entries:
        entries["thumbnails/Thumbnail.png"] = thumb
        gui_order.append("thumbnails/Thumbnail.png")

    out = os.path.join(tmp, "out.FCStd")
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        for n in app_order + gui_order:
            z.writestr(n, entries[n])

    if os.stat(doc).st_mtime_ns != mtime:
        sys.exit("the document changed on disk during the edit: nothing written")
    bdir = os.path.join(os.path.dirname(doc), "backup")
    os.makedirs(bdir, exist_ok=True)
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    shutil.copy2(doc, os.path.join(bdir, f"{os.path.splitext(os.path.basename(doc))[0]}-{stamp}.FCStd"))
    shutil.move(out, doc)
    print("saved", doc)


if __name__ == "__main__":
    main()
