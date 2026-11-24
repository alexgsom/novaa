"""Nova language update 12: trilingual while-loop direct/bytecode parity."""
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / 'bin' / ('nova-native.exe' if os.name == 'nt' else 'nova-native')
COMPILER = ROOT / 'compiler' / 'novac_selfhost.nbc'


def run(*args, timeout=30):
    return subprocess.run([str(x) for x in args], capture_output=True,
                          encoding='utf-8', errors='replace', timeout=timeout)


class While12Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not ENGINE.is_file() or not COMPILER.is_file():
            raise unittest.SkipTest('Build native engine and self-hosted compiler first')
        cls.tmp = tempfile.TemporaryDirectory(prefix='nova-while-')
        cls.dir = Path(cls.tmp.name)

    @classmethod
    def tearDownClass(cls):
        if hasattr(cls, 'tmp'):
            cls.tmp.cleanup()

    def check_parity(self, code, expected):
        source = self.dir / 'test.nova'
        bytecode = self.dir / 'test.nbc'
        source.write_text(code, encoding='utf-8')
        compiled = run(ENGINE, 'exec', COMPILER, '--max-steps', '15000000',
                       '--', source, bytecode)
        self.assertEqual(compiled.returncode, 0, compiled.stderr)
        direct = run(ENGINE, 'run', source)
        via_vm = run(ENGINE, 'exec', bytecode)
        self.assertEqual(direct.returncode, 0, direct.stderr)
        self.assertEqual(via_vm.returncode, 0, via_vm.stderr)
        self.assertEqual(direct.stdout, expected)
        self.assertEqual(via_vm.stdout, expected)

    def test_english_while(self):
        self.check_parity('''let n = 1
while n <= 5
    show n
    n = n + 1
end
''', '1\n2\n3\n4\n5\n')

    def test_ukrainian_while(self):
        self.check_parity('''нехай n = 3
поки n > 0
    покажи n
    n = n - 1
кінець
''', '3\n2\n1\n')

    def test_polish_while_break_continue(self):
        self.check_parity('''niech n = 0
dopóki n < 8
    n = n + 1
    jeśli n == 2
        kontynuuj
    koniec
    jeśli n == 5
        przerwij
    koniec
    pokaż n
koniec
''', '1\n3\n4\n')

    def test_nested_while_repeat_and_function(self):
        self.check_parity('''function count(n)
    let x = 0
    while x < n
        repeat 2
            show x
        end
        x = x + 1
    end
end
count(2)
''', '0\n0\n1\n1\n')

    def test_missing_condition_or_end_rejected(self):
        p = self.dir/'bad.nova'
        p.write_text('while\n    show 1\nend\n', encoding='utf-8')
        d = run(ENGINE, 'run', p)
        c = run(ENGINE, 'exec', COMPILER, '--max-steps', '15000000', '--', p, self.dir/'bad.nbc')
        self.assertNotEqual(d.returncode, 0)
        self.assertNotEqual(c.returncode, 0)
        p.write_text('while true\n    show 1\n', encoding='utf-8')
        d = run(ENGINE, 'run', p)
        c = run(ENGINE, 'exec', COMPILER, '--max-steps', '15000000', '--', p, self.dir/'bad.nbc')
        self.assertNotEqual(d.returncode, 0)
        self.assertNotEqual(c.returncode, 0)

    def test_infinite_loop_step_budget(self):
        p = self.dir/'limit.nova'
        p.write_text('while true\nend\n', encoding='utf-8')
        d = run(ENGINE, 'run', p, '--max-steps', '50')
        self.assertNotEqual(d.returncode, 0)
        self.assertIn('step limit', d.stderr.lower())
        via = self.dir/'limit.nbc'
        c = run(ENGINE, 'exec', COMPILER, '--max-steps', '15000000', '--', p, via)
        self.assertEqual(c.returncode, 0, c.stderr)
        b = run(ENGINE, 'exec', via, '--max-steps', '50')
        self.assertNotEqual(b.returncode, 0)
        self.assertIn('step limit', b.stderr.lower())

if __name__ == '__main__':
    unittest.main()
