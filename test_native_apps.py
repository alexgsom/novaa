"""Native runtime integration tests. Python only orchestrates the C++ executable."""
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / 'bin' / ('nova-native.exe' if os.name == 'nt' else 'nova-native')

class NativeAppTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not EXE.exists():
            if os.name == 'nt':
                subprocess.run(['powershell','-NoProfile','-File',str(ROOT/'build_windows.ps1')],check=True)
            else:
                subprocess.run(['sh',str(ROOT/'build.sh')],check=True)

    def execute(self, src, directory, filename='main.nova', *args, mode='run', bom=False, crlf=False):
        path=Path(directory)/filename
        contents=src.replace('\n','\r\n') if crlf else src
        path.write_bytes((b'\xef\xbb\xbf' if bom else b'')+contents.encode('utf-8'))
        return subprocess.run([str(EXE),mode,str(path),*args],cwd=directory,
            capture_output=True,text=True,encoding='utf8',timeout=8)

    def test_bom_crlf_unicode_paths(self):
        with tempfile.TemporaryDirectory(prefix='Nova Україна Żółw ') as d:
            p=self.execute('нехай імʼя = "Жовтень Żółw"\nпокажи імʼя\n',d,'тестова програма.nova',bom=True,crlf=True)
            self.assertEqual(p.returncode,0,p.stderr)
            self.assertEqual(p.stdout,'Жовтень Żółw\n')

    def test_storage_utf8_write_read_append(self):
        with tempfile.TemporaryDirectory(prefix='Nova Польща ') as d:
            code='''import storage
show storage.write("дані-żółw.txt", "Привіт")
show storage.append("дані-żółw.txt", "-świat")
show storage.exists("дані-żółw.txt")
show storage.read("дані-żółw.txt")
'''
            p=self.execute(code,d)
            self.assertEqual(p.returncode,0,p.stderr)
            self.assertEqual(p.stdout,'True\nTrue\nTrue\nПривіт-świat\n')
            self.assertEqual((Path(d)/'дані-żółw.txt').read_text(encoding='utf8'),'Привіт-świat')

    def test_storage_mkdir_list_sort(self):
        with tempfile.TemporaryDirectory() as d:
            p=self.execute('''import storage
show storage.mkdir("nova_data")
show storage.write("nova_data/b.txt", "B")
show storage.write("nova_data/a.txt", "A")
show storage.list("nova_data")
show storage.mkdir("nova_data")
''',d)
            self.assertEqual(p.returncode,0,p.stderr)
            self.assertEqual(p.stdout,'True\nTrue\nTrue\n["a.txt", "b.txt"]\nFalse\n')

    def test_storage_catch_missing_file(self):
        with tempfile.TemporaryDirectory() as d:
            p=self.execute('''import storage
try
  show storage.read("missing.txt")
catch error
  show error.message
end
''',d)
            self.assertEqual(p.returncode,0,p.stderr)
            self.assertIn('cannot open file for reading',p.stdout)

    def test_storage_text_validation(self):
        with tempfile.TemporaryDirectory() as d:
            p=self.execute('import storage\nshow storage.write("x.txt", 42)\n',d)
            self.assertNotEqual(p.returncode,0)
            self.assertIn('contents must be text',p.stderr)

    def test_embedded_utf8_bom_source(self):
        # BOM is removed both when loading .nova and when executing embedded bytes.
        if os.name == 'nt':
            self.skipTest('Windows embedded BOM covered by smoke_windows.ps1')
        with tempfile.TemporaryDirectory() as d:
            source=Path(d)/'bom.nova'; exe=Path(d)/'bom'
            source.write_bytes(b'\xef\xbb\xbf'+'покажи "БОМ успішно"\n'.encode('utf8'))
            build=subprocess.run(['sh',str(ROOT/'build_app.sh'),str(source),str(exe)],
                                 capture_output=True,text=True,timeout=45)
            self.assertEqual(build.returncode,0,build.stderr)
            source.unlink()
            p=subprocess.run([str(exe)],capture_output=True,text=True,encoding='utf8',timeout=8)
            self.assertEqual(p.returncode,0,p.stderr)
            self.assertEqual(p.stdout,'БОМ успішно\n')

    def test_system_platform_and_path_join(self):
        with tempfile.TemporaryDirectory() as d:
            p=self.execute('import system\nshow system.platform()\nshow system.join("a", "b", "c.txt")\n',d)
            self.assertEqual(p.returncode,0,p.stderr)
            self.assertEqual(p.stdout,('windows' if os.name=='nt' else 'linux')+'\n'+str(Path('a')/'b'/'c.txt')+'\n')

    def test_module_not_imported(self):
        with tempfile.TemporaryDirectory() as d:
            p=self.execute('show storage.exists("test.txt")\n',d)
            self.assertNotEqual(p.returncode,0)
            self.assertIn('storage',p.stderr)

    def test_windows_ui_blocks_parse(self):
        with tempfile.TemporaryDirectory() as d:
            for name in ['windows_hello_app.nova','windows_notes_app.nova']:
                src=(ROOT/'examples'/name).read_text(encoding='utf8')
                p=self.execute(src,d,name,mode='check')
                self.assertEqual(p.returncode,0,p.stderr)

    @unittest.skipIf(os.name=='nt','Interactive Win32 GUI is manually tested; cannot automate click on CI')
    def test_gui_unavailable_on_linux(self):
        with tempfile.TemporaryDirectory() as d:
            p=self.execute('import ui\nshow ui.window("test")\n',d)
            self.assertNotEqual(p.returncode,0)
            self.assertIn('Windows only',p.stderr)

    @unittest.skipIf(os.name=='nt','Windows standalone building checked in smoke_windows.ps1')
    def test_embedded_app_storage_no_source(self):
        with tempfile.TemporaryDirectory(prefix='Нотатки Nova ') as d:
            source=Path(d)/'notes.nova';exe=Path(d)/'notes'
            source.write_text('import storage\nshow storage.write("a.txt", "Nova")\nshow storage.read("a.txt")\n',encoding='utf8')
            p=subprocess.run(['sh',str(ROOT/'build_app.sh'),str(source),str(exe)],capture_output=True,text=True,timeout=45)
            self.assertEqual(p.returncode,0,p.stderr)
            source.unlink()
            result=subprocess.run([str(exe)],cwd=d,capture_output=True,text=True,timeout=8)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertEqual(result.stdout,'True\nNova\n')

if __name__=='__main__':unittest.main()
