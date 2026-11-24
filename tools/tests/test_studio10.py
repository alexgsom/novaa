"""Native Studio and Windows release checks (static checks run on Linux;
actual build and smoke run in Windows CI)."""
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
STUDIO=ROOT/'studio/nova_studio.cpp'
WORKFLOW=ROOT/'.github/workflows/windows-acceptance.yml'
RUNNER=ROOT/'bin'/('nova-native.exe' if os.name=='nt' else 'nova-native')
COMPILER=ROOT/'compiler/novac_selfhost.nbc'

class Studio10Tests(unittest.TestCase):
    def test_editor_is_native_cpp_and_utf8(self):
        source=STUDIO.read_text(encoding='utf8')
        self.assertIn('int WINAPI wWinMain',source)
        self.assertIn('MultiByteToWideChar(CP_UTF8',source)
        self.assertIn('WideCharToMultiByte(CP_UTF8',source)
        self.assertIn('GetOpenFileNameW',source)
        self.assertIn('GetSaveFileNameW',source)
        self.assertNotIn('python.exe',source.lower())
        self.assertNotIn('tkinter',source.lower())

    def test_editor_launches_selfhosted_compiler_and_packer(self):
        source=STUDIO.read_text(encoding='utf8')
        self.assertIn('novac_selfhost.nbc',source)
        self.assertIn('CreateProcessW',source)
        self.assertIn('WM_APP+77',source)
        self.assertIn('std::thread',source)
        self.assertIn('TerminateProcess',source)
        self.assertIn('L"pack"',source)
        self.assertIn('L"exec"',source)

    def test_windows_release_includes_studio_and_selftest(self):
        build=(ROOT/'build_windows.ps1').read_text(encoding='utf8')
        cmake=(ROOT/'CMakeLists.txt').read_text(encoding='utf8')
        workflow=WORKFLOW.read_text(encoding='utf8')
        install=(ROOT/'install_windows.ps1').read_text(encoding='utf8')
        self.assertIn('nova_studio.cpp',build)
        self.assertIn('add_executable(NovaStudio WIN32',cmake)
        self.assertIn('--self-test',workflow)
        self.assertIn('bin/NovaStudio.exe',workflow)
        self.assertIn('NovaStudio.exe',install)

    def test_windows_ci_no_orphaned_exe_lines(self):
        # The 0.9 workflow accidentally contained two unattached bare EXE paths.
        workflow=WORKFLOW.read_text(encoding='utf8')
        self.assertNotIn('          $output = & ./bin/NovaPortable08Demo.exe\n            bin/NovaHello09.exe',workflow)
        self.assertIn('./tests/smoke_portable08.ps1',workflow)
        smoke=(ROOT/'tests/smoke_portable08.ps1').read_text(encoding='utf8')
        self.assertIn('$actual = & $target',smoke)

    def test_editor_accepts_drag_drop_and_command_line_paths(self):
        source=STUDIO.read_text(encoding='utf8')
        self.assertIn('WM_DROPFILES',source)
        self.assertIn('DragQueryFileW',source)
        self.assertIn('DragAcceptFiles',source)
        self.assertIn('CommandLineToArgvW',source)

    def test_file_menu_is_opt_in_not_default_association(self):
        register=(ROOT/'register_nova_files.ps1').read_text(encoding='utf8')
        unregister=(ROOT/'unregister_nova_files.ps1').read_text(encoding='utf8')
        self.assertIn('HKCU:',register)
        self.assertIn('SystemFileAssociations',register)
        self.assertIn('NovaStudio.exe',register)
        self.assertNotIn('\\.nova\\(Default)',register)
        self.assertIn('Remove-Item',unregister)

    @unittest.skipUnless(RUNNER.exists() and COMPILER.exists(), 'Native runner needed')
    def test_selfhosted_compiler_works_with_unicode_source_path(self):
        with tempfile.TemporaryDirectory(prefix='NovaStudio-Łódź-') as folder:
            src=Path(folder)/'Привіт-на-Windows.nova'
            bc=Path(folder)/'Привіт-на-Windows.nbc'
            src.write_text('нехай name = "Żółw"\npokaż name\n',encoding='utf8')
            cp=subprocess.run([str(RUNNER),'exec',str(COMPILER),'--max-steps','15000000','--',str(src),str(bc)],
                              capture_output=True,text=True,encoding='utf-8',timeout=35)
            self.assertEqual(cp.returncode,0,cp.stderr)
            output=subprocess.run([str(RUNNER),'exec',str(bc)],capture_output=True,text=True,encoding='utf-8',timeout=25)
            self.assertEqual(output.returncode,0,output.stderr)
            self.assertEqual(output.stdout.strip(),'Żółw')

if __name__=='__main__':unittest.main()
