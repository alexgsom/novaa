"""Stage-1 compiler IS A NOVA PROGRAM. Python is only a test harness."""
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NOVA = ROOT / 'bin' / ('nova-native.exe' if os.name == 'nt' else 'nova-native')
COMPILER = ROOT / 'compiler' / 'novac.nova'


class Bootstrap05Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not NOVA.exists():
            raise unittest.SkipTest('Build native engine first')

    def execute(self, code, step_limit='100000'):
        with tempfile.TemporaryDirectory(prefix='Nova-samohost-Żółw-') as root:
            directory=Path(root)
            source=directory / 'my-program.nova'
            product=directory / 'my-program.nbc'
            source.write_text(code, encoding='utf8')
            build=subprocess.run([str(NOVA), 'run', str(COMPILER), '--max-steps', '2000000', '--', str(source), str(product)],
                                 cwd=directory, capture_output=True,text=True,encoding='utf8', timeout=25)
            if build.returncode:
                return build, None, None, None
            compiled=json.loads(product.read_text(encoding='utf8'))
            vm=subprocess.run([str(NOVA), 'exec', str(product), '--max-steps',step_limit],
                              cwd=directory,capture_output=True,text=True,encoding='utf8',timeout=12)
            reference=subprocess.run([str(NOVA), 'run', str(source), '--max-steps', step_limit],
                                     cwd=directory,capture_output=True,text=True,encoding='utf8',timeout=12)
            return build, vm, reference, compiled

    def matches(self, source, expected):
        build, vm, reference, compiled=self.execute(source)
        self.assertEqual(build.returncode, 0, build.stderr)
        self.assertEqual(vm.returncode, 0, vm.stderr)
        self.assertEqual(reference.returncode, 0, reference.stderr)
        self.assertEqual(vm.stdout, expected)
        self.assertEqual(vm.stdout, reference.stdout)
        self.assertEqual(compiled['version'],1)
        self.assertEqual(compiled['format'],'nova-bytecode')
        self.assertEqual(compiled['instructions'][-1],['halt'])

    def test_multilingual_and_precedence(self):
        src=(ROOT/'examples/bootstrap/trilingual.nova').read_text(encoding='utf8')
        self.matches(src,'Привіт, Оля\n14\n✓ wynik poprawny\nNova!\nNova!\nNova!\n')

    def test_operators_and_conversions(self):
        self.matches('''let x = -(2+3) * 4
show x
show str(x) + "!"
show int("9") + 1
show not false
show 2 < 3 and 4 >= 4
show 2 > 3 or 1 == 1
show 8 % 3
''','-20\n-20!\n10\nTrue\nTrue\nTrue\n2\n')

    def test_nested_repeat_and_if_else(self):
        self.matches('''let count = 0
repeat 3
  repeat 2
    count = count + 1
    if count % 2 == 0
      show "even"
    else
      show "odd"
    end
  end
end
show count
''','odd\neven\nodd\neven\nodd\neven\n6\n')

    def test_if_without_else(self):
        self.matches('''if false
  show "not printed"
end
if true
  show "printed"
end
''','printed\n')

    def test_zero_repeat(self):
        self.matches('''let n = 0
repeat 0
  n = 900
end
show n
''','0\n')

    def test_negative_and_unicode_identifiers(self):
        self.matches(r'''niech żółw = -3
show żółw + 5
show "Привіт \"світ\""
''','2\nПривіт "світ"\n')

    def test_boolean_literals_polish_ukrainian(self):
        self.matches('''якщо prawda і не хиба
  покажи "так"
кінець
jeśli fałsz lub істина
  pokaż "tak"
koniec
''','так\ntak\n')

    def test_error_unknown_statement(self):
        build, vm, ref, obj=self.execute('type Unsupported\nend\n')
        self.assertNotEqual(build.returncode,0)
        self.assertIn('Unsupported statement',build.stderr)
        self.assertIsNone(vm)

    def test_error_unbalanced_blocks(self):
        for source in ('if true\nshow 1\n','else\n','end\n','repeat 3\nshow 3\n'):
            with self.subTest(source=source):
                build,*_=self.execute(source)
                self.assertNotEqual(build.returncode,0)

    def test_error_expression_operators(self):
        for source in ('show 2 +\n','show (1+2\n','show 2 3\n','show @\n', 'show "unfinished\n'):
            with self.subTest(source=source):
                build,*_=self.execute(source)
                self.assertNotEqual(build.returncode,0)

    def test_vm_runtime_error_div_zero(self):
        build,vm,ref,_=self.execute('show 4 / 0\n')
        self.assertEqual(build.returncode,0,build.stderr)
        self.assertNotEqual(vm.returncode,0)
        self.assertIn('division by zero',vm.stderr)
        self.assertIn('division by zero',ref.stderr)

    def test_vm_runtime_error_unknown_var(self):
        build,vm,ref,_=self.execute('show missing\n')
        self.assertEqual(build.returncode,0,build.stderr)
        self.assertNotEqual(vm.returncode,0)
        self.assertIn('undefined variable',vm.stderr)

    def test_vm_step_limit(self):
        build,vm,ref,_=self.execute('repeat 200\nshow "x"\nend\n',step_limit='100')
        self.assertEqual(build.returncode,0,build.stderr)
        self.assertNotEqual(vm.returncode,0)
        self.assertIn('step limit',vm.stderr)

    def test_vm_corrupt_bytecode_rejection(self):
        problems=[
            {'format':'nova-bytecode','version':2,'instructions':[]},
            {'format':'nova-bytecode','version':1,'instructions':[['secret','execute']]},
            {'format':'nova-bytecode','version':1,'instructions':[['jump',-2]]},
            {'format':'nova-bytecode','version':1,'instructions':[['jump',999]]},
            {'format':'nova-bytecode','version':1,'instructions':[['print']]},
            {'format':'nova-bytecode','version':1,'instructions':[['push',{}]]},
            {'format':'nova-bytecode','version':1,'instructions':[['push',1],['store',4]]},
        ]
        with tempfile.TemporaryDirectory() as d:
            file=Path(d)/'corrupt.nbc'
            for index,code in enumerate(problems):
                with self.subTest(index=index):
                    file.write_text(json.dumps(code),encoding='utf8')
                    proc=subprocess.run([str(NOVA),'exec',str(file)],capture_output=True,text=True,encoding='utf8',timeout=10)
                    self.assertNotEqual(proc.returncode,0)
                    self.assertIn('Nova Native:',proc.stderr)

    def test_vm_stack_underflow(self):
        with tempfile.TemporaryDirectory() as d:
            file=Path(d)/'underflow.nbc'
            file.write_text(json.dumps({'format':'nova-bytecode','version':1,'instructions':[['print']]}))
            proc=subprocess.run([str(NOVA),'exec',str(file)],capture_output=True,text=True,encoding='utf8',timeout=10)
            self.assertNotEqual(proc.returncode,0)
            self.assertIn('stack underflow',proc.stderr)

    def test_system_args_and_unicode_text_slice(self):
        with tempfile.TemporaryDirectory() as d:
            src=Path(d)/'параметри.nova'
            src.write_text('''import system
import text
show system.args()
show text.slice("Україна",0,3)
show text.slice("ażb",1)
show text.index_of("zażółć", "ół")
''',encoding='utf8')
            proc=subprocess.run([str(NOVA),'run',str(src),'--','foo','Żółw'],capture_output=True,text=True,encoding='utf8',timeout=10)
            self.assertEqual(proc.returncode,0,proc.stderr)
            self.assertEqual(proc.stdout,'["foo", "Żółw"]\nУкр\nżb\n3\n')

    @unittest.skipIf(os.name == 'nt', 'Windows bootstrap EXE compiled and validated by CI PowerShell smoke test')
    def test_native_compiler_and_native_bytecode_executable(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)
            novac=root/'novac-native'
            app=root/'bytecode-app'
            src=root/'source.nova'
            out=root/'result.nbc'
            src.write_text('let x = 6*7\npokaż "Żółw"\nshow x\n',encoding='utf8')
            p=subprocess.run(['sh',str(ROOT/'build_app.sh'),str(COMPILER),str(novac)],capture_output=True,text=True,encoding='utf8',timeout=90)
            self.assertEqual(p.returncode,0,p.stderr)
            p=subprocess.run([str(novac),str(src),str(out)],capture_output=True,text=True,encoding='utf8',timeout=12)
            self.assertEqual(p.returncode,0,p.stderr)
            p=subprocess.run(['sh',str(ROOT/'build_bytecode_app.sh'),str(out),str(app)],capture_output=True,text=True,encoding='utf8',timeout=90)
            self.assertEqual(p.returncode,0,p.stderr)
            src.unlink();out.unlink()
            p=subprocess.run([str(app)],capture_output=True,text=True,encoding='utf8',timeout=10)
            self.assertEqual(p.returncode,0,p.stderr)
            self.assertEqual(p.stdout,'Żółw\n42\n')

if __name__=='__main__':
    unittest.main()
