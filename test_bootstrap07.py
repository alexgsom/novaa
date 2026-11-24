"""Self-hosted Nova bytecode compiler: stage-0 -> stage-1 -> stage-2 identity.

The Python standard library is used exclusively as a test harness, not as a
Nova runtime or a compiler implementation. No network is required.
"""
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / 'bin' / ('nova-native.exe' if os.name == 'nt' else 'nova-native')
COMPILER = ROOT / 'compiler' / 'novac.nova'


def execute(*argv, cwd=None, timeout=40):
    return subprocess.run([str(x) for x in argv], cwd=cwd, capture_output=True,
                          encoding='utf-8', errors='replace', text=True, timeout=timeout)


class Bootstrap07Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not ENGINE.exists():
            raise unittest.SkipTest('Build native engine first')
        cls.tmp = tempfile.TemporaryDirectory(prefix='Nova-self-host-Łódź-')
        cls.directory = Path(cls.tmp.name)
        cls.one = cls.directory / 'stage1.nbc'
        cls.two = cls.directory / 'stage2.nbc'
        first = execute(ENGINE, 'run', COMPILER, '--max-steps', 15000000,
                        '--', COMPILER, cls.one, timeout=75)
        if first.returncode:
            raise AssertionError('Stage-0 failed to compile novac.nova: '+first.stderr)
        second = execute(ENGINE, 'exec', cls.one, '--max-steps', 15000000,
                         '--', COMPILER, cls.two, timeout=75)
        if second.returncode:
            raise AssertionError('Stage-1 compiler cannot compile itself: '+second.stderr)

    @classmethod
    def tearDownClass(cls):
        if hasattr(cls, 'tmp'):
            cls.tmp.cleanup()

    def compile_run(self, src, expect=None):
        source = self.directory / 'Привіт-Żółw.nova'
        output = self.directory / 'test-product.nbc'
        source.write_text(src, encoding='utf8')
        compiled = execute(ENGINE, 'exec', self.two, '--max-steps', 3000000,
                           '--', source, output, timeout=40)
        self.assertEqual(compiled.returncode, 0, compiled.stderr)
        binary = execute(ENGINE, 'exec', output, timeout=25)
        direct = execute(ENGINE, 'run', source, timeout=25)
        self.assertEqual(binary.returncode, 0, binary.stderr)
        self.assertEqual(direct.returncode, 0, direct.stderr)
        self.assertEqual(binary.stdout, direct.stdout)
        if expect is not None:
            self.assertEqual(binary.stdout, expect)
        return output

    def test_01_self_compilation_byte_identical(self):
        self.assertEqual(self.one.read_bytes(), self.two.read_bytes())
        obj = json.loads(self.one.read_text(encoding='utf8'))
        self.assertEqual(obj['format'], 'nova-bytecode')
        self.assertEqual(obj['version'], 1)
        self.assertGreater(len(obj['instructions']), 100)

    def test_02_self_compiler_can_compile_itself_again(self):
        third = self.directory / 'stage3.nbc'
        result = execute(ENGINE, 'exec', self.two, '--max-steps', 15000000,
                         '--', COMPILER, third, timeout=75)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(third.read_bytes(), self.two.read_bytes())

    def test_03_unicode_objects_fields_and_index_assignment(self):
        self.compile_run('''let values = [{"name": "Львів", "counts": [2, 3]}]
show values[0].name
values[0]["counts"][1] = 9
show values[0]["counts"][1]
let v = {"a": 1}
v.a = 7
show v.a
''', 'Львів\n9\n7\n')

    def test_04_import_stdlib_text_lists_json(self):
        self.compile_run('''import text
import lists
import json
let parts = text.split(" Жółw,Львів ", ",")
let items = []
lists.append(items, text.trim(parts[0]))
lists.append(items, text.trim(parts[1]))
show json.stringify({"places": items, "count": len(items)})
''', '{"count":2,"places":["Жółw","Львів"]}\n')

    def test_05_nested_functions_and_boolean(self):
        self.compile_run('''funkcja fact(n)
    jeśli n <= 1
        zwróć 1
    koniec
    return n * fact(n - 1)
koniec
show false and (1 / 0)
show true or (1 / 0)
show fact(6)
''', 'False\nTrue\n720\n')

    def test_06_standard_module_file_io(self):
        p = self.directory / 'Нотатки-Żółw.txt'
        src = ("import storage\nlet file = " + json.dumps(str(p), ensure_ascii=False) +
               '\nstorage.write(file, "Привіт!")\nshow storage.read(file)\nshow storage.exists(file)\n')
        self.compile_run(src, 'Привіт!\nTrue\n')

    def test_07_empty_objects_and_nested_literals(self):
        self.compile_run('''let x = {}
let y = {"x": [], "d": {"v": [1, 2]}}
x["data"] = y
show x["data"]["d"]["v"][1]
''', '2\n')

    def test_08_import_requirement_enforced(self):
        broken = self.directory / 'noimport.nbc'
        broken.write_text(json.dumps({'format': 'nova-bytecode', 'version': 1,
            'instructions': [['push', 'hello'], ['call', 'text.trim', 1], ['print'], ['halt']]}), encoding='utf8')
        result = execute(ENGINE, 'exec', broken)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('module not imported', result.stderr)

    def test_09_unknown_module_and_bad_object_opcode_rejected(self):
        product = self.directory / 'reject.nbc'
        for code, match in [
            ([['import', 'nonexistent'], ['halt']], 'unsupported bytecode import'),
            ([['make_object', 1], ['halt']], 'stack underflow'),
            ([['push', 3], ['get_attr', 'a'], ['halt']], 'property read expects'),
            ([['push', 4], ['push', 1], ['push', 2], ['set_index'], ['halt']], 'index assignment expects'),
        ]:
            with self.subTest(code=code):
                product.write_text(json.dumps({'format':'nova-bytecode', 'version':1,'instructions':code}), encoding='utf8')
                result = execute(ENGINE, 'exec', product)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(match, result.stderr)

    def test_10_utf8_keywords_and_lists(self):
        self.compile_run('''niech miasta = [{"miasto": "Łódź"}, {"miasto": "Київ"}]
dla item w miasta
    pokaż item.miasto
koniec
''', 'Łódź\nКиїв\n')

    def test_11_generated_compiler_compiles_file_with_invalid_syntax(self):
        file = self.directory / 'broken.nova'
        output = self.directory / 'broken.nbc'
        file.write_text('let data = {"x": 1\n', encoding='utf8')
        bad = execute(ENGINE, 'exec', self.two, '--max-steps', 1500000, '--', file, output)
        self.assertNotEqual(bad.returncode, 0)
        self.assertIn('Unclosed delimiter', bad.stderr)

    def test_12_vm_step_budget_limits_compiler(self):
        file = self.directory / 'simple.nova'
        output = self.directory / 'budget.nbc'
        file.write_text('show 1\n', encoding='utf8')
        limited = execute(ENGINE, 'exec', self.two, '--max-steps', 20, '--', file, output)
        self.assertNotEqual(limited.returncode, 0)
        self.assertIn('step limit exceeded', limited.stderr)


if __name__=='__main__':
    unittest.main()
