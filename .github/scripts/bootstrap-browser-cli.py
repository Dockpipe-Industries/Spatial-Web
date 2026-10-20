#!/usr/bin/env python3
"""Install the exact committed native CLI pin for the Browser-only SDK producer."""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import platform
import tempfile
import urllib.request
import zipfile


def bootstrap():
    root = Path(__file__).resolve().parents[2]
    lock = json.loads((root / "release/browser-toolchains.lock.json").read_text())
    operating_system = {"Linux": "linux", "Darwin": "darwin", "Windows": "windows"}[platform.system()]
    architecture = {"x86_64": "amd64", "AMD64": "amd64", "aarch64": "arm64", "arm64": "arm64"}[platform.machine()]
    identity = operating_system + "-" + architecture
    entry = lock["platforms"][identity]
    destination = Path(tempfile.mkdtemp(prefix="spatial-web-cli-", dir=os.environ["RUNNER_TEMP"]))
    archive = destination / ("cli." + entry["format"])
    received = 0
    checksum = hashlib.sha256()
    with urllib.request.urlopen(entry["url"], timeout=120) as response, archive.open("wb") as output:
        if not response.url.startswith("https://"):
            raise ValueError("CLI redirects must preserve HTTPS")
        while chunk := response.read(1 << 20):
            received += len(chunk)
            if received > 128 << 20:
                raise ValueError("CLI archive exceeds 128 MiB")
            checksum.update(chunk)
            output.write(chunk)
    if checksum.hexdigest() != entry["sha256"]:
        raise ValueError("Native CLI digest differs from committed pin")
    executable = destination / ("dockpipe.exe" if operating_system == "windows" else "dockpipe")
    if entry["format"] == "zip":
        with zipfile.ZipFile(archive) as bundle:
            members = bundle.infolist()
            if len(members) != 1 or members[0].filename != "dockpipe.exe" or not 0 < members[0].file_size <= 256 << 20:
                raise ValueError("CLI ZIP must contain one bounded dockpipe.exe")
            with bundle.open(members[0]) as source, executable.open("xb") as output:
                while chunk := source.read(1 << 20):
                    output.write(chunk)
    else:
        specification = importlib.util.spec_from_file_location("locked_cli", root / ".github/scripts/bootstrap-dockpipe.py")
        module = importlib.util.module_from_spec(specification)
        specification.loader.exec_module(module)
        module.extract_executable(archive, executable)
    archive.unlink()
    for value in [str(destination), str(executable)]:
        if "\n" in value or "\r" in value:
            raise ValueError("CLI paths cannot contain newlines")
    with Path(os.environ["GITHUB_PATH"]).open("a") as paths:
        paths.write(str(destination) + "\n")
    with Path(os.environ["GITHUB_ENV"]).open("a") as environment:
        environment.write("DOCKPIPE_BIN=" + str(executable) + "\n")


if __name__ == "__main__":
    bootstrap()
