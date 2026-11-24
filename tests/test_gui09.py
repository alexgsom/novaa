"""Compiled Win32 UI bridge regression tests (Python runs tests only, not Nova)."""
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
EXAMPLES=ROOT/'examples'/'windows09'
RUNNER=ROOT/'bin'/('nova-native.exe' if os.name=='nt' else 'nova-native')
COMPILER=ROOT/'compiler'/'novac_selfhost.nbc'


def invoke(*parts, timeout=40):
    return subprocess.run([str(p) for p in parts],capture_output=True,text=True,
                          encoding='utf-8', errors='replace',timeout=timeout)


@unittest.skipUnless(RUNNER.exists() and COMPILER.exists(), 'Native runtime must be built')
class CompiledGui09Tests(unittest.TestCase):
    def setUp(self):
        tmp=tempfile.TemporaryDirectory(prefix='Nova-GUI-Żółw-')
        self.addCleanup(tmp.cleanup)
        self.dest=Path(tmp.name)

    def compile(self, file):
        output=self.dest/(file+'.nbc')
        result=invoke(RUNNER,'exec',COMPILER,'--max-steps','15000000', '--',EXAMPLES/file,output)
        self.assertEqual(result.returncode,0,result.stderr)
        return json.loads(output.read_text(encoding='utf8')),output

    def test_01_hello_callbacks_compiled(self):
        data,path=self.compile('hello_gui.nova')
        instructions=data['instructions']
        self.assertIn(['import','ui'],instructions)
        self.assertIn(['call','ui.button',3],instructions)
        self.assertIn(['call','ui.run',1],instructions)
        self.assertIn(['load','greet'],instructions)
        self.assertIn(['call','heading.set',1],instructions)
        self.assertIn(['call','person.get',0],instructions)

    def test_02_counter_has_two_callbacks_and_shared_state(self):
        data,_=self.compile('counter_gui.nova')
        commands=[i[0] for i in data['instructions']]
        self.assertEqual(commands.count('function'),2)
        self.assertIn(['set_attr','count'],data['instructions'])
        self.assertIn(['call','count_text.set',1],data['instructions'])

    def test_03_notes_and_menu_compile(self):
        data,_=self.compile('notes_gui.nova')
        self.assertIn(['import','storage'],data['instructions'])
        self.assertIn(['call','ui.menu',4],data['instructions'])
        self.assertIn(['call','storage.write',2],data['instructions'])

    def test_04_noninteractive_smoke_compiles(self):
        data,path=self.compile('ui_smoke.nova')
        self.assertIn(['call','ui.checkbox',3],data['instructions'])
        self.assertIn(['call','ui.select',2],data['instructions'])
        if os.name=='nt':
            result=invoke(RUNNER,'exec',path,timeout=30)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertIn('WIN32_UI_OK',result.stdout)
        else:
            result=invoke(RUNNER,'exec',path)
            self.assertNotEqual(result.returncode,0)
            self.assertIn('available on Windows only',result.stderr)

    def test_05_unsupported_ui_rejected_on_non_windows(self):
        if os.name=='nt':self.skipTest('Linux-specific error')
        path=self.dest/'bad.nbc'
        path.write_text(json.dumps({'format':'nova-bytecode','version':1,
           'instructions':[['import','ui'],['push','Hello'],['call','ui.window',1],['pop'],['halt']]}))
        result=invoke(RUNNER,'exec',path)
        self.assertIn('available on Windows only',result.stderr)

    def test_06_unimported_ui_call_fails(self):
        path=self.dest/'bad.nbc'
        path.write_text(json.dumps({'format':'nova-bytecode','version':1,
           'instructions':[['push','Hello'],['call','ui.window',1],['pop'],['halt']]}))
        result=invoke(RUNNER,'exec',path)
        self.assertNotEqual(result.returncode,0)
        self.assertIn('module not imported',result.stderr)

    def test_07_unknown_function_rejected(self):
        path=self.dest/'bad.nbc'
        path.write_text(json.dumps({'format':'nova-bytecode','version':1,
           'instructions':[['load','unknown_greet'],['halt']]}))
        result=invoke(RUNNER,'exec',path)
        self.assertNotEqual(result.returncode,0)
        self.assertIn('undefined variable',result.stderr)

    def test_08_bundled_gui_program_integrity(self):
        if os.name=='nt':self.skipTest('Generic pack test uses Linux runner; Windows GUI runner is separate')
        _,bytecode=self.compile('hello_gui.nova')
        exe=self.dest/'hello-gui'
        result=invoke(RUNNER,'pack',bytecode,exe)
        self.assertEqual(result.returncode,0,result.stderr)
        result=invoke(RUNNER,'inspect',exe)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertIn('Integrity: OK',result.stdout)
        result=invoke(exe)
        self.assertIn('available on Windows only',result.stderr)

    def test_09_self_host_still_bit_identical(self):
        first=self.dest/'compiler.nbc'
        result=invoke(RUNNER,'exec',COMPILER,'--max-steps','15000000','--',ROOT/'compiler/novac.nova',first,timeout=75)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertEqual(first.read_bytes(),COMPILER.read_bytes())

    def test_10_function_as_first_class_callback_is_available(self):
        # Non-Windows test: function values are intentionally restricted to a marker object.
        path=self.dest/'function.nbc'
        doc={'format':'nova-bytecode','version':1,'instructions':[
            ['function','hello',[],5],['push',5],['print'],['push',None],['ret'],
            ['load','hello'],['get_attr','__nova_function'],['print'],['halt']]}
        path.write_text(json.dumps(doc))
        result=invoke(RUNNER,'exec',path)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertEqual(result.stdout,'hello\n')

if __name__=='__main__':unittest.main()
