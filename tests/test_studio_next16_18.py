"""Updates 16–18: GUI integration contract and identical Nova bytecode across three languages."""
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
ENGINE=ROOT/'bin'/('nova-native.exe' if os.name=='nt' else 'nova-native')
COMPILER=ROOT/'compiler/novac_selfhost.nbc'

class StudioNext16To18(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.studio=(ROOT/'studio/nova_studio.cpp').read_text(encoding='utf-8')
        cls.designer=(ROOT/'studio/studio_designer15_win.hpp').read_text(encoding='utf-8')
        cls.trinity=(ROOT/'studio/studio_trinity16.hpp').read_text(encoding='utf-8')
        cls.living=(ROOT/'studio/studio_living17.hpp').read_text(encoding='utf-8')
        cls.proof=(ROOT/'studio/studio_aiproof18.hpp').read_text(encoding='utf-8')

    def test_trinity_views_are_new_unsaved_tabs(self):
        for token in ('ID_TRINITY_EN', 'ID_TRINITY_UK', 'ID_TRINITY_PL',
                      'nova_trinity16::translate', 'newDocument(false)'):
            self.assertIn(token,self.studio)
        for token in ('comment=true', 'if(quote)', 'previousNonSpace', 'leadingOnly'):
            self.assertIn(token,self.trinity)

    def test_living_ui_roundtrip_never_overwrites_handwritten_source(self):
        for token in ('exportEditable(', 'importEditable(', 'form.toNova()',
                      'sourceSnapshot', 'WM_APP+81'):
            self.assertIn(token,self.designer+self.studio+self.living)
        self.assertIn('if(exportEditable(form)!=source)',self.living)
        self.assertIn('editorText(editor)!=change->originalSnapshot',self.studio)
        self.assertIn('MB_YESNO|MB_ICONQUESTION',self.studio)

    def test_ai_proof_compiles_not_executes_candidate(self):
        for token in ('startProof(', 'nova_aiproof18::prepare',
                      'PROOF_DONE', 'Compile-only check',
                      'nova_aiproof18::stillApplies'):
            self.assertIn(token,self.studio+self.proof)
        self.assertIn('input.wstring(),bytecode.wstring()',self.studio)
        self.assertIn('No generated code was executed',self.studio)
        self.assertIn('if(!proof->compiled)',self.studio)

    def test_ci_builds_all_new_test_targets(self):
        for flow in ('.github/workflows/native-build.yml','.github/workflows/windows-acceptance.yml'):
            source=(ROOT/flow).read_text(encoding='utf-8')
            for name in ('nova-studio-trinity16-tests','nova-studio-living17-tests','nova-studio-aiproof18-tests'):
                self.assertIn(name,source)

@unittest.skipUnless(ENGINE.exists() and COMPILER.exists(),'Native runtime and self-hosted compiler required')
class TrinityBytecodeEquivalence(unittest.TestCase):
    def test_three_languages_compile_byte_identically_and_run(self):
        outputs=[];compiled=[]
        with tempfile.TemporaryDirectory() as folder:
            for lang in ('en','uk','pl'):
                source=ROOT/f'examples/studio_next/trinity16_{lang}.nova'
                out=Path(folder)/f'{lang}.nbc'
                result=subprocess.run([str(ENGINE),'exec',str(COMPILER),'--max-steps','15000000',
                                       '--',str(source),str(out)],cwd=ROOT,capture_output=True,
                                      encoding='utf-8',errors='replace',timeout=60)
                self.assertEqual(result.returncode,0,result.stderr)
                compiled.append(out.read_bytes())
                run=subprocess.run([str(ENGINE),'exec',str(out)],cwd=ROOT,capture_output=True,
                                   encoding='utf-8',errors='replace',timeout=20)
                self.assertEqual(run.returncode,0,run.stderr)
                outputs.append(run.stdout)
        self.assertEqual(compiled[0],compiled[1])
        self.assertEqual(compiled[0],compiled[2])
        self.assertEqual(outputs[0],outputs[1])
        self.assertEqual(outputs[0],outputs[2])
        self.assertIn('15',outputs[0])

if __name__=='__main__':unittest.main()
