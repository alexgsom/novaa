"""Next 14 source-level UI contract tests; native behavior covered by CTest."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[1]
class StudioNext14(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.studio=(ROOT/'studio/nova_studio.cpp').read_text(encoding='utf-8')
        cls.cmake=(ROOT/'CMakeLists.txt').read_text(encoding='utf-8')
    def test_tabs_store_separate_buffers_and_confirm_on_close(self):
        for token in ('struct Document','std::vector<Document> documents','captureDocument()',
                      'switchDocument(', 'closeAllDocuments()', 'TCN_SELCHANGE',
                      'ID_TAB_NEXT','ID_TAB_CLOSE','Ctrl+Tab','Ctrl+W'):
            self.assertIn(token,self.studio)
    def test_smart_editor_and_clickable_errors(self):
        for token in ('EN_MSGFILTER','insertAutoIndentedNewLine()',
                      'completeKeyword()', 'ID_COMPLETE','ID_PROBLEMS',
                      'populateProblems(', 'goToProblem()', 'LBN_DBLCLK'):
            self.assertIn(token,self.studio)
        self.assertIn('studio/studio_next14.hpp', (ROOT/'tests/studio_next14_native.cpp').read_text().replace('../studio/studio_next14.hpp','studio/studio_next14.hpp'))
    def test_review_first_ai_never_applies_in_callback(self):
        for token in ('previewAiFix()', 'aiFixSnapshot', 'current!=aiFixSnapshot',
                      'extractNovaCode(lastAiAnswer)', 'MB_YESNO',
                      'case ID_AI_PREVIEW:previewAiFix()'):
            self.assertIn(token,self.studio)
        before=self.studio.split('if(m==AI_DONE){',1)[1].split('if(m==JOB_DONE)',1)[0]
        self.assertNotIn('EM_REPLACESEL',before)
    def test_windows_link_and_smoke(self):
        self.assertIn('comctl32',self.cmake)
        self.assertIn('comctl32.lib',(ROOT/'build_windows.ps1').read_text(encoding='utf-8'))
        self.assertIn('SMOKE_TABS',self.studio)
        self.assertIn('nova-studio-next14-tests',self.cmake)
        for workflow in ('windows-acceptance.yml','native-build.yml'):
            self.assertIn('nova-studio-next14-tests',
                (ROOT/'.github/workflows'/workflow).read_text(encoding='utf-8'))
        package=(ROOT/'tools/create_windows_portable.ps1').read_text(encoding='utf-8')
        self.assertIn('NOVA_STUDIO_NEXT_14_UA.md',package)
if __name__=='__main__':unittest.main()
