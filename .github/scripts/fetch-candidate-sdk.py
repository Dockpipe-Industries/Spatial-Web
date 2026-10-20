#!/usr/bin/env python3
"""Fetch a caller-pinned SDK archive into a fresh, workflow-owned directory."""
import hashlib
import pathlib
import re
import sys
import tarfile
import urllib.parse
import urllib.request

url, checksum, destination = sys.argv[1:]
if urllib.parse.urlparse(url).scheme != "https":
    raise SystemExit("SDK archives require HTTPS")
if not re.fullmatch(r"[0-9a-f]{64}", checksum):
    raise SystemExit("SDK archive SHA-256 must be explicit lowercase hex")
root = pathlib.Path(destination)
root.mkdir(parents=True, exist_ok=False)
archive = root.parent / (root.name + ".tar.gz")
maximum_archive_bytes = 4 * 1024 * 1024 * 1024
received_bytes = 0
with urllib.request.urlopen(url, timeout=120) as response, archive.open("wb") as output:
    while chunk := response.read(1024 * 1024):
        received_bytes += len(chunk)
        if received_bytes > maximum_archive_bytes:
            raise SystemExit("SDK archive exceeds the 4 GiB download budget")
        output.write(chunk)
with archive.open("rb") as source:
    digest = hashlib.sha256()
    for block in iter(lambda: source.read(1024 * 1024), b""):
        digest.update(block)
    actual = digest.hexdigest()
if actual != checksum:
    raise SystemExit("SDK archive SHA-256 mismatch")
with tarfile.open(archive) as bundle:
    members = bundle.getmembers()
    if len(members) > 100000 or sum(member.size for member in members) > 16 * 1024 * 1024 * 1024:
        raise SystemExit("SDK archive exceeds the extraction budget")
    bundle.extractall(root, members=members, filter="data")
