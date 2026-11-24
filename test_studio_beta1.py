"""Nova Studio Beta 1: compile real portable C++ text logic and check Windows wiring.
Running these tests on Linux is NOT a substitute for a Windows GUI smoke test.
"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT/'studio/nova_studio.cpp'
TEXT = ROOT/'studio/studio_text.hpp'
NATIVE = ROOT/'tests/studio_text_native.cpp'

class StudioBeta1Tests(unittest.TestCase):
    def test_portable_search_and_highlighting_compile_and_run(self):
        with tempfile.TemporaryDirectory(prefix='nova-studio-test-') as temp:
            output = Path(temp)/('studio_text_test.exe' if os.name=='nt' else 'studio_text_test')
            compiler = shutil.which('g++') or shutil.which('clang++')
            if compiler:
                command = [compiler, '-std=c++17','-O2','-Wall','-Wextra','-Werror',str(NATIVE),'-o',str(output)]
            elif os.name=='nt' and shutil.which('cl.exe'):
                command = ['cl.exe','/nologo','/EHsc','/std:c++17', '/utf-8', '/W4', f'/Fe:{output}', str(NATIVE)]
            else:
                self.skipTest('No C++17 compiler installed')
            compile_result = subprocess.run(command,cwd=temp,capture_output=True,text=True,timeout=40)
            self.assertEqual(compile_result.returncode,0,compile_result.stdout+compile_result.stderr)
            run_result = subprocess.run([str(output)],capture_output=True,text=True,timeout=15)
            self.assertEqual(run_result.returncode,0,run_result.stdout+run_result.stderr)

    def test_studio_has_real_rich_edit_and_keyword_colorization(self):
        source=SRC.read_text(encoding='utf8')
        self.assertIn('LoadLibraryW(L"Msftedit.dll")',source)
        self.assertIn('L"RICHEDIT50W"',source)
        self.assertIn('EM_SETCHARFORMAT',source)
        self.assertIn('nova_studio::tokenize',source)
        self.assertIn('HIGHLIGHT_TIMER',source)
        self.assertIn('positions[doc.size()]=position',source)

    def test_native_search_and_replace_dialog_integration(self):
        source=SRC.read_text(encoding='utf8')
        self.assertIn('FindTextW(&searchDialog)',source)
        self.assertIn('ReplaceTextW(&searchDialog)',source)
        self.assertIn('FINDMSGSTRINGW',source)
        self.assertIn('FR_REPLACEALL',source)
        self.assertIn('nova_studio::replaceAll',source)
        self.assertIn('IsDialogMessageW(findWindow,&msg)',source)

    def test_editor_has_navigation_shortcuts_and_caret(self):
        source=SRC.read_text(encoding='utf8')
        self.assertIn('ID_NEXT',source)
        self.assertIn('ID_PREVIOUS',source)
        self.assertIn('ID_FIND',source)
        self.assertIn('ID_REPLACE',source)
        self.assertIn('EN_SELCHANGE',source)
        self.assertIn('EM_LINEFROMCHAR',source)

    def test_editor_rejects_nul_and_limits_output_and_syntax_pass(self):
        source=SRC.read_text(encoding='utf8')
        self.assertIn('NUL bytes are not allowed',source)
        self.assertIn('Output truncated after 1 MiB',source)
        self.assertIn('doc.size()>128*1024',source)
        self.assertIn('nova_studio::tokenize(doc,3000)',source)

    def test_exe_rebuild_is_staged_and_atomic(self):
        source=SRC.read_text(encoding='utf8')
        self.assertIn('L".nova-building-"',source)
        self.assertIn('MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH',source)
        self.assertIn('fs::remove(staged,ec)',source)
        self.assertIn('Windows could not replace the EXE',source)

    def test_native_studio_selftest_covers_text_helpers(self):
        source=SRC.read_text(encoding='utf8')
        self.assertIn('nova_studio::findNext(L"Київ Łódź"',source)
        self.assertIn('nova_studio::replaceAll(L"Nova nova"',source)
        self.assertIn('nova_studio::tokenize(L"покажи',source)

    def test_native_text_module_does_not_depend_on_windows_or_python(self):
        header=TEXT.read_text(encoding='utf8')
        self.assertNotIn('#include <windows.h>',header)
        self.assertNotIn('Python.h',header)
        self.assertIn('case L\'Ł\'',header)
        self.assertIn('case L\'Ż\'',header)

if __name__=='__main__':
    unittest.main()
