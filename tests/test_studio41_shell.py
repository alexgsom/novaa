"""Structural regression tests for Windows-only Nova Studio shell.
These check source integration; the actual GUI smoke runs on Windows CI.
"""
import pathlib
import unittest

SOURCE = (pathlib.Path(__file__).parent.parent / 'studio' / 'nova_studio.cpp').read_text(encoding='utf-8')

class VisualShell42SourceTests(unittest.TestCase):
    def test_native_menu_bar_is_replaced(self):
        self.assertIn('HMENU menu=CreatePopupMenu()', SOURCE)
        self.assertIn('TrackPopupMenu(appMenu,', SOURCE)
        self.assertIn('GetMenu(mainWindow)!=nullptr', SOURCE)
        self.assertNotIn('SetMenu(h,menu)', SOURCE)

    def test_new_activity_rail_has_real_commands(self):
        for symbol in ('ID_NAV_FILES', 'ID_NAV_SEARCH', 'ID_NAV_AI',
                       'ID_NAV_FOCUS', 'ID_NAV_OUTPUT', 'ID_SHELL_MENU'):
            self.assertIn('case ' + symbol + ':', SOURCE)
        self.assertIn('MoveWindow(rails[i],8,145+i*52,42,42,TRUE)', SOURCE)
        self.assertIn('railGlyph(', SOURCE)

    def test_console_toggle_is_functional(self):
        self.assertIn('showOutputPanel=!showOutputPanel', SOURCE)
        self.assertIn("'J',ID_VIEW_OUTPUT", SOURCE)
        self.assertIn('ShowWindow(control,outputHeight?SW_SHOW:SW_HIDE)', SOURCE)
        self.assertIn('SMOKE_VISUAL_SHELL', SOURCE)

    def test_aura_paints_real_dark_tabs(self):
        self.assertIn('SetWindowSubclass(tabs,tabsSubclass', SOURCE)
        self.assertIn('void paintTabs(HWND hwnd)', SOURCE)
        self.assertIn('CreateCompatibleBitmap(dc,width,height)', SOURCE)
        self.assertIn('!GetWindowSubclass(tabs,tabsSubclass', SOURCE)

    def test_orphan_button_is_hidden(self):
        self.assertIn('ShowWindow(btnOpen,SW_HIDE)', SOURCE)
        self.assertIn('IsWindowVisible(btnOpen)', SOURCE)

    def test_shell_has_distinct_surfaces(self):
        self.assertIn('fill(0,0,w,58,BAR_BG)', SOURCE)
        self.assertIn('fill(0,58,w,105,SUB_BAR)', SOURCE)
        self.assertIn('fill(0,105,58,height-28,RAIL_BG)', SOURCE)
        self.assertIn('WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN', SOURCE)

if __name__ == '__main__':
    unittest.main()
