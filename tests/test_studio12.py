"""Ensure the Win32 editor exposes the update-12 commands in the native UI."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'studio' / 'nova_studio.cpp'


class Studio12Tests(unittest.TestCase):
    def test_toolbar_buttons_share_real_actions(self):
        source=SOURCE.read_text(encoding='utf8')
        # Preview 13 redesigned the toolbar; actions must remain connected to commands.
        for caption, command in [('Save', 'ID_SAVE'), ('Run F5', 'ID_RUN'),
                                 ('Compile', 'ID_CHECK'), ('Build EXE', 'ID_BUILD'),
                                 ('Stop', 'ID_STOP')]:
            self.assertIn(f'newButton(h,L"{caption}"', source.replace('▶  Run F5','Run F5'))
            self.assertIn(command, source)
        self.assertIn('MoveWindow(item.h', source)
        self.assertIn('SMOKE_BEFORE_TOGGLE_COMMENT', source)

    def test_comment_menu_keyboard_and_undoable_edit(self):
        source = SOURCE.read_text(encoding='utf8')
        self.assertIn('L"Toggle comment\\tCtrl+K"', source)
        self.assertIn("'K',ID_TOGGLE_COMMENT", source)
        self.assertIn('case ID_TOGGLE_COMMENT:toggleComment()', source)
        self.assertIn('EM_REPLACESEL,TRUE', source)

if __name__ == '__main__':
    unittest.main()
