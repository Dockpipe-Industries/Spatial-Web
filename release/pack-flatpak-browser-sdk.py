#!/usr/bin/env python3
"""Pack the independently qualified Flatpak Browser SDK, without Qt runtime files."""
import argparse
import gzip
import importlib.util
import json
from pathlib import Path
import re
import tarfile
from flatpak_profile import CHECKS, PRODUCT, PLATFORM, STEM, read_stack

specification = importlib.util.spec_from_file_location("native_packer", Path(__file__).with_name("pack-sdk.py"))
native = importlib.util.module_from_spec(specification)
specification.loader.exec_module(native)


def pack(sdk, output, build_directory, qualification):
    stack = read_stack()
    proof = json.loads(qualification.read_text())
    if (proof.get("platform") != PLATFORM or proof.get("qt_components") != stack["qt_components"]
            or proof.get("flatpak") != stack["flatpak"] or proof.get("compiler") != stack["compiler"]
            or not CHECKS.issubset(proof.get("checks", []))
            or not re.fullmatch(r"[0-9a-f]{40}", proof.get("source_sha", ""))
            or proof.get("source_state") not in {"committed-snapshot", "local-proof"}):
        raise ValueError("Flatpak SDK lacks exact stack and installed runtime qualification")
    if native.compiler_identity(build_directory) != stack["compiler"]:
        raise ValueError("Actual build compiler differs from the qualified stack")
    required = sdk / "lib/cmake/SpatialWebFlatpakBrowser/SpatialWebFlatpakBrowserConfig.cmake"
    if not required.is_file() or (sdk / "lib/cmake/SpatialWebBrowser").exists():
        raise ValueError("Flatpak SDK must export its separate package, not the native ABI")
    output.mkdir(parents=True, exist_ok=True)
    entries = []
    for path in sorted(sdk.rglob("*")):
        if path.is_dir():
            if path.is_symlink():
                raise ValueError("Linked SDK directories are not admitted")
            continue
        resolved = path.resolve(strict=True)
        if not resolved.is_relative_to(sdk.resolve()) or not resolved.is_file():
            raise ValueError("SDK aliases must stay within the installed prefix")
        relative = path.relative_to(sdk).as_posix()
        if any(part.lower().startswith(("qt6", "libqt6", "spatialpipe", "forgepipe")) for part in path.relative_to(sdk).parts):
            raise ValueError("Flatpak Browser SDK must not bundle Qt or spatial runtimes")
        entries.append({"path": relative, "bytes": resolved.stat().st_size, "sha256": native.digest(resolved)})
    if len(entries) > 10000 or sum(entry["bytes"] for entry in entries) > 512 << 20:
        raise ValueError("Flatpak SDK exceeds its installed capacity")
    inventory = output / (STEM + ".files.json")
    inventory.write_text(json.dumps({"schema": 1, "artifact_class": "sdk", "files": entries}, indent=2) + "\n")
    archive = output / (STEM + ".tar.gz")
    with archive.open("xb") as target, gzip.GzipFile(fileobj=target, mode="wb", filename="", mtime=0) as compressed:
        with tarfile.open(fileobj=compressed, mode="w") as bundle:
            for entry in entries:
                path = sdk / entry["path"]
                member = tarfile.TarInfo(entry["path"])
                member.size = entry["bytes"]
                member.mode = 0o755 if path.stat().st_mode & 0o111 else 0o644
                with path.open("rb") as source:
                    bundle.addfile(member, source)
    metadata = {
        "schema": 1, "product": PRODUCT, "version": "0.1.0", "platform": PLATFORM,
        "source_repository": "Dockpipe-Industries/Spatial-Web", "source_sha": proof["source_sha"],
        "source_state": proof["source_state"], "runtime_bundled": False,
        "compiler": stack["compiler"], "abi": {"cxx_standard": 17},
        "qt_components": stack["qt_components"], "flatpak": stack["flatpak"],
        "archive": {"name": archive.name, "size_bytes": archive.stat().st_size, "sha256": native.digest(archive)},
        "file_manifest": {"name": inventory.name, "sha256": native.digest(inventory)},
        "checks": sorted(CHECKS),
    }
    (output / (STEM + ".release.json")).write_text(json.dumps(metadata, indent=2) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--build-directory", type=Path, required=True)
    parser.add_argument("--qualification", type=Path, required=True)
    arguments = parser.parse_args()
    pack(arguments.sdk, arguments.output, arguments.build_directory, arguments.qualification)
