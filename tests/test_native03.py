"""Nova Native 0.3 cross-platform regression tests. Python is test harness ONLY."""
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / 'bin' / ('nova-native.exe' if os.name == 'nt' else 'nova-native')


class Native03Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not EXE.exists():
            script = ROOT / ('build_windows.ps1' if os.name == 'nt' else 'build.sh')
            cmd = (['powershell', '-NoProfile', '-File', str(script)] if os.name == 'nt'
                   else ['sh', str(script)])
            subprocess.run(cmd, cwd=ROOT, check=True, timeout=80)

    def nova(self, source, *, env=None, mode='run', cwd=None):
        with tempfile.TemporaryDirectory() as tmp:
            file = Path(tmp) / 'sample.nova'
            file.write_text(source, encoding='utf8')
            return subprocess.run([str(EXE), mode, str(file)],
                                  cwd=cwd or tmp, capture_output=True,
                                  text=True, encoding='utf8', timeout=8, env=env)

    def ok(self, source, expected):
        proc = self.nova(source)
        self.assertEqual(proc.returncode, 0, proc.stderr)
        self.assertEqual(proc.stdout, expected)

    def test_json_parse_trilingual(self):
        self.ok('import json\nlet d=json.parse("{\\"місто\\":\\"Київ\\",\\"n\\":3}")\nshow d["місто"]\nshow d.n\n',
                'Київ\n3\n')

    def test_json_parse_arrays_null_bool_nested(self):
        self.ok('import json\nlet d=json.parse("[null,true,false,{\\"x\\":[2,3]}]")\nshow d[0]\nshow d[1]\nshow d[2]\nshow d[3].x[1]\n',
                'None\nTrue\nFalse\n3\n')

    def test_json_unicode_surrogate_pair(self):
        data = r'"\uD83D\uDE80"'  # JSON text includes two Unicode escapes
        source = 'import json\nshow json.parse(' + json.dumps(data, ensure_ascii=False) + ')\n'
        self.ok(source, '🚀\n')

    def test_json_stringify_unicode(self):
        self.ok('import json\nshow json.stringify({"а":"Їжак", "z":[1,true,null]})\n',
                '{"z":[1,true,null],"а":"Їжак"}\n')

    def test_json_jsonstring_escape(self):
        self.ok('import json\nshow json.stringify("a\\nb\\t\\\\c")\n',
                '"a\\nb\\t\\\\c"\n')

    def test_json_pretty_round_trip(self):
        self.ok('import json\nlet x={"a":[1,{"b":"text"}]}\nlet y=json.parse(json.pretty(x))\nshow y.a[1].b\n',
                'text\n')

    def test_json_trailing_garbage_caught(self):
        self.ok('import json\ntry\n json.parse("true false")\ncatch e\n show e.message\nend\n',
                'Runtime error (line 3): json: JSON at byte 6: trailing data\n')

    def test_json_invalid_escape_caught(self):
        source='import json\ntry\n show json.parse("\\"\\\\q\\"")\ncatch e\n show e.message\nend\n'
        p=self.nova(source)
        self.assertEqual(p.returncode,0,p.stderr)
        self.assertIn('invalid escape',p.stdout)

    def test_json_invalid_surrogate_caught(self):
        source = 'import json\ntry\n json.parse(' + json.dumps('"\\uD800"') + ')\ncatch e\n show e.message\nend\n'
        p=self.nova(source)
        self.assertEqual(p.returncode,0,p.stderr)
        self.assertIn('low surrogate',p.stdout)

    def test_json_invalid_number_caught(self):
        p=self.nova('import json\ntry\n json.parse("[01]")\ncatch e\n show e.message\nend\n')
        self.assertEqual(p.returncode,0,p.stderr)
        self.assertIn('JSON',p.stdout)

    def test_json_duplicate_key_last_value(self):
        self.ok('import json\nshow json.parse("{\\"a\\":1,\\"a\\":2}").a\n', '2\n')

    def test_json_cycle_protected(self):
        p=self.nova('import json\nimport lists\nlet a=[]\nlists.append(a,a)\ntry\n json.stringify(a)\ncatch e\n show e.message\nend\n')
        self.assertEqual(p.returncode,0,p.stderr)
        self.assertIn('cyclic list',p.stdout)

    def test_cyclic_lists_show_and_compare_without_crash(self):
        self.ok('import lists\nlet a=[]\nlists.append(a,a)\nshow a\nshow a==a\n',
                '[[...]]\nTrue\n')

    def test_json_depth_limit(self):
        with tempfile.TemporaryDirectory() as d:
            src='import json\ntry\n json.parse(storage.read("too_deep.txt"))\ncatch e\n show e.message\nend\n'
            Path(d,'too_deep.txt').write_text('['*66+'0'+']'*66,encoding='utf8')
            p=self.nova('import storage\n'+src,cwd=d)
            self.assertEqual(p.returncode,0,p.stderr)
            self.assertIn('nesting limit',p.stdout)

    def test_json_missing_import(self):
        p=self.nova('show json.parse("true")\n')
        self.assertNotEqual(p.returncode,0)
        self.assertIn('json',p.stderr)

    def test_text_split_empty_fields(self):
        self.ok('import text\nshow text.split(",x,", ",")\n', '["", "x", ""]\n')

    def test_text_join_unicode(self):
        self.ok('import text\nshow text.join("—",["Їжак", "Żółw", "Ola"])\n', 'Їжак—Żółw—Ola\n')

    def test_text_contains_starts_ends(self):
        self.ok('import text\nshow text.starts_with("Привіт", "При")\nshow text.ends_with("Żółw", "ółw")\nshow text.contains("abc", "bc")\n',
                'True\nTrue\nTrue\n')

    def test_text_replace_trim(self):
        self.ok('import text\nshow text.replace("a-b-a", "a", "x")\nshow text.trim("  Ola  ")\n', 'x-b-x\nOla\n')

    def test_text_reject_empty_separator(self):
        p=self.nova('import text\nshow text.split("abc", "")\n')
        self.assertNotEqual(p.returncode,0)
        self.assertIn('separator cannot be empty',p.stderr)

    def test_text_join_type_validation(self):
        p=self.nova('import text\nshow text.join(",",[1,2])\n')
        self.assertNotEqual(p.returncode,0)
        self.assertIn('list of text',p.stderr)

    def test_lists_append_sort_pop_reverse(self):
        self.ok('import lists\nlet a=[3,1]\nshow lists.append(a,2)\nshow lists.sort(a)\nshow lists.pop(a)\nshow lists.reverse(a)\n',
                '3\n[1, 2, 3]\n3\n[2, 1]\n')

    def test_lists_invalid_mixed_type(self):
        p=self.nova('import lists\nshow lists.sort([1,"2"])\n')
        self.assertNotEqual(p.returncode,0)
        self.assertIn('one type',p.stderr)

    def test_storage_rename_copy_size_remove(self):
        with tempfile.TemporaryDirectory(prefix='Nova Żółw ') as d:
            code='''import storage
show storage.write("żółw.txt","Привіт")
show storage.copy("żółw.txt","копія.txt")
show storage.size("копія.txt")
show storage.rename("копія.txt","новий.txt")
show storage.read("новий.txt")
show storage.remove("żółw.txt")
show storage.exists("żółw.txt")
'''
            p=self.nova(code,cwd=d)
            self.assertEqual(p.returncode,0,p.stderr)
            self.assertEqual(p.stdout,'True\nTrue\n12\nTrue\nПривіт\nTrue\nFalse\n')

    def test_storage_prevent_clobber(self):
        with tempfile.TemporaryDirectory() as d:
            p=self.nova('import storage\nstorage.write("a", "A")\nstorage.write("b", "B")\ntry\n storage.copy("a","b")\ncatch e\n show e.message\nend\nshow storage.read("b")\n',cwd=d)
            self.assertEqual(p.returncode,0,p.stderr)
            self.assertIn('destination already exists',p.stdout)
            self.assertTrue(p.stdout.endswith('B\n'))

    def test_storage_remove_refuses_directory(self):
        with tempfile.TemporaryDirectory() as d:
            p=self.nova('import storage\nstorage.mkdir("sampledir")\ntry\n storage.remove("sampledir")\ncatch e\n show e.message\nend\n',cwd=d)
            self.assertEqual(p.returncode,0,p.stderr)
            self.assertIn('not directories',p.stdout)

    @unittest.skipIf(os.name=='nt', 'APPDATA value depends on Windows user account')
    def test_system_appdata_xdg(self):
        env={**os.environ,'XDG_CONFIG_HOME':'/tmp/nova-custom'}
        p=self.nova('import system\nshow system.appdata()\n',env=env)
        self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(p.stdout,'/tmp/nova-custom\n')

    def test_system_temp(self):
        p=self.nova('import system\nshow len(system.temp()) > 0\n')
        self.assertEqual(p.returncode,0,p.stderr)
        self.assertEqual(p.stdout,'True\n')

    def test_examples_parsed(self):
        for fname in ['windows_nova_notes_pro.nova','windows_tasks_json.nova','windows_ui_smoke.nova','native_json_demo.nova']:
            with self.subTest(fname=fname):
                p=self.nova((ROOT/'examples'/fname).read_text(encoding='utf8'),mode='check')
                self.assertEqual(p.returncode,0,p.stderr)

    def test_json_demo_runs(self):
        p=subprocess.run([str(EXE),'run',str(ROOT/'examples/native_json_demo.nova')],capture_output=True,
                         text=True,encoding='utf8',timeout=8)
        self.assertEqual(p.returncode,0,p.stderr)
        self.assertIn('Київ\nОля, Jan, Żaneta\n',p.stdout)

    def test_json_file_embedded_standalone(self):
        if os.name=='nt':self.skipTest('Embedded Windows EXE covered in smoke_windows.ps1')
        with tempfile.TemporaryDirectory() as d:
            source=Path(d,'json_app.nova');exe=Path(d,'json_app')
            source.write_text('import json\nimport storage\nlet x={"мова":"Nova","version":0.3}\nstorage.write("state.json",json.stringify(x))\nshow json.parse(storage.read("state.json")).мова\n',encoding='utf8')
            p=subprocess.run(['sh',str(ROOT/'build_app.sh'),str(source),str(exe)],capture_output=True,text=True,timeout=90)
            self.assertEqual(p.returncode,0,p.stderr)
            source.unlink()
            p=subprocess.run([str(exe)],cwd=d,capture_output=True,text=True,encoding='utf8',timeout=8)
            self.assertEqual(p.returncode,0,p.stderr)
            self.assertEqual(p.stdout,'Nova\n')
            self.assertEqual(json.loads(Path(d,'state.json').read_text(encoding='utf8')),{'мова':'Nova','version':0.3})

if __name__=='__main__':unittest.main()
