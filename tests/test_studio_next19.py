"""Static integration contract for Windows-specific Preview 19 controls.
Actual Win32 executable smoke runs on the Windows CI job.
"""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
class Next19(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.studio=(ROOT/'studio/nova_studio.cpp').read_text(encoding='utf-8')
        cls.designer=(ROOT/'studio/studio_designer15_win.hpp').read_text(encoding='utf-8')
        cls.native=(ROOT/'studio/studio_next19.hpp').read_text(encoding='utf-8')
        cls.ai=(ROOT/'studio/nova_ai.hpp').read_text(encoding='utf-8')
    def test_editor_minimap_and_line_navigation(self):
        for item in ('drawSidePane(', 'miniCache=', 'goToLine(', 'ID_SYMBOLS', 'navigateSymbols(',
                     'applyBackground(', 'smartSuggestions(', 'ID_PROJECT_SEARCH', 'searchProject('):
            self.assertIn(item,self.studio)
    def test_ai_requires_approval_and_verification(self):
        for item in ('collectProjectContext(', 'AI — Explicit project source consent',
                     'parseMultiFile(', 'showMultiReview(', 'startMultiProof(',
                     'MULTI_DONE', 'MoveFileExW(', 'if(!result->passed)',
                     'Project file changed after AI request'):
            self.assertIn(item,self.studio)
        self.assertIn('maxOutputTokens=mode==ID_AI_MULTI?8000:1100',self.studio)
        self.assertIn('maxOutputTokens>10000',self.ai)
    def test_designer_preview_grid_and_property_editor(self):
        for item in ('gridEnabled','CreatePen(PS_SOLID,1,RGB(235,239,246))','applyProperties(',
                     'showPreview(', 'loadTemplate(', 'resizing=', 'TEMPLATE_LOGIN',
                     'TEMPLATE_FEEDBACK', 'TEMPLATE_DASHBOARD'):
            self.assertIn(item,self.designer)
    def test_msvc_gui_source_compilation_regressions(self):
        # Do not embed a physical newline inside a C++ wide-string literal.
        self.assertIn('starter template?\\n"',self.designer)
        self.assertNotIn('starter template?\nUnsaved',self.designer)
        # Windows RECT uses LONG; std::max needs arguments of the same type.
        self.assertIn('std::max(1,(int)rect.bottom)',self.studio)
        self.assertIn('std::max(1,(int)rect.right)',self.studio)
    def test_windows_ci_targets_and_portable_suite(self):
        for flow in (ROOT/'.github/workflows/windows-acceptance.yml',ROOT/'.github/workflows/native-build.yml'):
            text=flow.read_text(encoding='utf-8')
            self.assertIn('nova-studio-next19-tests',text)
            self.assertIn('nova-studio-designer19-tests',text)
        self.assertIn('safeRelativeNova',self.native)
    def test_windows_gui_smoke_checks_controls(self):
        for item in ('!IsWindow(miniMap)','!IsWindow(gutter)','!IsWindow(projectSearch)','!IsWindow(btnAiMulti)'):
            self.assertIn(item,self.studio)
if __name__=='__main__':unittest.main()
