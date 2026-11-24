"""Nova-authored stage-1 compiler and native VM: functions, control flow, and safety."""
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
NOVA=ROOT/'bin'/('nova-native.exe' if os.name=='nt' else 'nova-native')
NOVAC=ROOT/'compiler'/'novac.nova'

class Bootstrap06Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not NOVA.exists():raise unittest.SkipTest('Build native engine first')

    def program(self,source,compare=True,steps=100000):
        with tempfile.TemporaryDirectory(prefix='Nova06-Żółw-') as d:
            f=Path(d)/'Україна.nova';output=Path(d)/'wynik.nbc'
            f.write_text(source,encoding='utf8')
            compile_result=subprocess.run([str(NOVA),'run',str(NOVAC),'--max-steps','4000000','--',str(f),str(output)],capture_output=True,text=True,encoding='utf8',timeout=30)
            if compile_result.returncode:return compile_result,None,None,None
            vm=subprocess.run([str(NOVA),'exec',str(output),'--max-steps',str(steps)],capture_output=True,text=True,encoding='utf8',timeout=15)
            ref=None
            if compare:
                ref=subprocess.run([str(NOVA),'run',str(f),'--max-steps',str(steps)],capture_output=True,text=True,encoding='utf8',timeout=15)
            return compile_result,vm,ref,json.loads(output.read_text(encoding='utf8'))

    def match(self,source,expected):
        build,vm,ref,bc=self.program(source)
        self.assertEqual(build.returncode,0,build.stderr)
        self.assertEqual(vm.returncode,0,vm.stderr)
        self.assertEqual(ref.returncode,0,ref.stderr)
        self.assertEqual(vm.stdout,expected)
        self.assertEqual(vm.stdout,ref.stdout)
        self.assertEqual(bc['format'],'nova-bytecode')
        self.assertEqual(bc['version'],1)

    def test_function_parameters_and_multilingual(self):
        self.match('''функція greet(person, times)
  let msg = "Привіт, " + person
  repeat times
    покажи msg
  кінець
  поверни times + 1
кінець
pokaż greet("Żółw", 2)
''','Привіт, Żółw\nПривіт, Żółw\n3\n')

    def test_recursive_factorial(self):
        self.match('''function fac(n)
    if n <= 1
        return 1
    end
    return n * fac(n - 1)
end
show fac(6)
''','720\n')

    def test_fibonacci_recursive(self):
        self.match('''funkcja fib(n)
    jeśli n <= 1
        zwróć n
    koniec
    zwróć fib(n - 1) + fib(n - 2)
koniec
pokaż fib(9)
''','34\n')

    def test_nested_calls_and_builtin_len_unicode(self):
        self.match('''function decorate(x)
    return "[" + x + "]"
end
show decorate(decorate("Łódź"))
show len(decorate("żółw"))
show int("12") + len("Привіт")
show str(20) + "!"
''','[[Łódź]]\n6\n18\n20!\n')

    def test_nested_call_arguments(self):
        self.match('''function sum3(a, b, c)
    return a + b + c
end
show sum3(1 + 2, sum3(1, 2, 3), 7)
''','16\n')

    def test_function_default_return(self):
        self.match('''function no_result()
    let temp = 6
end
show no_result()
''','None\n')

    def test_globals_readable_local_mutation_is_isolated(self):
        self.match('''let a = 42
function demo()
    show a
    let a = 7
    return a
end
show demo()
show a
''','42\n7\n42\n')

    def test_forward_declared_function(self):
        build,vm,ref,_=self.program('show make(6)\nfunction make(x)\nreturn x * 3\nend\n',compare=False)
        self.assertEqual(build.returncode,0,build.stderr)
        self.assertEqual(vm.returncode,0,vm.stderr)
        self.assertEqual(vm.stdout,'18\n')

    def test_local_params_and_recursion_do_not_leak(self):
        build,vm,ref,_=self.program('function id(x)\nreturn x\nend\nshow id(3)\nshow x\n')
        self.assertEqual(build.returncode,0,build.stderr)
        self.assertNotEqual(vm.returncode,0)
        self.assertIn('undefined variable: x',vm.stderr)

    def test_break_continue_inside_nested_if_repeat(self):
        self.match('''let total = 0
repeat 7
    total = total + 1
    if total == 2
        continue
    end
    if total == 4
        break
    end
    show total
end
''','1\n3\n')

    def test_short_circuit_returns_operands_and_skips_errors(self):
        self.match('''show 0 and (10 / 0)
show "ok" or (10 / 0)
show 0 or 5
show 4 and 9
show false or "так"
show true and false or 17
''','0\nok\n5\n9\nтак\n17\n')

    def test_nested_short_circuit_and_calls(self):
        self.match('''function double(x)
    return x * 2
end
show 0 or (false or double(9))
show 1 and (2 and double(3))
show 1 or double(100)
''','18\n6\n1\n')

    def test_assert_true(self):
        self.match('''перевір 2 + 3 == 5
show "passed"
''','passed\n')

    def test_raise_and_assert_failure(self):
        for source,phrase in [('raise "halt"\n','raised: halt'),('assert 4 == 5\n','assertion failed')]:
            with self.subTest(source=source):
                build,vm,ref,_=self.program(source)
                self.assertEqual(build.returncode,0,build.stderr)
                self.assertNotEqual(vm.returncode,0)
                self.assertIn(phrase,vm.stderr)
                self.assertNotEqual(ref.returncode,0)

    def test_recursive_depth_guard(self):
        src='function recurse()\nreturn recurse()\nend\nshow recurse()\n'
        build,vm,_,_=self.program(src,compare=False,steps=100000)
        self.assertEqual(build.returncode,0,build.stderr)
        self.assertNotEqual(vm.returncode,0)
        self.assertIn('call depth limit',vm.stderr)

    def test_invalid_source_functions(self):
        bad=[
            'function name(a, a)\nreturn a\nend\n',
            'return 5\n',
            'break\n',
            'continue\n',
            'function outer()\nfunction inner()\nend\nend\n',
            'function broken(a,)\nend\n',
            'function no_closing(a)\nreturn a\n',
            'show add(1,)\n',
            'show 1, 2\n',
            'show (2,3)\n',
            'show f(1,,2)\n',
        ]
        for src in bad:
            with self.subTest(source=src):
                build,vm,_,_=self.program(src,compare=False)
                self.assertNotEqual(build.returncode,0)
                self.assertIsNone(vm)

    def test_invalid_vm_calls_and_function_definitions(self):
        broken=[
            [['ret']],
            [['call','missing',0]],
            [['call','len',-1]],
            [['call','len',100]],
            [['function','f',[],100],['halt']],
            [['function','f',[],0],['halt']],
            [['function','f',[],3],['push',None],['ret'],['function','f',[],6],['push',None],['ret'],['halt']],
            [['function','f',['a','a'],3],['push',None],['ret'],['halt']],
            [['function','f',[],3],['push',None],['ret'],['call','f',1],['halt']],
        ]
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'bad.nbc'
            for code in broken:
                with self.subTest(code=code):
                    p.write_text(json.dumps({'format':'nova-bytecode','version':1,'instructions':code}),encoding='utf8')
                    run=subprocess.run([str(NOVA),'exec',str(p)],capture_output=True,text=True,encoding='utf8',timeout=12)
                    self.assertNotEqual(run.returncode,0)
                    self.assertIn('Nova Native:',run.stderr)

    def test_function_call_errors(self):
        cases=[('function plus(x)\nreturn x\nend\nshow plus(1, 2)\n','incorrect argument count'),
               ('show missing(5)\n','undefined function')]
        for src,expected in cases:
            with self.subTest(src=src):
                build,vm,_,_=self.program(src,compare=False)
                self.assertEqual(build.returncode,0,build.stderr)
                self.assertNotEqual(vm.returncode,0)
                self.assertIn(expected,vm.stderr)

    def test_for_range_and_localized_keywords(self):
        self.match('''let total = 0
для n в range(1, 8, 2)
    total = total + n
кінець
dla letter w "Żółw"
    pokaż letter
koniec
show total
''','Ż\nó\nł\nw\n16\n')

    def test_nested_for_and_repeat(self):
        self.match('''let count = 0
for a in range(2)
    repeat 3
        count = count + 1
    end
end
show count
''','6\n')

    def test_for_break_continue(self):
        self.match('''let sum = 0
for x in range(1, 10)
    if x == 2
        continue
    end
    if x > 4
        break
    end
    sum = sum + x
end
show sum
''','8\n')

    def test_list_literals_nested_and_indices(self):
        self.match('''let values = [1, "Два", [3, 4]]
show values
show values[1]
show values[-1][0]
show "Żółw"[-2]
show len([])
show [1, 2, 3][0]
''','[1, "Два", [3, 4]]\nДва\n3\nł\n0\n1\n')

    def test_for_list_unicode_and_nested_index(self):
        self.match('''let total = 0
for item in [[1, 2], [3, 4]]
    total = total + item[0]
end
show total
''','4\n')

    def test_index_and_loop_errors(self):
        for src,expected in [
            ('show [1][2]\n','index out of range'),
            ('show 42[0]\n','indexing requires list'),
            ('for i in 10\n show i\nend\n','for expects list'),
            ('show range(1, 9, 0)\n','range step cannot be zero'),
        ]:
            with self.subTest(source=src):
                build,vm,_,_=self.program(src,compare=False)
                self.assertEqual(build.returncode,0,build.stderr)
                self.assertNotEqual(vm.returncode,0)
                self.assertIn(expected,vm.stderr)

    def test_nested_list_literals_with_calls_and_boolean(self):
        self.match('''function pick(x)
    return x[0] + x[1]
end
show pick([2 + 3, 4])
show [0 and (1 / 0), 1 or (1 / 0)]
''','9\n[0, 1]\n')

    def test_invalid_list_sources(self):
        bad = ['show [1,]\n','show [,1]\n','show [1,,2]\n','show [1\n',
               'show 1]\n','show [3][1,2]\n','show [] [0,1]\n']
        for src in bad:
            with self.subTest(source=src):
                build,vm,_,_=self.program(src,compare=False)
                self.assertNotEqual(build.returncode,0)
                self.assertIsNone(vm)

    def test_vm_legacy_bytecode_still_runs(self):
        src=ROOT/'examples'/'bootstrap'/'trilingual.nbc'
        proc=subprocess.run([str(NOVA),'exec',str(src)],capture_output=True,text=True,encoding='utf8',timeout=10)
        self.assertEqual(proc.returncode,0,proc.stderr)
        self.assertIn('Привіт, Оля',proc.stdout)

if __name__=='__main__':unittest.main()
