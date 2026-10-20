#!/usr/bin/env python3
"""Pack an installed, tested SDK; Qt and spatial runtimes remain separate artifacts."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import tarfile
import os
import subprocess
import tempfile


def digest(path):
    checksum = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1 << 20), b""):
            checksum.update(block)
    return checksum.hexdigest()


def compiler_identity(build):
    candidates = list((build / "CMakeFiles").glob("*/CMakeCXXCompiler.cmake"))
    if len(candidates) != 1:
        raise ValueError("SDK requires one unambiguous native compiler identity")
    content = candidates[0].read_text()
    identity = {}
    for key, cmake_key in [("id", "CMAKE_CXX_COMPILER_ID"), ("version", "CMAKE_CXX_COMPILER_VERSION")]:
        match = re.search(r'set\(' + cmake_key + r' "([^"\n]+)"\)', content)
        if not match:
            raise ValueError("Native compiler identity is incomplete")
        identity[key] = match.group(1)
    return identity


def pack(sdk, output, variant, platform, source_sha, build_directory):
    if not re.fullmatch(r"[0-9a-f]{40}", source_sha):
        raise ValueError("SDK requires the exact committed source SHA")
    platforms = {"linux-amd64", "linux-arm64", "darwin-amd64", "darwin-arm64", "windows-amd64"}
    if platform not in platforms or variant not in {"browser", "projection"}:
        raise ValueError("Unsupported SDK platform or variant")
    if variant == "projection" and platform != "linux-amd64":
        raise ValueError("Projection currently requires the qualified Linux amd64 Scene ABI")
    if not (sdk / "include/SpatialWeb/Browser/BrowserWidget.h").is_file():
        raise ValueError("SDK lacks the browser public header")
    if variant == "browser" and (sdk / "lib/cmake/SpatialWebProjection").exists():
        raise ValueError("Ordinary Browser SDK cannot include optional Projection")
    product = "SpatialWebBrowserSDK" if variant == "browser" else "SpatialWebProjectionSDK"
    stem = product + "-0.1.0-" + platform
    output.mkdir(parents=True, exist_ok=True)
    files = []
    for path in sorted(sdk.rglob("*")):
        if path.is_dir():
            if path.is_symlink():
                raise ValueError("SDK directory aliases are not supported")
            continue
        resolved = path.resolve(strict=True)
        if not resolved.is_relative_to(sdk.resolve()) or not resolved.is_file():
            raise ValueError("SDK file aliases must remain inside the installed prefix")
        files.append({"path": path.relative_to(sdk).as_posix(), "bytes": resolved.stat().st_size, "sha256": digest(resolved)})
    if len(files) > 10000 or sum(item["bytes"] for item in files) > 512 << 20:
        raise ValueError("SDK exceeds bounded archive capacity")
    inventory = output / ("sdk-files.json" if variant == "projection" else stem + ".files.json")
    inventory.write_text(json.dumps({"schema": 1, "artifact_class": "sdk", "files": files}, indent=2) + "\n")
    archive = output / (stem + ".tar.gz")
    with archive.open("xb") as target, gzip.GzipFile(fileobj=target, mode="wb", filename="", mtime=0) as compressed:
        with tarfile.open(fileobj=compressed, mode="w") as bundle:
            for entry in files:
                path = sdk / entry["path"]
                item = tarfile.TarInfo(entry["path"])
                item.size = entry["bytes"]
                item.mode = 0o755 if path.stat().st_mode & 0o111 else 0o644
                with path.open("rb") as source:
                    bundle.addfile(item, source)
    with tempfile.TemporaryDirectory(prefix="spatial-web-archive-proof-", dir=sdk.parent) as temporary:
        proof = Path(temporary)
        relocated = proof / "sdk"
        with tarfile.open(archive) as bundle:
            bundle.extractall(relocated, filter="data")
        source = Path(__file__).resolve().parents[1] / "tests/consumer"
        optional = "ON" if variant == "projection" else "OFF"
        subprocess.run(["cmake", "-S", str(source), "-B", str(proof / "consumer"),
            "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL", "-DCMAKE_PREFIX_PATH=" + str(relocated),
            "-DSPATIAL_WEB_TEST_COMPOSITION=" + optional,
            "-DSPATIAL_WEB_TEST_PROJECTION=" + optional], check=True, timeout=120)
        subprocess.run(["cmake", "--build", str(proof / "consumer"), "--config", "Release", "--parallel", "2"],
            check=True, timeout=300)
        executable = proof / "consumer/spatial-web-consumer"
        if platform.startswith("windows"):
            executable = proof / "consumer/Release/spatial-web-consumer.exe"
        environment = os.environ.copy()
        environment["QT_QPA_PLATFORM"] = "offscreen"
        environment["QT_QUICK_BACKEND"] = "software"
        environment["QTWEBENGINE_CHROMIUM_FLAGS"] = "--disable-gpu"
        environment["PATH"] = str(relocated / "bin") + os.pathsep + environment.get("PATH", "")
        subprocess.run([str(executable)], env=environment, check=True, timeout=90)
    metadata = {"schema": 1, "product": product, "version": "0.1.0", "platform": platform,
        "qt_version": "6.10.3", "source_repository": "Dockpipe-Industries/Spatial-Web",
        "source_sha": source_sha, "source_state": "committed-snapshot", "runtime_bundled": False, "compiler": compiler_identity(build_directory), "abi": {"cxx_standard": 17},
        "msvc_runtime": "dynamic-release-/MD" if platform == "windows-amd64" else None,
        "archive": {"name": archive.name, "size_bytes": archive.stat().st_size, "sha256": digest(archive)},
        "file_manifest": {"name": inventory.name, "sha256": digest(inventory)},
        "checks": ["sdk-component-install", "relocated-external-consumer-compile-link", "focused-browser-tests", "relocated-external-consumer-compile-link-run"]}
    if variant == "projection":
        from projection_dependencies import read_projection_lock
        metadata["dependencies"], metadata["dependency_lock"] = read_projection_lock()
    (output / ("sdk-release.json" if variant == "projection" else stem + ".release.json")).write_text(json.dumps(metadata, indent=2) + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--variant", required=True)
    parser.add_argument("--platform", required=True)
    parser.add_argument("--source-sha", required=True)
    parser.add_argument("--build-directory", type=Path, required=True)
    arguments = parser.parse_args()
    pack(arguments.sdk, arguments.output, arguments.variant, arguments.platform, arguments.source_sha, arguments.build_directory)
