"""Exact source and archive identities for the optional Projection SDK."""
import hashlib
import json
from pathlib import Path
import re
from urllib.parse import urlsplit

LOCK_PATH = Path(__file__).resolve().parent / "projection-dependencies.lock.json"
PRODUCT_REPOSITORIES = {
    "SpatialPipe": "Dockpipe-Industries/SpatialPipe",
    "SpatialPipeScene": "Dockpipe-Industries/SpatialPipe",
    "ForgePipe": "Dockpipe-Industries/forgepipe",
}


def read_projection_lock(path=LOCK_PATH):
    if path.is_symlink() or not path.is_file() or path.stat().st_size > 64 << 10:
        raise ValueError("Projection requires a bounded regular dependency lock")
    content = path.read_bytes()
    lock = json.loads(content)
    if lock.get("schema") != 1 or lock.get("qt_version") != "6.10.3" or lock.get("platform") != "linux-amd64":
        raise ValueError("Projection lock must bind the qualified Linux amd64 Qt ABI")
    dependencies = lock.get("dependencies", {})
    if not isinstance(dependencies, dict) or set(dependencies) != set(PRODUCT_REPOSITORIES):
        raise ValueError("Projection requires exactly the installed runtime, scene and engine SDKs")
    for product, repository in PRODUCT_REPOSITORIES.items():
        entry = dependencies[product]
        if not isinstance(entry, dict):
            raise ValueError("Projection dependency identity must be an object")
        url = urlsplit(entry.get("url", ""))
        if (entry.get("version") != "0.1.0" or url.scheme != "https" or url.netloc != "github.com"
                or not url.path.startswith("/Dockpipe-Industries/dockpipe/releases/download/")
                or url.query or url.fragment or url.username or url.password
                or not re.fullmatch(r"[0-9a-f]{64}", entry.get("sha256", ""))
                or entry.get("source_repository") != repository
                or not re.fullmatch(r"[0-9a-f]{40}", entry.get("source_sha", ""))):
            raise ValueError("Dependency requires exact public archive, checksum and owning source identity")
        tag = url.path.split("/releases/download/", 1)[1].split("/", 1)[0]
        if entry["source_sha"][:12] not in tag:
            raise ValueError("Dependency release tag must bind its owning source revision")
        mirror = entry.get("public_mirror_target")
        if mirror is not None and not re.fullmatch(r"[0-9a-f]{40}", mirror):
            raise ValueError("Public mirror target must be an exact source revision")
    if dependencies["SpatialPipe"] != dependencies["SpatialPipeScene"]:
        raise ValueError("Runtime and Scene must share the qualified Scene SDK identity")
    identity = {"name": path.name, "sha256": hashlib.sha256(content).hexdigest()}
    return dependencies, identity
