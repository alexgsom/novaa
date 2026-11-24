"""Nova portable native packaging 0.8 (Python used ONLY for test orchestration)."""
import os
import stat
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / 'bin' / ('nova-native.exe' if os.name == 'nt' else 'nova-native')
EXAMPLE = ROOT / 'examples' / 'bootstrap' / 'trilingual.nbc'
SOURCE = ROOT / 'examples' / 'bootstrap' / 'trilingual.nova'
COMPILER = ROOT / 'compiler' / 'novac_selfhost.nbc'
EXT = '.exe' if os.name == 'nt' else ''


def command(*args, timeout=25):
    return subprocess.run([str(x) for x in args], capture_output=True, text=True,
                          encoding='utf-8', errors='replace', timeout=timeout)


class PortableTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not RUNNER.exists():
            raise unittest.SkipTest('Build native runtime first')

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='Nova-Portable-Żółw-')
        self.addCleanup(self.temp.cleanup)
        self.dir = Path(self.temp.name)
        self.app = self.dir / ('Привіт-Łódź' + EXT)
        self.source = self.dir / 's.nbc'
        self.source.write_bytes(EXAMPLE.read_bytes())

    def pack(self, *extra):
        return command(RUNNER, 'pack', self.source, self.app, *extra)

    def test_01_pack_and_execute_matches_bytecode_vm(self):
        r = self.pack()
        self.assertEqual(r.returncode, 0, r.stderr)
        direct = command(RUNNER, 'exec', self.source)
        bundled = command(self.app)
        self.assertEqual(bundled.returncode, 0, bundled.stderr)
        self.assertEqual(bundled.stdout, direct.stdout)
        self.assertIn('Привіт', bundled.stdout)

    def test_02_inspect_reports_verified_integrity(self):
        self.assertEqual(self.pack().returncode, 0)
        r = command(RUNNER, 'inspect', self.app)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn('Integrity: OK', r.stdout)
        self.assertIn('Bytecode bytes:', r.stdout)

    def test_03_unpack_roundtrip_byte_identical(self):
        self.assertEqual(self.pack().returncode, 0)
        extracted = self.dir / 'extracted.nbc'
        r = command(RUNNER, 'unpack', self.app, extracted)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(extracted.read_bytes(), self.source.read_bytes())

    def test_04_reject_overwrite_without_force(self):
        self.assertEqual(self.pack().returncode, 0)
        before = self.app.read_bytes()
        second = self.pack()
        self.assertNotEqual(second.returncode, 0)
        self.assertIn('--force', second.stderr)
        self.assertEqual(before, self.app.read_bytes())

    def test_05_force_replaces_package(self):
        self.assertEqual(self.pack().returncode, 0)
        self.source.write_text('{"format":"nova-bytecode","version":1,"instructions":[["push",10],["print"],["halt"]]}', encoding='utf-8')
        self.assertEqual(self.pack('--force').returncode, 0)
        self.assertEqual(command(self.app).stdout, '10\n')

    def test_06_invalid_json_refused_without_writing_output(self):
        self.source.write_text('not-json', encoding='utf-8')
        result = self.pack()
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(self.app.exists())

    def test_07_wrong_bytecode_version_refused(self):
        self.source.write_text('{"format":"nova-bytecode","version":999,"instructions":[]}',encoding='utf-8')
        result = self.pack()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('version', result.stderr)

    def test_08_corrupt_payload_detected_on_launch_and_inspect(self):
        self.assertEqual(self.pack().returncode, 0)
        payload = bytearray(self.app.read_bytes())
        payload[-40] ^= 0x01
        self.app.write_bytes(payload)
        for arg in [[], ['inspect', self.app]]:
            proc = command(self.app, *arg) if not arg else command(RUNNER, *arg)
            self.assertNotEqual(proc.returncode, 0)
            self.assertIn('checksum mismatch', proc.stderr)

    def test_09_corrupt_length_refused(self):
        self.assertEqual(self.pack().returncode, 0)
        data = bytearray(self.app.read_bytes())
        data[-16:-8] = (50000000).to_bytes(8, 'little')
        self.app.write_bytes(data)
        result = command(RUNNER, 'inspect', self.app)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('length', result.stderr)

    def test_10_reject_wrong_platform_extension(self):
        if os.name == 'nt':
            self.skipTest('A Windows runtime naturally creates .exe')
        target = self.dir / 'NotAWindowsApp.exe'
        r = command(RUNNER, 'pack', self.source, target)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn('Windows EXE', r.stderr)
        self.assertFalse(target.exists())

    def test_11_no_packaged_runner_reuse(self):
        self.assertEqual(self.pack().returncode, 0)
        nextapp = self.dir / ('other'+EXT)
        result = command(RUNNER, 'pack', self.source, nextapp, '--runtime', self.app)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('already packaged', result.stderr)

    def test_12_refuse_overwrite_runtime_or_bytecode(self):
        x = command(RUNNER, 'pack', self.source, RUNNER)
        self.assertNotEqual(x.returncode, 0)
        y = command(RUNNER, 'pack', self.source, self.source)
        self.assertNotEqual(y.returncode, 0)
        self.assertEqual(self.source.read_bytes(), EXAMPLE.read_bytes())

    def test_13_preserve_executable_permission(self):
        if os.name == 'nt':
            self.skipTest('Windows uses PE executable permission')
        self.assertEqual(self.pack().returncode, 0)
        self.assertTrue(os.stat(self.app).st_mode & stat.S_IXUSR)

    def test_14_compilerless_pipeline_from_self_hosted_bytecode(self):
        generated = self.dir / 'compiled.nbc'
        r = command(RUNNER, 'exec', COMPILER, '--max-steps', '15000000', '--', SOURCE, generated, timeout=70)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.source.write_bytes(generated.read_bytes())
        self.assertEqual(self.pack().returncode, 0)
        self.assertEqual(command(self.app).stdout, command(RUNNER,'run',SOURCE).stdout)

    def test_15_packaged_self_hosted_compiler_compiles_nova(self):
        self.source.write_bytes(COMPILER.read_bytes())
        self.assertEqual(self.pack().returncode, 0)
        generated = self.dir / 'compiled.nbc'
        r = command(self.app,'--max-steps','15000000','--',SOURCE,generated,timeout=70)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(command(RUNNER,'exec',generated).returncode,0)

    def test_16_regular_runtime_has_no_package(self):
        r=command(RUNNER,'inspect',RUNNER)
        self.assertNotEqual(r.returncode,0)
        self.assertIn('Not a Nova portable',r.stderr)

    def test_17_unpack_refuses_existing_file(self):
        self.assertEqual(self.pack().returncode, 0)
        target = self.dir / 'out.nbc'
        target.write_text('DO NOT DELETE')
        r = command(RUNNER, 'unpack', self.app, target)
        self.assertNotEqual(r.returncode, 0)
        self.assertEqual(target.read_text(), 'DO NOT DELETE')
        r = command(RUNNER, 'unpack', self.app, target, '--force')
        self.assertEqual(r.returncode, 0)
        self.assertEqual(target.read_bytes(), self.source.read_bytes())

    def test_18_too_big_bytecode_refused(self):
        self.source.write_bytes(b' '*(4*1024*1024+1))
        r = self.pack()
        self.assertNotEqual(r.returncode, 0)
        self.assertFalse(self.app.exists())

    def test_19_unrelated_executable_rejected(self):
        bad = self.dir / ('not-nova'+EXT)
        bad.write_bytes(RUNNER.read_bytes()[:1024])
        result = command(RUNNER, 'pack', self.source, self.app, '--runtime', bad)
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(self.app.exists())

    def test_20_help_does_not_execute_packaged_program(self):
        self.assertEqual(self.pack().returncode, 0)
        r = command(self.app, '--help')
        self.assertEqual(r.returncode,0)
        self.assertIn('Nova portable application',r.stdout)
        self.assertNotIn('Привіт',r.stdout)


if __name__ == '__main__':
    unittest.main()
