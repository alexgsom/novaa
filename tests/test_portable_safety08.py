"""Portable packaging malformed-input and Windows-PE recognition tests."""
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
RUNNER=ROOT/'bin'/('nova-native.exe' if os.name=='nt' else 'nova-native')


class PackageSafetyTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not RUNNER.exists():
            raise unittest.SkipTest('Native runner not compiled')

    def setUp(self):
        tmp=tempfile.TemporaryDirectory(prefix='nova-pe-Ångström-')
        self.addCleanup(tmp.cleanup)
        self.folder=Path(tmp.name)
        self.bytecode=self.folder/'input.nbc'
        self.output=self.folder/('hello.exe' if os.name=='nt' else 'hello')

    def pack(self,*extra):
        return subprocess.run([str(RUNNER),'pack',str(self.bytecode),str(self.output),*map(str,extra)],capture_output=True,text=True,timeout=20)

    def test_reject_unknown_opcode(self):
        self.bytecode.write_text('{"format":"nova-bytecode","version":1,"instructions":[["dangerous_opcode"],["halt"]]}')
        r=self.pack()
        self.assertNotEqual(r.returncode,0)
        self.assertIn('Unknown opcode',r.stderr)
        self.assertFalse(self.output.exists())

    def test_reject_incorrect_operand_count(self):
        self.bytecode.write_text('{"format":"nova-bytecode","version":1,"instructions":[["print",123],["halt"]]}')
        r=self.pack()
        self.assertNotEqual(r.returncode,0)
        self.assertIn('argument count',r.stderr)
        self.assertFalse(self.output.exists())

    def test_reject_fake_windows_pe(self):
        """MZ + COFF PE marker + Nova marker does not equal a runnable Win64 EXE."""
        fake=self.folder/'fake.exe'
        bytes_=bytearray(512)
        bytes_[:2]=b'MZ'
        bytes_[0x3c:0x40]=(0x80).to_bytes(4,'little')
        bytes_[0x80:0x84]=b'PE\x00\x00'
        bytes_[0x84:0x86]=(0x8664).to_bytes(2,'little')
        bytes_[300:316]=b'NOVA_BYTECODE_08'
        fake.write_bytes(bytes_)
        self.bytecode.write_text('{"format":"nova-bytecode","version":1,"instructions":[["halt"]]}')
        r=self.pack('--runtime',fake)
        self.assertNotEqual(r.returncode,0)
        self.assertIn('PE',r.stderr)
        self.assertFalse(self.output.exists())


if __name__=='__main__':
    unittest.main()
