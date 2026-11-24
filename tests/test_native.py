"""Regression tests for the C++ Nova interpreter.
Python runs the tests only: Nova Native itself does not use Python.
"""
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / 'bin' / ('nova-native.exe' if os.name == 'nt' else 'nova-native')


class NativeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not EXE.exists():
            if os.name == 'nt':
                subprocess.run(['powershell', '-NoProfile', '-File', str(ROOT / 'build_windows.ps1')], check=True)
            else:
                subprocess.run(['sh', str(ROOT / 'build.sh')], check=True)

    def run_nova(self, source, *args, input_text=None, mode='run'):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / 'test.nova'
            p.write_text(source, encoding='utf8')
            return subprocess.run([str(EXE), mode, str(p), *args], input=input_text,
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                  encoding='utf8', timeout=6)

    def ok(self, source, output, *args, input_text=None):
        p = self.run_nova(source, *args, input_text=input_text)
        self.assertEqual(p.returncode, 0, p.stderr)
        self.assertEqual(p.stdout, output)

    def test_mixed_keywords(self):
        self.ok('niech x = 4\nякщо x > 2\n  pokaż x\ninaczej\n  show 0\nкінець\n', '4\n')

    def test_unicode_names(self):
        self.ok('нехай імʼя = "Оля"\nпокажи імʼя\n', 'Оля\n')

    def test_arithmetic_precedence(self):
        self.ok('show 2 + 3 * 4\nshow 2 ** 3 ** 2\nshow -2 ** 2\n', '14\n512\n-4\n')

    def test_floordiv_and_mod(self):
        self.ok('show -7 // 3\nshow -7 % 3\n', '-3\n2\n')

    def test_string_escaping(self):
        self.ok('show "hi\\nthere"\nshow "not # a comment" # real comment\n', 'hi\nthere\nnot # a comment\n')

    def test_boolean_aliases(self):
        self.ok('show істина and prawda\nshow хиба or fałsz\nshow не хиба\n', 'True\nFalse\nTrue\n')

    def test_short_circuit(self):
        self.ok('show false and (1/0)\nshow true or (1/0)\n', 'False\nTrue\n')

    def test_repeat(self):
        self.ok('let n = 0\nповтори 4 разів\n n = n + 2\nкінець\nshow n\n', '8\n')

    def test_for_range(self):
        self.ok('let s = 0\ndla i w range(1,5)\n s = s+i\nkoniec\nshow s\n', '10\n')

    def test_for_break_continue(self):
        self.ok('let s = 0\nдля i в range(1,9)\n if i == 3\n  continue\n end\n if i == 6\n  break\n end\n s = s+i\nend\nshow s\n', '12\n')

    def test_unicode_for(self):
        self.ok('для знак в "Їж"\n покажи знак\nкінець\nshow len("Їж")\nshow "Їж"[0]\n', 'Ї\nж\n2\nЇ\n')

    def test_function_return(self):
        self.ok('funkcja square(x)\n zwróć x * x\nkoniec\nshow square(7)\n', '49\n')

    def test_recursion(self):
        self.ok('function fib(n)\n if n < 2\n  return n\n end\n return fib(n-1) + fib(n-2)\nend\nshow fib(7)\n', '13\n')

    def test_list_and_index_assignment(self):
        self.ok('let a=[1,2,3]\na[1]=20\nshow a\nshow len(a)\nshow a[-1]\n', '[1, 20, 3]\n3\n3\n')

    def test_objects(self):
        self.ok('let a={"name":"Ada", "age":18}\na.age=19\na["name"]="Ola"\nshow a.name\nshow a.age\n', 'Ola\n19\n')

    def test_match(self):
        self.ok('вибір 2\n коли 1\n  show "no"\n przypadek 2\n  pokaż "yes"\n else\n  show "else"\nкінець\n', 'yes\n')

    def test_match_else(self):
        self.ok('match "x"\n case "y"\n  show 0\n else\n  show 1\nend\n', '1\n')

    def test_try_catch_raise(self):
        p=self.run_nova('try\n raise "boom"\ncatch err\n show err.message\nend\n')
        self.assertEqual(p.returncode,0,p.stderr)
        self.assertIn('boom',p.stdout)

    def test_try_catch_assert(self):
        p=self.run_nova('спробуй\n перевір 2 == 3\nзлови err\n pokaż err.message\nкінець\n', '--lang','pl')
        self.assertEqual(p.returncode,0,p.stderr)
        self.assertIn('Błąd wykonania',p.stdout)

    def test_divide_by_zero(self):
        p=self.run_nova('let broken = 3 / 0')
        self.assertNotEqual(p.returncode,0)
        self.assertIn('division by zero',p.stderr)

    def test_missing_end(self):
        p=self.run_nova('if true\nshow 1\n',mode='check')
        self.assertNotEqual(p.returncode,0)
        self.assertIn('missing end',p.stderr)

    def test_valid_check(self):
        p=self.run_nova('if true\n show 2\nend\n',mode='check')
        self.assertEqual(p.returncode,0,p.stderr)
        self.assertIn('OK:',p.stdout)

    def test_known_imports_and_unknown_import(self):
        self.ok('import storage\nimport system\nshow storage.exists("____nova_missing____")\nshow system.platform()\n',
                'False\n' + ('windows' if os.name == 'nt' else 'linux') + '\n')
        p=self.run_nova('import something_unknown\n')
        self.assertNotEqual(p.returncode,0)
        self.assertIn('Cannot open file',p.stderr)

    def test_unknown_name(self):
        p=self.run_nova('show missing_name\n')
        self.assertNotEqual(p.returncode,0)
        self.assertIn('Unknown name',p.stderr)

    def test_assignment_undefined_name(self):
        p=self.run_nova('missing = 2\n')
        self.assertNotEqual(p.returncode,0)

    def test_limits(self):
        p=self.run_nova('repeat 50\n show 1\nend\n','--max-steps','4')
        self.assertNotEqual(p.returncode,0)
        self.assertIn('step limit',p.stderr)

    def test_ask(self):
        self.ok('let name=ask("Name: ")\nshow name\n','Name: Sam\n',input_text='Sam\n')

    def test_empty_input(self):
        p=self.run_nova('show\n')
        self.assertNotEqual(p.returncode,0)

    def test_incomplete_expr_no_crash(self):
        for code in ['show (\n','show [1,\n','show f(\n','show {\n']:
            p=self.run_nova(code)
            self.assertNotEqual(p.returncode,0,p.stdout)
            self.assertNotEqual(p.returncode,-11,'segmentation fault')

    @unittest.skipIf(os.name == 'nt', 'Windows EXE checked in smoke_windows.ps1')
    def test_standalone_build_runs_without_source(self):
        with tempfile.TemporaryDirectory() as d:
            source=Path(d)/'app.nova'
            output=Path(d)/'app'
            source.write_text('покажи "Незалежна Nova"\nshow 6 * 7\n', encoding='utf8')
            p=subprocess.run(['sh',str(ROOT/'build_app.sh'),str(source),str(output)],
                             capture_output=True,text=True,timeout=45)
            self.assertEqual(p.returncode,0,p.stderr)
            source.unlink()
            run=subprocess.run([str(output)],cwd=d,
                               env={'PATH':'/usr/bin:/bin','LANG':'C.UTF-8'},
                               capture_output=True,text=True,timeout=6)
            self.assertEqual(run.returncode,0,run.stderr)
            self.assertEqual(run.stdout,'Незалежна Nova\n42\n')

    @unittest.skipIf(os.name == 'nt', 'Windows EXE checked in smoke_windows.ps1')
    def test_standalone_program_limits(self):
        with tempfile.TemporaryDirectory() as d:
            source=Path(d)/'loop.nova';output=Path(d)/'loop'
            source.write_text('repeat 500\n show 1\nend\n',encoding='utf8')
            p=subprocess.run(['sh',str(ROOT/'build_app.sh'),str(source),str(output)],
                             capture_output=True,text=True,timeout=45)
            self.assertEqual(p.returncode,0,p.stderr)
            run=subprocess.run([str(output),'--max-steps','2'],capture_output=True,text=True,timeout=6)
            self.assertNotEqual(run.returncode,0)
            self.assertIn('step limit',run.stderr)

    def test_unexpected_else(self):
        p=self.run_nova('else\n show 1\nend\n', mode='check')
        self.assertNotEqual(p.returncode,0)

    def test_no_type_support_yet(self):
        p=self.run_nova('type Person\n name="Nova"\nend\n')
        self.assertNotEqual(p.returncode,0)

    def test_older_multilingual_example_parity(self):
        old=ROOT.parent/'Nova_1_2_Beta_1'/'multilingual_demo.nova'
        if not old.exists():self.skipTest('older version unavailable')
        p=self.run_nova(old.read_text(encoding='utf8'),'--lang','uk')
        q=subprocess.run(['python',str(old.parent/'nova12.py'),'run',str(old)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,encoding='utf8',timeout=6)
        self.assertEqual((p.returncode,p.stdout),(q.returncode,q.stdout),p.stderr+' '+q.stderr)

    def test_older_match_example_parity_without_import(self):
        old=ROOT.parent/'Nova_1_2_Beta_1'/'match_demo.nova'
        if not old.exists():self.skipTest('older version unavailable')
        source=old.read_text(encoding='utf8').replace('import text\n','')
        p=self.run_nova(source,'--lang','uk')
        self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(p.stdout,'18\nСума дорівнює 18\n')


if __name__=='__main__':unittest.main()
