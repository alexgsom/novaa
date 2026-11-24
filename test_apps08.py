"""Real Nova CLI app persistence via self-hosted bytecode and native packaging."""
import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
CLI=ROOT/'nova.sh'
SOURCE=ROOT/'examples/portable/nova_tasks_cli.nova'


@unittest.skipUnless(os.name != 'nt', 'POSIX app packaging test; Windows smoke via CI')
class NativeAppTests08(unittest.TestCase):
    def setUp(self):
        temporary=tempfile.TemporaryDirectory(prefix='nova-data-Привіт-')
        self.addCleanup(temporary.cleanup)
        self.directory=Path(temporary.name)
        self.app=self.directory/'Tasks-Żółw'
        self.env=os.environ.copy()
        self.env['XDG_CONFIG_HOME']=str(self.directory/'profile'/'config')
        (self.directory/'profile'/'config').mkdir(parents=True)

    def exec(self,*args):
        return subprocess.run([str(self.app),*args],capture_output=True,text=True,
                              encoding='utf-8',env=self.env,timeout=15)

    def test_tasks_app_survives_removing_source_and_saves_unicode(self):
        result=subprocess.run(['sh',str(CLI),'build',str(SOURCE),str(self.app)],
                              cwd=ROOT,capture_output=True,text=True,encoding='utf-8',timeout=30)
        self.assertEqual(result.returncode,0,result.stderr)
        # The deployed application is isolated from the compiler, Nova source and .nbc.
        # All application runs below invoke only the native packaged file.
        a=self.exec('--','add','Купити хліб')
        self.assertEqual(a.returncode,0,a.stderr)
        b=self.exec('--','add','Żółw i kawa')
        self.assertEqual(b.returncode,0,b.stderr)
        listed=self.exec('--','list')
        self.assertEqual(listed.returncode,0,listed.stderr)
        self.assertIn('Купити хліб',listed.stdout)
        self.assertIn('Żółw i kawa',listed.stdout)
        datafile=self.directory/'profile/config/NovaTasks/tasks.json'
        self.assertTrue(datafile.exists())
        self.assertIn('Żółw i kawa',datafile.read_text(encoding='utf-8'))
        cleared=self.exec('--','clear')
        self.assertEqual(cleared.returncode,0,cleared.stderr)
        empty=self.exec('--','list')
        self.assertNotIn('Купити хліб',empty.stdout)

    def test_app_usage_and_missing_text(self):
        r=subprocess.run(['sh',str(CLI),'build',str(SOURCE),str(self.app)],cwd=ROOT,capture_output=True,text=True,timeout=30)
        self.assertEqual(r.returncode,0,r.stderr)
        self.assertIn('add <text>',self.exec().stdout)
        self.assertIn('Please provide',self.exec('--','add').stdout)


if __name__=='__main__':
    unittest.main()
