"""Opt-in VM trace contract (only runs when Linux native binary is built)."""
from pathlib import Path
import subprocess
import unittest

ROOT=Path(__file__).resolve().parents[1]
EXE=ROOT/'build'/'nova-native'
BYTECODE=ROOT/'examples'/'bootstrap'/'trilingual.nbc'

@unittest.skipUnless(EXE.is_file() and BYTECODE.is_file(), 'native executable not built')
class TraceTests(unittest.TestCase):
    def test_bounded_trace_has_no_literal_values(self):
        p=subprocess.run([str(EXE),'exec',str(BYTECODE),'--trace-steps','3'],
                         capture_output=True,text=True,encoding='utf-8',timeout=10)
        self.assertEqual(p.returncode,0,p.stderr)
        lines=p.stderr.splitlines()
        self.assertEqual(len(lines),3)
        self.assertTrue(all(x.startswith('[nova-trace] step=') for x in lines))
        self.assertNotIn('Оля',p.stderr)
    def test_default_produces_no_trace(self):
        p=subprocess.run([str(EXE),'exec',str(BYTECODE)],capture_output=True,
                         text=True,encoding='utf-8',timeout=10)
        self.assertEqual(p.returncode,0,p.stderr)
        self.assertNotIn('[nova-trace]',p.stderr)
    def test_invalid_trace_limit_rejected(self):
        p=subprocess.run([str(EXE),'exec',str(BYTECODE),'--trace-steps','10001'],
                         capture_output=True,text=True,encoding='utf-8',timeout=10)
        self.assertNotEqual(p.returncode,0)
        self.assertIn('1..10000',p.stderr)

if __name__=='__main__': unittest.main()
