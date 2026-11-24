"""Nova Studio Next Preview 1 source/CI acceptance (Windows GUI smoke runs on Windows CI)."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]

class StudioNext13(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.studio=(ROOT/'studio/nova_studio.cpp').read_text(encoding='utf-8')
        cls.win=(ROOT/'studio/nova_ai_win.hpp').read_text(encoding='utf-8')
        cls.header=(ROOT/'studio/nova_ai.hpp').read_text(encoding='utf-8')

    def test_three_pane_workspace(self):
        for key in ('ID_PROJECT_LIST','ID_AI_LOG','ID_AI_INPUT','refreshProject()',
                    'WM_DRAWITEM','SHELL_BG','EDIT_BG','Choose the folder',
                    'ID_QUICK_ACTIONS','Ctrl+Shift+P'):
            self.assertIn(key,self.studio)

    def test_ai_is_opt_in_and_code_is_selection_only(self):
        self.assertIn('if(mode==ID_AI_ASK)',self.studio)
        self.assertIn('if(last<=first)',self.studio)
        self.assertIn('doc.substr(first,last-first)',self.studio)
        self.assertIn('Select Nova code in the editor first',self.studio)
        self.assertIn('ID_AI_COPY:copyAi()',self.studio)
        self.assertIn('OPENAI_API_KEY',self.studio)

    def test_api_is_https_and_does_not_embed_key(self):
        self.assertIn('WINHTTP_FLAG_SECURE',self.win)
        self.assertIn('api.openai.com',self.win)
        self.assertIn('/v1/responses',self.win)
        self.assertIn('WinHttpSetTimeouts',self.win)
        self.assertIn('GetEnvironmentVariableW',self.win)
        self.assertNotIn('sk-proj-',self.win)
        self.assertNotIn('sk-proj-',self.studio)

    def test_portable_distribution_includes_ai_launcher(self):
        package=(ROOT/'tools/create_windows_portable.ps1').read_text(encoding='utf-8')
        self.assertIn("'start_nova_ai.ps1'",package)
        self.assertIn("'NOVA_STUDIO_NEXT_13_UA.md'",package)
        self.assertIn("'examples/studio_next/hello_next.nova'",package)

    def test_build_dependencies_and_both_ci_targets(self):
        cmake=(ROOT/'CMakeLists.txt').read_text(encoding='utf-8')
        ps1=(ROOT/'build_windows.ps1').read_text(encoding='utf-8')
        for lib in ('winhttp','ole32'):
            self.assertIn(lib,cmake)
            self.assertIn(lib+'.lib',ps1)
        for ci in ('native-build.yml','windows-acceptance.yml'):
            workflow=(ROOT/'.github/workflows'/ci).read_text(encoding='utf-8')
            self.assertIn('nova-studio-ai-tests',workflow)

if __name__=='__main__':unittest.main()
