# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, unicode_literals)

import unittest

from mantidqt.utils.qt.test import GuiTest
from mantidqt.widgets.codeeditor.multifileinterpreter import MultiPythonFileInterpreter


class MultiPythonFileInterpreterTest(GuiTest):

    def test_default_contains_single_editor(self):
        widget = MultiPythonFileInterpreter()
        self.assertEqual(1, widget.editor_count)

    def test_add_editor(self):
        widget = MultiPythonFileInterpreter()
        self.assertEqual(1, widget.editor_count)
        widget.append_new_editor()
        self.assertEqual(2, widget.editor_count)

    def test_editor_widget_not_leaked(self):
        self.fail("Todo after #24700 has been merged, to reuse the helper functions.")


if __name__ == '__main__':
    unittest.main()
