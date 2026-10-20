"""Admission of the separately qualified official Flatpak Browser ABI."""
import json
from pathlib import Path
import re

PLATFORM = "linux-amd64-flatpak-kde-6.10"
PRODUCT = "SpatialWebFlatpakBrowserSDK"
STEM = PRODUCT + "-0.1.0-" + PLATFORM
LOCK = Path(__file__).resolve().parent / "flatpak-stack.lock.json"
COMPONENTS = {
    "Qt6Core": "6.10.3", "Qt6Gui": "6.10.3", "Qt6Widgets": "6.10.3",
    "Qt6Network": "6.10.3", "Qt6PrintSupport": "6.10.3",
    "Qt6WebEngineCore": "6.10.2", "Qt6WebEngineWidgets": "6.10.2",
}
REFS = {
    "sdk": "runtime/org.kde.Sdk/x86_64/6.10",
    "runtime": "runtime/org.kde.Platform/x86_64/6.10",
    "base": "app/io.qt.qtwebengine.BaseApp/x86_64/6.10",
}
CHECKS = {
    "sdk-component-install", "focused-browser-tests",
    "relocated-external-consumer-compile-link-run", "flatpak-installed-runtime-dom",
}


def read_stack(path=LOCK):
    if path.is_symlink() or not path.is_file() or path.stat().st_size > 64 << 10:
        raise ValueError("Flatpak stack requires a bounded regular lock")
    stack = json.loads(path.read_text())
    if (stack.get("schema") != 1 or stack.get("platform") != PLATFORM
            or stack.get("profile") != "flatpak-kde-6.10"
            or stack.get("qt_components") != COMPONENTS
            or stack.get("compiler") != {"id": "GNU", "version": "15.2.0"}):
        raise ValueError("Flatpak stack differs from the exact qualified profile")
    identities = stack.get("flatpak", {})
    if set(identities) != set(REFS):
        raise ValueError("Flatpak stack must bind SDK, Platform and BaseApp")
    for key, reference in REFS.items():
        item = identities[key]
        if (set(item) != {"ref", "commit"} or item.get("ref") != reference
                or not re.fullmatch(r"[0-9a-f]{64}", item.get("commit", ""))):
            raise ValueError("Flatpak reference requires an exact OSTree commit")
    return stack
