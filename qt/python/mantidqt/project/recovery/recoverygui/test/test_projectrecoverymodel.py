# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#

import unittest
import mock
import os
import shutil
import time

from mantidqt.project.recovery.recoverygui.projectrecoverymodel import ProjectRecoveryModel
from mantidqt.project.recovery.projectrecovery import ProjectRecovery
from mantid.simpleapi import CreateSampleWorkspace
from mantid.api import AnalysisDataService as ADS
from mantid.kernel import ConfigService


class ProjectRecoveryModelTest(unittest.TestCase):
    def setUp(self):
        self.pr = ProjectRecovery(mock.MagicMock(), mock.MagicMock(), mock.MagicMock())

        # Set up some checkpoints
        self.setup_some_checkpoints()

        self.prm = ProjectRecoveryModel(self.pr, mock.MagicMock())

    def tearDown(self):
        # Make sure to clear the hostname layer between tests
        ADS.clear()

        if os.path.exists(self.pid):
            shutil.rmtree(self.pid)

    def setup_some_checkpoints(self):
        self.pr._spin_off_another_time_thread = mock.MagicMock()
        directory = self.pr.recovery_directory_hostname

        # Add a numbered folder for the pid
        self.pid = os.path.join(directory, "3000000")
        if not os.path.exists(self.pid):
            os.makedirs(self.pid)
        self.pr.recovery_directory_pid = self.pid

        # Add 5 workspaces
        for ii in range(0, 5):
            CreateSampleWorkspace(OutputWorkspace=str(ii))

        self.pr.recovery_save()

    def test_find_number_of_workspaces_in_directory(self):
        # Expect 0 as just checkpoints
        self.assertEqual(self.prm.find_number_of_workspaces_in_directory(self.pid), 0)

        # Expect 1 as only one checkpoint needed
        self.assertTrue(os.path.exists(self.pid))
        list_dir = os.listdir(self.pid)
        list_dir.sort()
        self.assertEqual(self.prm.find_number_of_workspaces_in_directory(os.path.join(self.pid, list_dir[0])), 5)

    def test_get_row_as_string(self):
        row = self.prm.rows[0]
        self.assertEqual(self.prm.get_row(row[0]), row)

    def test_get_row_as_int(self):
        row = self.prm.rows[0]
        self.assertEqual(self.prm.get_row(0), row)

    def test_get_row_as_string_not_found(self):
        row = ["", "", ""]
        self.assertEqual(self.prm.get_row("asdadasdasd"), row)

    def test_start_mantid_normally(self):
        self.prm.start_mantid_normally()
        self.assertEqual(self.prm.presenter.close_view.call_count, 1)

    def test_recover_selected_checkpoint(self):
        checkpoint = os.listdir(self.pid)[0]
        self.prm._create_thread_and_manage = mock.MagicMock()
        self.prm.recover_selected_checkpoint(checkpoint)

        self.prm.presenter.change_start_mantid_to_cancel_label.assert_called_once()
        self.prm._create_thread_and_manage.assert_called_once()

    def test_open_selected_in_editor(self):
        checkpoint = os.listdir(self.pid)[0]
        self.prm.project_recovery.open_checkpoint_in_script_editor = mock.MagicMock()
        self.prm.open_selected_in_editor(checkpoint)

        self.prm.project_recovery.open_checkpoint_in_script_editor.assert_called_once()
        self.assertEqual(self.prm.project_recovery.open_checkpoint_in_script_editor.call_args,
                         mock.call(os.path.join(self.pid, checkpoint)))

    def test_decide_last_checkpoint(self):
        CreateSampleWorkspace(OutputWorkspace="6")
        self.pr.recovery_save()

        checkpoints = os.listdir(self.pid)
        checkpoints.sort()

        last_checkpoint = self.prm.decide_last_checkpoint()
        self.assertEqual(checkpoints[-1], os.path.basename(last_checkpoint))

    def test_fill_rows(self):
        # Wait a second so we can have two rows with different names
        time.sleep(1)

        CreateSampleWorkspace(OutputWorkspace="6")
        self.pr.recovery_save()

        self.prm.fill_rows()

        checkpoints = os.listdir(self.pid)
        checkpoints.sort()

        self.assertEqual(["", "", ""], self.prm.rows[2])
        self.assertEqual([checkpoints[0], "5", "No"], self.prm.rows[1])
        self.assertEqual([checkpoints[1], "6", "No"], self.prm.rows[0])

    def test_get_number_of_checkpoints(self):
        self.assertEqual(int(ConfigService.getString("projectRecovery.numberOfCheckpoints")),
                         self.prm.get_number_of_checkpoints())

    def test_update_checkpoint_tried(self):
        checkpoints = os.listdir(self.pid)

        self.assertEqual(self.prm.rows[0][2], "No")
        self.prm._update_checkpoint_tried(os.path.join(self.pid, checkpoints[0]))
        self.assertEqual(self.prm.rows[0][2], "Yes")
