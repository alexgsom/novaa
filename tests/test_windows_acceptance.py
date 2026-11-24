"""Portable tests of Windows artifact verifier and CI entry points."""
from pathlib import Path
import importlib.util
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('verify_pe', ROOT / 'tools/verify_pe.py')
verify_pe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verify_pe)


def mock_pe(*, subsystem=2, machine=0x8664, magic=0x20B):
    buf = bytearray(512)
    buf[:2] = b'MZ'
    struct.pack_into('<I', buf, 0x3c, 0x80)
    buf[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HH',buf,0x84,machine,3)
    struct.pack_into('<H',buf,0x80+20,112)
    struct.pack_into('<H',buf,0x98,magic)
    struct.pack_into('<H',buf,0x98+68,subsystem)
    return bytes(buf)


class WindowsAcceptanceTests(unittest.TestCase):
    def test_expected_pe32_plus_gui_and_console(self):
        self.assertEqual(verify_pe.inspect_pe(mock_pe(subsystem=2))['subsystem'], 'GUI')
        self.assertEqual(verify_pe.inspect_pe(mock_pe(subsystem=3))['subsystem'], 'Console')

    def test_rejects_corruption_wrong_arch_and_subsystem(self):
        for data in (b'', b'MZ', b'ELF', mock_pe(machine=0x14c),
                     mock_pe(magic=0x10b), mock_pe(subsystem=1), mock_pe()[:100]):
            with self.subTest(size=len(data)):
                with self.assertRaises(verify_pe.PEError):
                    verify_pe.inspect_pe(data)

    def test_pipeline_defines_real_windows_runner_and_artifact(self):
        workflow = (ROOT / '.github/workflows/windows-acceptance.yml').read_text(encoding='utf8')
        self.assertIn('runs-on: windows-2022', workflow)
        self.assertIn('build_windows.ps1 -Compiler MSVC', workflow)
        self.assertIn('smoke_win32_click.ps1', workflow)
        self.assertIn('smoke_selfhost07.ps1', workflow)
        self.assertIn('actions/upload-artifact@v4', workflow)
        self.assertIn('nova-windows-x64', workflow)

    def test_studio_has_real_win32_editor_smoke(self):
        studio = (ROOT / 'studio/nova_studio.cpp').read_text(encoding='utf8')
        self.assertIn('guiSmoke',studio)
        self.assertIn('IsWindow(mainWindow)',studio)
        self.assertIn('SendMessageW(mainWindow,WM_COMMAND',studio)
        self.assertIn('--gui-smoke',studio)

    def test_gui_click_smoke_invokes_real_button_command(self):
        script = (ROOT / 'tests/smoke_win32_click.ps1').read_text(encoding='utf8')
        self.assertIn('0x00F5', script)
        self.assertIn('SendMessageTimeout', script)
        self.assertIn('CLICKED_OK_Київ_Łódź', script)
        program = (ROOT / 'tests/windows_gui_click.nova').read_text(encoding='utf8')
        self.assertIn('ui.button', program)
        self.assertIn('storage.write',program)

if __name__ == '__main__':
    unittest.main()
