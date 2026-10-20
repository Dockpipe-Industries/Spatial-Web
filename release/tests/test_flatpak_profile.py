import copy
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

RELEASE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(RELEASE))
from flatpak_profile import CHECKS, read_stack


class FlatpakProfileTest(unittest.TestCase):
    def setUp(self):
        self.stack = read_stack()

    def admit(self, value):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'stack.json'
            path.write_text(json.dumps(value))
            return read_stack(path)

    def test_official_component_patch_versions_are_distinct(self):
        self.assertEqual(self.stack['qt_components']['Qt6Core'], '6.10.3')
        self.assertEqual(self.stack['qt_components']['Qt6WebEngineCore'], '6.10.2')
        self.assertIn('flatpak-installed-runtime-dom', CHECKS)

    def test_native_uniform_version_cannot_replace_profile(self):
        stack = copy.deepcopy(self.stack)
        stack['qt_components']['Qt6WebEngineCore'] = '6.10.3'
        with self.assertRaises(ValueError):
            self.admit(stack)

    def test_mutable_reference_or_missing_commit_rejected(self):
        for key in ('sdk', 'runtime', 'base'):
            stack = copy.deepcopy(self.stack)
            stack['flatpak'][key]['commit'] = 'latest'
            with self.subTest(key=key), self.assertRaises(ValueError):
                self.admit(stack)

    def test_flatpak_publisher_requires_committed_source_and_runtime_proof(self):
        specification = importlib.util.spec_from_file_location('flatpak_publisher', RELEASE / 'publish-flatpak-browser-sdk.py')
        publisher = importlib.util.module_from_spec(specification)
        specification.loader.exec_module(publisher)
        with patch.dict('os.environ', {'GITHUB_EVENT_NAME': 'pull_request'}, clear=True), \
                patch.object(publisher, 'run') as external_operation:
            with self.assertRaises(ValueError):
                publisher.publish(Path('unused'))
            external_operation.assert_not_called()
        from flatpak_profile import STEM, PRODUCT, PLATFORM
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            archive = directory / (STEM + '.tar.gz')
            archive.write_bytes(b'qualified-sdk')
            inventory = directory / (STEM + '.files.json')
            inventory.write_text('{"schema":1,"artifact_class":"sdk","files":[]}')
            metadata = {
                'schema': 1, 'product': PRODUCT, 'version': '0.1.0', 'platform': PLATFORM,
                'source_repository': publisher.REPOSITORY, 'source_sha': 'a' * 40,
                'source_state': 'committed-snapshot', 'runtime_bundled': False,
                'compiler': self.stack['compiler'], 'abi': {'cxx_standard': 17},
                'qt_components': self.stack['qt_components'], 'flatpak': self.stack['flatpak'],
                'checks': sorted(CHECKS),
                'archive': {'name': archive.name, 'size_bytes': archive.stat().st_size,
                            'sha256': publisher.digest(archive)},
                'file_manifest': {'name': inventory.name, 'sha256': publisher.digest(inventory)},
            }
            receipt = directory / (STEM + '.release.json')
            receipt.write_text(json.dumps(metadata))
            self.assertEqual(len(publisher.validate(directory, 'a' * 40)), 3)
            for change in ({'source_state': 'local-proof'}, {'checks': sorted(CHECKS - {'flatpak-installed-runtime-dom'})},
                           {'qt_version': '6.10.3'}, {'source_sha': 'b' * 40}):
                altered = dict(metadata, **change)
                receipt.write_text(json.dumps(altered))
                with self.subTest(change=change), self.assertRaises(ValueError):
                    publisher.validate(directory, 'a' * 40)
            receipt.write_text(json.dumps(metadata))
            archive.write_bytes(b'tampered-sdk')
            with self.assertRaises(ValueError):
                publisher.validate(directory, 'a' * 40)



if __name__ == '__main__':
    unittest.main()
