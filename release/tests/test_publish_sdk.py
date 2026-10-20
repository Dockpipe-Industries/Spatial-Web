import importlib.util
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


def load_publisher(name):
    path = Path(__file__).resolve().parents[1] / ("publish-" + name + "-sdk.py")
    specification = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


class CandidatePublisherTests(unittest.TestCase):
    source_sha = "a" * 40

    def fixture(self, module, directory):
        for platform, stem in module.STEMS.items():
            archive = directory / (stem + ".tar.gz")
            archive.write_bytes(b"bounded-qualified-fixture")
            inventory = directory / ("sdk-files.json" if len(module.PLATFORMS) == 1 else stem + ".files.json")
            inventory.write_text('{"schema":1,"files":[]}')
            metadata = {
                "schema": 1,
                "product": "SpatialWebProjectionSDK" if len(module.PLATFORMS) == 1 else "SpatialWebBrowserSDK",
                "version": "0.1.0",
                "platform": platform,
                "qt_version": "6.10.3",
                "source_repository": module.REPOSITORY,
                "source_sha": self.source_sha,
                "source_state": "committed-snapshot",
                "runtime_bundled": False,
                "compiler": {"id": "GNU", "version": "11.4.0"},
                "abi": {"cxx_standard": 17},
                "msvc_runtime": "dynamic-release-/MD" if platform == "windows-amd64" else None,
                "archive": {"name": archive.name, "size_bytes": archive.stat().st_size, "sha256": module.digest(archive)},
                "file_manifest": {"name": inventory.name, "sha256": module.digest(inventory)},
                "checks": ["sdk-component-install", "focused-browser-tests", "relocated-external-consumer-compile-link-run"],
            }
            (directory / ("sdk-release.json" if len(module.PLATFORMS) == 1 else stem + ".release.json")).write_text(json.dumps(metadata))

    def test_complete_binary_matrix_and_projection_acceptance(self):
        for variant in ["browser", "projection"]:
            module = load_publisher(variant)
            with tempfile.TemporaryDirectory() as temporary:
                directory = Path(temporary)
                self.fixture(module, directory)
                assets = module.validate(directory, self.source_sha)
                self.assertEqual(len(assets), len(module.PLATFORMS) * 3)
                with self.assertRaises(ValueError):
                    module.validate(directory, "b" * 40)
                archive = next(directory.glob("*.tar.gz"))
                archive.write_bytes(b"tampered")
                with self.assertRaises(ValueError):
                    module.validate(directory, self.source_sha)

    def test_missing_platform_and_static_windows_runtime_rejected(self):
        module = load_publisher("browser")
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            self.fixture(module, directory)
            receipt = directory / (module.STEMS["windows-amd64"] + ".release.json")
            metadata = json.loads(receipt.read_text())
            metadata["msvc_runtime"] = "static-/MT"
            receipt.write_text(json.dumps(metadata))
            with self.assertRaises(ValueError):
                module.validate(directory, self.source_sha)
            receipt.unlink()
            with self.assertRaises(ValueError):
                module.validate(directory, self.source_sha)

    def test_existing_assets_and_tag_never_replaced(self):
        module = load_publisher("browser")
        assets = {"sdk.tar.gz": {"size": 3, "digest": "sha256:" + "a" * 64}}
        release = {"tag_name": "candidate", "target_commitish": self.source_sha,
            "prerelease": True, "assets": [{"name": "sdk.tar.gz", "size": 4, "digest": "sha256:" + "b" * 64}]}
        with self.assertRaises(ValueError):
            module.verify_release(release, "candidate", self.source_sha, assets)
        with self.assertRaises(ValueError):
            module.verify_tag({"target": {"__typename": "Commit", "oid": "b" * 40}}, self.source_sha)

    def test_wrong_dispatch_has_no_network_or_mutation(self):
        module = load_publisher("browser")
        with patch.dict(os.environ, {"GITHUB_EVENT_NAME": "pull_request"}, clear=True), patch.object(module, "run") as run:
            with self.assertRaises(ValueError):
                module.publish(Path("unused"))
            run.assert_not_called()


if __name__ == "__main__":
    unittest.main()
