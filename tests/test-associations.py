"""Package ownership and association repair tests, confined to temporary roots."""
import io
import os
from pathlib import Path
import subprocess
import tarfile
import tempfile

repo = Path(__file__).resolve().parents[1]
repair = repo / "DIST/restore-ipk-association.sh"
package = repo / "DIST/zaurusmd_0.7_arm.ipk"
checks = 0


def run(script, root, *args):
    subprocess.run(["sh", str(script), *args], check=True, capture_output=True,
                   env=dict(os.environ, ZAURUSMD_ROOT=str(root)))


def check(condition, label):
    global checks
    assert condition, label
    checks += 1
    print("PASS", label)


def write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)


def seed(root):
    qt = root / "home/QtPalmtop"
    write(qt / "bin/qinstall", "#!/bin/sh\nexit 0\n")
    (qt / "bin/qinstall").chmod(0o755)
    write(qt / "etc/mime.types", "# user custom types\ntext/plain\ttxt asc\napplication/x-user\tcustom\napplication/octet-stream bin ipk exe # keep ipk comment\n")
    write(root / "home/zaurus/Settings/mime.types", "text/plain txt\nimage/jpeg jpg jpeg\n")
    write(qt / "etc/slmime.types", "# custom categories\nMyImages image/\n")
    write(qt / "apps/Applications/textedit.desktop", "[Desktop Entry]\nExec=textedit\nMimeType=text/*\n")
    write(root / "home/zaurus/Documents/Notes/keep.md", "# user note\n")
    return qt


def mime_paths(root):
    return [root / "home/QtPalmtop/etc/mime.types", root / "home/zaurus/Settings/mime.types"]


def ipk_mappings(path):
    result = []
    for line in path.read_text().splitlines():
        fields = line.split("#", 1)[0].split()
        if len(fields) > 1 and "ipk" in fields[1:]:
            result.append(fields[0])
    return result


def assert_restored(root):
    qt = root / "home/QtPalmtop"
    desktop = qt / "apps/Settings/qinstall.desktop"
    check(desktop.read_bytes() == (repo / "packaging/qinstall.desktop").read_bytes(), "Sharp qinstall restored exactly")
    for path in mime_paths(root):
        check(ipk_mappings(path) == ["application/ipkg"], "unique IPK mapping: " + str(path.relative_to(root)))
        check("text/plain" in path.read_text() and "txt" in path.read_text(), "TXT mapping retained")
    check((qt / "apps/Applications/textedit.desktop").is_file(), "text editor desktop retained")
    check((root / "home/zaurus/Documents/Notes/keep.md").read_text() == "# user note\n", "user document untouched")


with tarfile.open(package) as outer:
    payload = outer.extractfile("./data.tar.gz").read()
    control = outer.extractfile("./control.tar.gz").read()
with tarfile.open(fileobj=io.BytesIO(payload)) as data:
    members = data.getmembers()
owned = [m.name.removeprefix("./") for m in members if not m.isdir()]
check(not any("/apps/Settings/" in n or "/etc/" in n or "/home/zaurus/Settings/" in n for n in owned),
      "package does not own system launcher or MIME files")
with tarfile.open(fileobj=io.BytesIO(control)) as control_tar:
    control_text = control_tar.extractfile("./control").read().decode()
check("Depends:" not in control_text, "GUI install has no unsatisfied dependency gate")


def extract(root, hooks):
    with tarfile.open(fileobj=io.BytesIO(payload)) as data:
        data.extractall(root, filter="data")
    with tarfile.open(fileobj=io.BytesIO(control)) as data:
        data.extractall(hooks, filter="data")


def remove(root, hooks):
    run(hooks / "prerm", root, "remove")
    for name in owned:
        (root / name).unlink(missing_ok=True)
    for member in sorted((m for m in members if m.isdir()), key=lambda m: len(m.name), reverse=True):
        try:
            (root / member.name.removeprefix("./")).rmdir()
        except OSError:
            pass
    run(hooks / "postrm", root, "remove")


with tempfile.TemporaryDirectory(prefix="zaurus-association-test-") as directory:
    base = Path(directory)
    root, hooks = base / "device", base / "hooks"
    qt = seed(root)
    original_mime = (qt / "etc/mime.types").read_bytes()
    write(qt / "apps/Settings/qipkg.desktop", "[Desktop Entry]\nExec = qipkg\nMimeType=application/ipkg\n")
    run(repair, root)
    assert_restored(root)
    check(not (qt / "apps/Applications/zaurusmd.desktop").exists(), "repair without editor does not create a dead launcher")
    check(not (qt / "apps/Settings/qipkg.desktop").exists(), "incompatible qipkg launcher disabled")
    check((qt / "apps/Settings/qipkg.desktop.zaurusmd.disabled").exists(), "disabled launcher kept as backup")
    check((qt / "etc/mime.types.zaurusmd-before-ipk-fix").read_bytes() == original_mime, "original MIME backed up")
    check("application/x-user\tcustom" in (qt / "etc/mime.types").read_text(), "custom MIME formatting preserved")
    check("bin exe # keep ipk comment" in (qt / "etc/mime.types").read_text(), "only extension token removed, comment retained")
    check((qt / "etc/slmime.types").read_text().startswith("# custom categories"), "existing Sharp categories not reset")
    first = [p.read_bytes() for p in mime_paths(root)]
    run(repair, root)
    check(first == [p.read_bytes() for p in mime_paths(root)], "standalone repair is idempotent")

    extract(root, hooks)
    broken_script = qt / "bin/restore-file-associations"
    broken_script.write_text("#!/bin/sh\nexit 1\n")
    broken_script.chmod(0o755)
    run(hooks / "postinst", root, "configure")
    check(True, "postinst ignores non-critical association repair failures")
    extract(root, hooks)
    run(hooks / "postinst", root, "configure")
    for category in ("Applications", "Document"):
        entry = qt / f"apps/{category}/zaurusmd.desktop"
        check("Exec=zaurusmd\n" in entry.read_text(), "Qtopia executable has no literal %f argument")
        check(set(entry.read_text().split("MimeType=", 1)[1].splitlines()[0].split(";")) == {"text/markdown", "text/x-markdown"}, "both Markdown MIME aliases registered")
        entry.unlink()
    run(repair, root)
    check(all((qt / f"apps/{category}/zaurusmd.desktop").exists() for category in ("Applications", "Document")), "standalone repair restores missing editor launchers")
    check(all("# BEGIN zaurusmd MIME" in p.read_text() for p in mime_paths(root)), "installer adds marked Markdown mappings")
    first = [p.read_bytes() for p in mime_paths(root)]
    run(hooks / "postinst", root, "configure")
    check(first == [p.read_bytes() for p in mime_paths(root)], "repeated installation is idempotent")
    remove(root, hooks)
    assert_restored(root)
    check(all("# BEGIN zaurusmd MIME" not in p.read_text() for p in mime_paths(root)), "uninstall removes only its marked MIME additions")
    check(not (qt / "bin/zaurusmd").exists(), "uninstall actually removes editor")
    extract(root, hooks)
    run(hooks / "postinst", root, "configure")
    assert_restored(root)
    remove(root, hooks)
    assert_restored(root)

    # Older packages also owned both MIME files. Missing files need full defaults.
    for path in mime_paths(root):
        path.unlink()
    (qt / "apps/Settings/qinstall.desktop").unlink()
    run(repair, root)
    assert_restored(root)
    check(all("image/jpeg" in p.read_text() and "application/pdf" in p.read_text() for p in mime_paths(root)),
          "missing MIME files receive complete fallback, not an IPK-only file")

    # Preserve independent Markdown associations when this package is removed.
    for path in mime_paths(root):
        with path.open("a") as stream:
            stream.write("text/markdown md markdown mkd\n")
    extract(root, hooks)
    run(repair, root, "install")
    run(repair, root, "remove")
    check(all("text/markdown md markdown mkd" in p.read_text() for p in mime_paths(root)), "another app's Markdown mappings survive")

    # Qtopia can expose stock files through symlinks into its read-only ROM.
    rom = base / "rom-mime.types"
    rom.write_text("text/plain txt\napplication/ipkg ipk\n")
    target = mime_paths(root)[0]
    target.unlink()
    target.symlink_to(rom)
    run(repair, root)
    check(rom.read_text() == "text/plain txt\napplication/ipkg ipk\n", "ROM symlink target untouched")
    check(not target.is_symlink(), "repair installs a writable overlay MIME file")

print(f"PASS {checks} association and install/remove/reinstall checks")
