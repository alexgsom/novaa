"""Preview 3: designer integration, explicit AI app generation and interactive bytecode stdin."""
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
RUNTIME=ROOT/'bin'/('nova-native.exe' if os.name=='nt' else 'nova-native')
COMPILER=ROOT/'compiler/novac_selfhost.nbc'

class StudioNext15Contract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.studio=(ROOT/'studio/nova_studio.cpp').read_text(encoding='utf-8')
        cls.designer=(ROOT/'studio/studio_designer15_win.hpp').read_text(encoding='utf-8')
        cls.model=(ROOT/'studio/studio_designer15.hpp').read_text(encoding='utf-8')

    def test_visual_designer_has_actual_mouse_layout_and_reviewed_export(self):
        for word in ('WM_LBUTTONDOWN','WM_MOUSEMOVE','SetCapture','form.move(',
                     'form.toNova()', 'MB_YESNO','WM_EXPORT_FORM','ADD_LABEL', 'WM_NCDESTROY'):
            self.assertIn(word,self.designer)
        for word in ('ui.window(', 'ui.place(', 'ui.run(app)', 'function ',
                     'ui.button(', 'ui.label(', 'ui.textarea(', 'novaString'):
            self.assertIn(word,self.model)
        self.assertIn('case ID_DESIGNER:nova_designer15::DesignerWindow::show(mainWindow)',self.studio)
        self.assertIn('newDocument(false);',self.studio)
        self.assertIn('Generated Nova app in a new unsaved tab',self.studio)

    def test_interactive_stdin_is_an_explicit_run_only_pipe(self):
        for token in ('stdinPipe','sendTerminalInput()', 'SetHandleInformation(inputWrite',
                      'if(interactive)stdinPipe=inputWrite','JOB_OUTPUT',
                      'EM_REPLACESEL', 'CTRL'):
            # Ctrl+Enter accelerator is represented by FCONTROL, not string CTRL.
            if token=='CTRL':continue
            self.assertIn(token,self.studio)
        self.assertIn("{FVIRTKEY|FCONTROL,VK_RETURN,ID_TERMINAL_SEND}",self.studio)
        self.assertIn('source.parent_path(),true);',self.studio)
        self.assertIn('name=="ask"', (ROOT/'src/nova_vm.hpp').read_text(encoding='utf-8'))

    def test_ai_project_requires_manual_prompt_and_approval(self):
        self.assertIn('if(mode==ID_AI_PROJECT)',self.studio)
        self.assertIn('extractNovaCode(r->answer)',self.studio)
        self.assertIn('MB_YESNO|MB_ICONQUESTION',self.studio)
        self.assertIn('No files will be written and the code will NOT be run',self.studio)
        self.assertNotIn('sk-proj-',self.studio)

    def test_ci_and_windows_packaging_include_designer(self):
        cmake=(ROOT/'CMakeLists.txt').read_text()
        self.assertIn('nova-studio-designer15-tests',cmake)
        for flow in ('native-build.yml','windows-acceptance.yml'):
            self.assertIn('nova-studio-designer15-tests',(ROOT/'.github/workflows'/flow).read_text())
        pack=(ROOT/'tools/create_windows_portable.ps1').read_text()
        for name in ('NOVA_STUDIO_NEXT_15_UA.md','designer_demo15.nova','interactive_ask15.nova'):
            self.assertIn(name,pack)

@unittest.skipUnless(RUNTIME.exists() and COMPILER.exists(), 'Native runtime and self-hosted compiler required')
class Preview15Runtime(unittest.TestCase):
    def compile(self, source):
        dest=Path(self.temp.name)/'program.nbc'
        p=subprocess.run([str(RUNTIME),'exec',str(COMPILER),'--max-steps','15000000','--',str(source),str(dest)],
                         cwd=ROOT,capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=60)
        self.assertEqual(p.returncode,0,p.stderr)
        return dest

    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)

    def test_generated_visual_form_compiles_to_real_gui_bytecode(self):
        import json
        dest=self.compile(ROOT/'examples/studio_next/designer_demo15.nova')
        ins=json.loads(dest.read_text('utf-8'))['instructions']
        self.assertIn(['call','ui.window',3],ins)
        self.assertIn(['call','ui.button',3],ins)
        self.assertEqual(ins.count(['call','ui.place',5]),3)

    def test_compiled_ask_reads_utf8_from_process_stdin(self):
        dest=self.compile(ROOT/'examples/studio_next/interactive_ask15.nova')
        p=subprocess.run([str(RUNTIME),'exec',str(dest)],input='Оля\n',
                         capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=20)
        self.assertEqual(p.returncode,0,p.stderr)
        self.assertIn('Вітаю, Оля!',p.stdout)

if __name__=='__main__':unittest.main()
