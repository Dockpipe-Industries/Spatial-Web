#!/usr/bin/env python3
"""Build and qualify Browser in an exact, dependency-owned Flatpak stack."""
import argparse
import configparser
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

SOURCE = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(SOURCE / "release"))
from flatpak_profile import CHECKS, PLATFORM, read_stack, read_proof_driver

APP_ID = "com.spatialweb.FlatpakBrowserProof"


def run(arguments, *, environment=None, timeout=300, capture=False):
    print("+ " + " ".join(map(str, arguments)), flush=True)
    try:
        result = subprocess.run(list(map(str, arguments)), env=environment, check=True,
                                text=True, stdout=subprocess.PIPE if capture else None,
                                timeout=timeout)
    except subprocess.CalledProcessError as failure:
        if capture and failure.stdout:
            print(failure.stdout[-(4 << 20):], flush=True)
        raise
    if capture and len(result.stdout) > 4 << 20:
        raise ValueError("Flatpak command metadata exceeds capacity")
    return result.stdout.strip() if capture else ""


def verify_stack(stack):
    for entry in [*stack["flatpak"].values(), read_proof_driver()]:
        actual = run(["flatpak", "info", "--show-commit", entry["ref"]], capture=True)
        if actual != entry["commit"]:
            raise ValueError("Installed Flatpak commit differs: " + entry["ref"])


def build(output, source_sha, local_proof):
    stack = read_stack()
    verify_stack(stack)
    if output.exists() or output.is_symlink():
        raise ValueError("Flatpak SDK proof directory must be fresh")
    output.mkdir(parents=True)
    app = output / "app"
    run(["flatpak", "build-init", "--arch=x86_64", "--base=io.qt.qtwebengine.BaseApp",
         "--base-version=6.10", app, APP_ID, "org.kde.Sdk", "org.kde.Platform", "6.10"])
    metadata = configparser.ConfigParser()
    metadata.read(app / "metadata")
    if (metadata["Application"]["runtime"] != "org.kde.Platform/x86_64/6.10"
            or metadata["Application"]["sdk"] != "org.kde.Sdk/x86_64/6.10"):
        raise ValueError("Flatpak build initialized a different stack")
    options = ["flatpak", "build", "--share=network", f"--filesystem={SOURCE}:ro",
               f"--filesystem={output}", "--env=CMAKE_PREFIX_PATH=/app",
               "--env=QTWEBENGINEPROCESS_PATH=/app/bin/QtWebEngineProcess",
               "--env=QTWEBENGINE_RESOURCES_PATH=/app/resources",
               "--env=QTWEBENGINE_LOCALES_PATH=/app/translations/qtwebengine_locales"]

    def sdk_run(*arguments, test=False, timeout=300):
        command = list(options)
        if test:
            # Build containers lack the installed app's Chromium sandbox boundary.
            # This flag is confined to build tests; installed runtime proof rejects it.
            command.extend(["--env=QT_QPA_PLATFORM=offscreen", "--env=QT_QUICK_BACKEND=software",
                            "--env=QTWEBENGINE_DISABLE_SANDBOX=1",
                            "--env=QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu"])
        command.extend([str(app), *map(str, arguments)])
        return run(command, timeout=timeout)

    native = output / "native"
    sdk_run("cmake", "-S", SOURCE, "-B", native, "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DSPATIAL_WEB_ABI_PROFILE=flatpak-kde-6.10", "-DSPATIAL_WEB_BUILD_COMPOSITION=OFF",
            "-DSPATIAL_WEB_BUILD_PROJECTION=OFF", "-DBUILD_TESTING=ON")
    sdk_run("cmake", "--build", native, "--parallel", "2", timeout=600)
    sdk_run("ctest", "--test-dir", native, "--output-on-failure", test=True, timeout=600)
    sdk = output / "sdk"
    sdk_run("cmake", "--install", native, "--prefix", sdk)
    relocated = output / "relocated-sdk"
    shutil.copytree(sdk, relocated, symlinks=False)
    consumer = output / "consumer"
    sdk_run("cmake", "-S", SOURCE / "tests/consumer", "-B", consumer,
            f"-DCMAKE_PREFIX_PATH={relocated};/app", "-DSPATIAL_WEB_ABI_PROFILE=flatpak-kde-6.10",
            "-DSPATIAL_WEB_TEST_COMPOSITION=OFF", "-DSPATIAL_WEB_TEST_PROJECTION=OFF")
    sdk_run("cmake", "--build", consumer, "--parallel", "2")
    sdk_run(consumer / "spatial-web-consumer", test=True, timeout=90)
    qualify_runtime(output, source_sha, local_proof)


def qualify_runtime(output, source_sha, local_proof):
    stack = read_stack()
    app = output / "app"
    sdk = output / "sdk"
    native = output / "native"
    installed = app / "files/spatialweb-sdk"
    shutil.copytree(sdk, installed, symlinks=True)
    (app / "files/bin").mkdir(exist_ok=True)
    shutil.copy2(native / "FlatpakBrowserRuntimeTest", app / "files/bin/spatial-web-runtime-proof")
    run(["patchelf", "--set-rpath", "/app/spatialweb-sdk/lib:/app/lib",
         app / "files/bin/spatial-web-runtime-proof"])
    run(["flatpak", "build-finish", "--share=network", "--socket=x11", "--device=dri",
         "--command=spatial-web-runtime-proof", app])
    repository = output / "repository"
    run(["flatpak", "build-export", repository, app, "proof"], timeout=600)
    install_and_run(output, source_sha, local_proof)


def install_and_run(output, source_sha, local_proof):
    stack = read_stack()
    sdk = output / "sdk"
    native = output / "native"
    repository = output / "repository"
    # Copy the exact retained runtime commit into our own installation repository.
    # The user's or runner's existing Flatpak refs are read-only throughout.
    environment = proof_environment(output)
    for index, entry in enumerate([stack["flatpak"]["runtime"], read_proof_driver()]):
        location = Path(run(["flatpak", "info", "--show-location", entry["ref"]], capture=True))
        original_repository = location.parents[4] / "repo"
        if not original_repository.is_dir():
            raise ValueError("Cannot identify the retained Flatpak installation repository")
        runtime_repository = output / ("runtime-repository-" + str(index))
        run(["ostree", "init", "--repo=" + str(runtime_repository), "--mode=bare-user-only"])
        run(["ostree", "pull-local", "--repo=" + str(runtime_repository), original_repository,
             entry["commit"]], timeout=300)
        run(["ostree", "refs", "--repo=" + str(runtime_repository),
             "--create=" + entry["ref"], entry["commit"]])
        run(["ostree", "summary", "--repo=" + str(runtime_repository), "--update"])
        remote = "proof-runtime-" + str(index)
        run(["flatpak", "remote-add", "--user", "--no-gpg-verify", "--if-not-exists",
             remote, str(runtime_repository)], environment=environment)
        run(["flatpak", "install", "--user", "--noninteractive", "--no-deps", "--no-related",
             remote, entry["ref"]], environment=environment, timeout=180)
        actual = run(["flatpak", "info", "--user", "--show-commit", entry["ref"]],
                     environment=environment, capture=True)
        if actual != entry["commit"]:
            raise ValueError("Isolated proof runtime or driver differs from its qualification pin")
    run(["flatpak", "remote-add", "--user", "--no-gpg-verify", "--if-not-exists",
         "proof-app", str(repository)], environment=environment)
    run(["flatpak", "install", "--user", "--noninteractive", "--no-deps", "proof-app",
         "app/" + APP_ID + "/x86_64/proof"], environment=environment, timeout=180)
    run_runtime(output, source_sha, local_proof)


def proof_environment(output):
    environment = os.environ.copy()
    environment["FLATPAK_USER_DIR"] = str(output / "installation")
    environment["FLATPAK_GL_DRIVERS"] = "default"
    for variable, directory in (("HOME", "home"), ("XDG_CONFIG_HOME", "config"),
                                ("XDG_DATA_HOME", "data"), ("XDG_STATE_HOME", "state"),
                                ("XDG_CACHE_HOME", "cache")):
        path = output / directory
        path.mkdir(exist_ok=True)
        environment[variable] = str(path)
    return environment


def run_runtime(output, source_sha, local_proof):
    stack = read_stack()
    sdk = output / "sdk"
    native = output / "native"
    environment = proof_environment(output)
    runtime_command = ["dbus-run-session", "--", "flatpak", "run", "--user", "--branch=proof",
                       "--env=QT_QPA_PLATFORM=xcb", "--env=LIBGL_ALWAYS_SOFTWARE=1",
                       "--env=QTWEBENGINEPROCESS_PATH=/app/bin/QtWebEngineProcess",
                       "--env=QTWEBENGINE_RESOURCES_PATH=/app/resources",
                       "--env=QTWEBENGINE_LOCALES_PATH=/app/translations/qtwebengine_locales",
                       "--unset-env=QTWEBENGINE_DISABLE_SANDBOX", "--unset-env=QTWEBENGINE_CHROMIUM_FLAGS", APP_ID]
    result = run(["xvfb-run", "-a", *runtime_command], environment=environment, timeout=90, capture=True)
    print(result, flush=True)
    lines = [line for line in result.splitlines() if line.startswith('{"')]
    if len(lines) != 1:
        raise ValueError("Installed Flatpak DOM proof did not emit one receipt")
    runtime = json.loads(lines[0])
    if (runtime.get("check") != "flatpak-installed-runtime-dom" or runtime.get("dom") != "real-dom"
            or runtime.get("sandbox") != "default" or runtime.get("Qt6Core") != "6.10.3"
            or runtime.get("Qt6WebEngineCore") != "6.10.2"
            or runtime.get("runtime_commit") != stack["flatpak"]["runtime"]["commit"]
            or "llvmpipe" not in runtime.get("renderer", "").lower()):
        raise ValueError("Installed Flatpak DOM or runtime identity differs from the exact stack")
    (output / "installed-runtime-proof.json").write_text(json.dumps(runtime, indent=2) + "\n")
    receipt = {"schema": 1, "platform": PLATFORM, "source_sha": source_sha,
               "source_state": "local-proof" if local_proof else "committed-snapshot",
               "qt_components": stack["qt_components"], "flatpak": stack["flatpak"],
               "compiler": stack["compiler"], "checks": sorted(CHECKS),
               "qualification_driver": read_proof_driver(),
               "runtime_proof_sha256": hashlib.sha256((output / "installed-runtime-proof.json").read_bytes()).hexdigest()}
    (output / "qualification.json").write_text(json.dumps(receipt, indent=2) + "\n")
    run([sys.executable, SOURCE / "release/pack-flatpak-browser-sdk.py", "--sdk", sdk,
         "--output", output / "candidate", "--build-directory", native,
         "--qualification", output / "qualification.json"])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--source-sha", default=os.environ.get("GITHUB_SHA", ""))
    parser.add_argument("--local-proof", action="store_true")
    args = parser.parse_args()
    build(args.output.resolve(), args.source_sha, args.local_proof)
