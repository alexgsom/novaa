"""End-to-end portable workflow tests (no Python in the runtime or compiler)."""
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CLI = ROOT / 'nova.sh'
SOURCE = ROOT / 'examples/bootstrap/trilingual.nova'
RUNNER = ROOT / 'bin/nova-native'


@unittest.skipUnless(os.name != 'nt' and CLI.is_file() and RUNNER.is_file(), 'Linux shell CLI only')
class PortableCliTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory(prefix='nova-cli-Ярослав-')
        self.addCleanup(temp.cleanup)
        self.folder = Path(temp.name)

    def run_cli(self, *args):
        return subprocess.run(['sh', str(CLI), *[str(x) for x in args]], cwd=ROOT,
                              capture_output=True, text=True, encoding='utf-8', timeout=30)

    def test_doctor_compiles_executes_and_checks_integrity(self):
        result = self.run_cli('doctor')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('PASS:', result.stdout)

    def test_one_command_build_with_unicode_output(self):
        app = self.folder / 'Моя-Програма-Żubr'
        result = self.run_cli('build', SOURCE, app)
        self.assertEqual(result.returncode, 0, result.stderr)
        compiled = subprocess.run([str(app)], capture_output=True, text=True, encoding='utf-8')
        interpreted = subprocess.run([str(RUNNER), 'run', str(SOURCE)], capture_output=True, text=True, encoding='utf-8')
        self.assertEqual(compiled.returncode, 0)
        self.assertEqual(compiled.stdout, interpreted.stdout)

    def test_build_refuses_overwrite_unless_force(self):
        app = self.folder / 'hello'
        self.assertEqual(self.run_cli('build', SOURCE, app).returncode, 0)
        original = app.read_bytes()
        self.assertNotEqual(self.run_cli('build', SOURCE, app).returncode, 0)
        self.assertEqual(app.read_bytes(), original)
        self.assertEqual(self.run_cli('build', SOURCE, app, '--force').returncode, 0)
        self.assertEqual(app.read_bytes(), original)

    def test_compile_refuses_overwrite_unless_force(self):
        out = self.folder / 'program.nbc'
        self.assertEqual(self.run_cli('compile', SOURCE, out).returncode, 0)
        before = out.read_bytes()
        self.assertNotEqual(self.run_cli('compile', SOURCE, out).returncode, 0)
        self.assertEqual(out.read_bytes(), before)
        self.assertEqual(self.run_cli('compile', SOURCE, out, '--force').returncode, 0)
        self.assertEqual(out.read_bytes(), before)

    def test_reject_missing_source(self):
        app = self.folder / 'no-app'
        r = self.run_cli('build', self.folder / 'missing.nova', app)
        self.assertNotEqual(r.returncode, 0)
        self.assertFalse(app.exists())

    def test_reject_wrong_extension(self):
        out = self.folder / 'wrong.txt'
        r = self.run_cli('compile', SOURCE, out)
        self.assertNotEqual(r.returncode, 0)
        self.assertFalse(out.exists())

    def test_reject_unknown_option(self):
        out = self.folder / 'app'
        r = self.run_cli('build', SOURCE, out, '--surprise')
        self.assertNotEqual(r.returncode, 0)
        self.assertFalse(out.exists())

    def test_inspect_and_unpack(self):
        app = self.folder / 'hello'
        code = self.folder / 'restored.nbc'
        self.assertEqual(self.run_cli('build', SOURCE, app).returncode, 0)
        r = self.run_cli('inspect', app)
        self.assertEqual(r.returncode, 0)
        self.assertIn('Integrity: OK', r.stdout)
        self.assertEqual(self.run_cli('unpack', app, code).returncode, 0)
        compiled = self.folder / 'from-source.nbc'
        self.assertEqual(self.run_cli('compile', SOURCE, compiled).returncode, 0)
        self.assertEqual(code.read_bytes(), compiled.read_bytes())


if __name__ == '__main__':
    unittest.main()
