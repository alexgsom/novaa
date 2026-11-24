"""Nova Native 0.4 tests. Python is a test harness, never a Nova runtime dependency."""
import http.server
import json
import os
import pathlib
import subprocess
import tempfile
import threading
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / 'bin' / ('nova-native.exe' if os.name == 'nt' else 'nova-native')


class LocalHandler(http.server.BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass

    def do_GET(self):
        if self.path == '/oversize':
            body = b'x' * (2 * 1024 * 1024 + 100)
        elif self.path == '/missing':
            body = b'not found'
        elif self.path == '/redirect':
            self.send_response(302)
            self.send_header('Location', '/json')
            self.end_headers()
            return
        else:
            body = json.dumps({'city': 'Київ', 'number': 42}, ensure_ascii=False).encode('utf8')
        self.send_response(404 if self.path == '/missing' else 200)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        try:
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError):
            pass

    def do_POST(self):
        count = int(self.headers.get('Content-Length', '0'))
        body = self.rfile.read(count)
        self.send_response(201)
        self.end_headers()
        self.wfile.write(body)


class Native04Tests(unittest.TestCase):
    def setUp(self):
        if self._testMethodName.startswith('test_sqlite_') and os.name == 'nt' and not (ROOT / 'bin' / 'sqlite3.dll').exists():
            self.skipTest('SQLite tests on Windows require official sqlite3.dll beside nova-native.exe')

    @classmethod
    def setUpClass(cls):
        if not EXE.exists():
            raise unittest.SkipTest('Build native interpreter first')

    def nova(self, code, files=None, cwd=None):
        with tempfile.TemporaryDirectory(prefix='Nova Żółw ') as d:
            base = Path(d)
            src = base / 'main.nova'
            src.write_text(code, encoding='utf8')
            for name, content in (files or {}).items():
                target = base / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text(content, encoding='utf8')
            result = subprocess.run([str(EXE), 'run', str(src)], cwd=cwd or base,
                                    capture_output=True, text=True, encoding='utf8', timeout=15)
            return result

    def ok(self, source, expected, files=None):
        p = self.nova(source, files)
        self.assertEqual(p.returncode, 0, p.stderr)
        self.assertEqual(p.stdout, expected)

    def test_user_module_functions(self):
        self.ok('import calc\nshow calc.multiply(6,7)\n', '42\n',
                {'calc.nova': 'function multiply(a,b)\n return a*b\nend\n'})

    def test_user_module_globals(self):
        self.ok('import consts\nshow consts.value\n', 'Żółw\n',
                {'consts.nova': 'niech value = "Żółw"\n'})

    def test_user_module_nested(self):
        self.ok('import "lib/phrases.nova" as lib\nshow lib.greet("Оля")\n', 'Hello, Оля\n',
                {'lib/phrases.nova': 'import helper\nfunction greet(name)\n return helper.hello(name)\nend\n',
                 'lib/helper.nova': 'function hello(n)\n return "Hello, " + n\nend\n'})

    def test_user_module_cached(self):
        self.ok('import storage\nimport a\nimport a\nshow storage.read("count.txt")\n', 'x\n',
                {'a.nova': 'import storage\nstorage.append("count.txt","x")\n'})

    def test_import_cyclic_rejected(self):
        p = self.nova('import a\n', {'a.nova': 'import b\n', 'b.nova': 'import a\n'})
        self.assertNotEqual(p.returncode, 0)
        self.assertIn('cyclic module import', p.stderr)

    def test_import_traversal_rejected(self):
        p = self.nova('import "../other.nova" as other\n')
        self.assertNotEqual(p.returncode, 0)
        self.assertIn('module path cannot contain', p.stderr)

    def test_import_absolute_rejected(self):
        p = self.nova('import "/tmp/secret.nova" as secret\n')
        self.assertNotEqual(p.returncode, 0)
        self.assertIn('must be relative', p.stderr)

    def test_import_missing_rejected(self):
        p = self.nova('import absent\n')
        self.assertNotEqual(p.returncode, 0)
        self.assertIn('Cannot open file:', p.stderr)

    def test_import_filename_unicode(self):
        self.ok('import імпортер\nshow імпортер.hello()\n', 'Привіт!\n',
                {'імпортер.nova': 'функція hello()\n поверни "Привіт!"\nкінець\n'})

    def test_import_own_scope(self):
        self.ok('let x=4\nimport calc\nshow calc.test()\nshow x\n', '10\n4\n',
                {'calc.nova': 'let x=10\nfunction test()\n return x\nend\n'})

    def test_import_module_function_private_lookup(self):
        self.ok('import calc\nshow calc.double(5)\n', '10\n',
                {'calc.nova': 'function inner(x)\n return x + x\nend\nfunction double(y)\n return inner(y)\nend\n'})

    def test_sqlite_basic(self):
        self.ok('import sqlite\nlet db=sqlite.open(":memory:")\nsqlite.execute(db,"CREATE TABLE x (v TEXT)")\nshow sqlite.execute(db,"INSERT INTO x(v) VALUES(?)",["Київ"] )\nshow sqlite.query(db,"SELECT v FROM x")[0].v\nsqlite.close(db)\n', '1\nКиїв\n')

    def test_sqlite_types(self):
        self.ok('import sqlite\nlet db=sqlite.open(":memory:")\nsqlite.execute(db,"CREATE TABLE t (a,b,c,d)")\nsqlite.execute(db,"INSERT INTO t VALUES(?,?,?,?)",[true,false,12.5,null])\nlet a=sqlite.query(db,"SELECT * FROM t")[0]\nshow a.a\nshow a.b\nshow a.c\nshow a.d\n', '1\n0\n12.5\nNone\n')

    def test_sqlite_sql_injection_parameters(self):
        self.ok('import sqlite\nlet db=sqlite.open(":memory:")\nsqlite.execute(db,"CREATE TABLE items (name TEXT)")\nsqlite.execute(db,"INSERT INTO items VALUES(?)",["x\' OR 1=1 --"])\nshow sqlite.query(db,"SELECT * FROM items WHERE name=?",["x\' OR 1=1 --"])[0].name\n', "x' OR 1=1 --\n")

    def test_sqlite_bind_count(self):
        p = self.nova('import sqlite\nlet db=sqlite.open(":memory:")\nsqlite.query(db,"SELECT ?",[])\n')
        self.assertNotEqual(p.returncode, 0)
        self.assertIn('parameter count mismatch', p.stderr)

    def test_sqlite_multiple_statements_rejected(self):
        p = self.nova('import sqlite\nlet db=sqlite.open(":memory:")\nsqlite.execute(db,"CREATE TABLE a(x); DROP TABLE a")\n')
        self.assertNotEqual(p.returncode, 0)
        self.assertIn('Only one SQL statement', p.stderr)

    def test_sqlite_invalid_handle(self):
        p = self.nova('import sqlite\nlet d=sqlite.open(":memory:")\nsqlite.close(d)\nsqlite.query(d,"SELECT 1")\n')
        self.assertNotEqual(p.returncode, 0)
        self.assertIn('Invalid or closed SQLite', p.stderr)

    def test_sqlite_parameter_objects_rejected(self):
        p = self.nova('import sqlite\nlet d=sqlite.open(":memory:")\nsqlite.query(d,"SELECT ?",[{}])\n')
        self.assertNotEqual(p.returncode, 0)
        self.assertIn('SQL parameter must be', p.stderr)

    def test_sqlite_persistent_file(self):
        with tempfile.TemporaryDirectory() as d:
            source1 = 'import sqlite\nlet db=sqlite.open("items.db")\nsqlite.execute(db,"CREATE TABLE IF NOT EXISTS t (id INTEGER)")\nsqlite.execute(db,"INSERT INTO t VALUES(?)",[17])\nsqlite.close(db)\n'
            result = self.nova(source1, cwd=d)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = self.nova('import sqlite\nlet db=sqlite.open("items.db")\nshow sqlite.query(db,"SELECT id FROM t")[0].id\n', cwd=d)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stdout, '17\n')

    def test_http_get_json(self):
        with http.server.ThreadingHTTPServer(('127.0.0.1', 0), LocalHandler) as server:
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            try:
                self.ok(f'import http\nimport json\nlet r=http.get("http://127.0.0.1:{server.server_port}/json")\nshow r.status\nshow json.parse(r.body).city\n', '200\nКиїв\n')
            finally:
                server.shutdown()
                thread.join(3)

    def test_http_post(self):
        with http.server.ThreadingHTTPServer(('127.0.0.1', 0), LocalHandler) as server:
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            try:
                self.ok(f'import http\nlet r=http.post("http://127.0.0.1:{server.server_port}/", "Żółw", "text/plain")\nshow r.status\nshow r.body\n', '201\nŻółw\n')
            finally:
                server.shutdown()
                thread.join(3)

    def test_http_status_is_not_exception(self):
        with http.server.ThreadingHTTPServer(('127.0.0.1', 0), LocalHandler) as server:
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            try:
                self.ok(f'import http\nshow http.get("http://127.0.0.1:{server.server_port}/missing").status\n', '404\n')
            finally:
                server.shutdown()
                thread.join(3)

    def test_http_invalid_protocol(self):
        p = self.nova('import http\nhttp.get("file:///etc/passwd")\n')
        self.assertNotEqual(p.returncode, 0)
        self.assertIn('URL must start with http://', p.stderr)

    def test_http_oversize(self):
        with http.server.ThreadingHTTPServer(('127.0.0.1', 0), LocalHandler) as server:
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            try:
                p = self.nova(f'import http\nhttp.get("http://127.0.0.1:{server.server_port}/oversize")\n')
                self.assertNotEqual(p.returncode, 0)
                self.assertIn('response body exceeds 2 MiB', p.stderr)
            finally:
                server.shutdown()
                thread.join(3)

    def test_http_redirect_not_followed(self):
        with http.server.ThreadingHTTPServer(('127.0.0.1', 0), LocalHandler) as server:
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            try:
                self.ok(f'import http\nshow http.get("http://127.0.0.1:{server.server_port}/redirect").status\n', '302\n')
            finally:
                server.shutdown()
                thread.join(3)

    @unittest.skipIf(os.name == 'nt', 'Windows standalone packaging is tested in smoke_windows.ps1')
    def test_standalone_embeds_modules(self):
        with tempfile.TemporaryDirectory(prefix='Nova build Żółw ') as d:
            base = Path(d)
            (base / 'lib').mkdir()
            (base / 'main.nova').write_text('import "lib/helper.nova" as lib\nshow lib.greet("Київ")\n', encoding='utf8')
            (base / 'lib' / 'helper.nova').write_text('function greet(n)\n return "Hi " + n\nend\n', encoding='utf8')
            output = base / 'program'
            proc = subprocess.run(['sh', str(ROOT / 'build_app.sh'), str(base / 'main.nova'), str(output)], capture_output=True, text=True, timeout=90)
            self.assertEqual(proc.returncode, 0, proc.stderr)
            (base / 'main.nova').unlink()
            (base / 'lib' / 'helper.nova').unlink()
            proc = subprocess.run([str(output)], cwd=base, capture_output=True, text=True, encoding='utf8', timeout=8)
            self.assertEqual(proc.returncode, 0, proc.stderr)
            self.assertEqual(proc.stdout, 'Hi Київ\n')


if __name__ == '__main__':
    unittest.main()
