#!/usr/bin/env python3
"""Admit public, immutable dependency URLs only after the owner commits their lock."""
import argparse
import json
import os
from pathlib import Path


import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "release"))
from projection_dependencies import read_projection_lock


def read_lock(path):
    dependencies, _ = read_projection_lock(path)
    environment = {}
    for product, prefix in [("SpatialPipe", "SPATIALPIPE"), ("SpatialPipeScene", "SCENE"), ("ForgePipe", "FORGEPIPE")]:
        environment[prefix + "_URL"] = dependencies[product]["url"]
        environment[prefix + "_SHA256"] = dependencies[product]["sha256"]
    return environment


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    arguments = parser.parse_args()
    path = Path(__file__).resolve().parents[2] / "release/projection-dependencies.lock.json"
    if arguments.check:
        ready = path.is_file()
        if ready:
            read_lock(path)
        with Path(os.environ["GITHUB_OUTPUT"]).open("a") as output:
            output.write("ready=" + ("true" if ready else "false") + "\n")
        if not ready:
            print("::notice::Projection qualification skipped: exact public dependency lock is pending a separate approved PR.")
            with Path(os.environ["GITHUB_STEP_SUMMARY"]).open("a") as summary:
                summary.write("Projection qualification and publication are **skipped, pending the committed public dependency lock**. Browser SDK matrix remains independent.\n")
    else:
        environment = read_lock(path)
        with Path(os.environ["GITHUB_ENV"]).open("a") as output:
            for key, value in environment.items():
                output.write(key + "=" + value + "\n")
