# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

"""State describing the creation of pixel and wavelength adjustment workspaces for SANS reduction."""

from __future__ import (absolute_import, division, print_function)
import json
import copy

from six import with_metaclass

from sans.state.JsonSerializable import JsonSerializable
from sans.state.automatic_setters import automatic_setters
from sans.state.state_functions import (is_not_none_and_first_larger_than_second, one_is_none, validation_message)
from sans.common.enums import (RangeStepType, DetectorType, SANSFacility)


class StateAdjustmentFiles(with_metaclass(JsonSerializable)):
    def __init__(self):
        super(StateAdjustmentFiles, self).__init__()
        self.pixel_adjustment_file = None  # : Str()
        self.wavelength_adjustment_file = None  # : Str()

    def validate(self):
        is_invalid = {}
        # TODO: It would be nice to have a typed parameter for files which checks if a file input exists or not.
        #       This is very low priority, but would be nice to have.

        if is_invalid:
            raise ValueError("StateAdjustmentFiles: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


class StateWavelengthAndPixelAdjustment(with_metaclass(JsonSerializable)):
    def __init__(self):
        super(StateWavelengthAndPixelAdjustment, self).__init__()
        self.wavelength_low = None  # : List[Float] (Positive)
        self.wavelength_high = None  # : List[Float] (Positive)
        self.wavelength_step = None  # : Float (Positive)
        self.wavelength_step_type = RangeStepType.NOT_SET

        self.idf_path = None  # : Str()

        self.adjustment_files = {DetectorType.LAB.value: StateAdjustmentFiles(),
                                 DetectorType.HAB.value: StateAdjustmentFiles()}

    def validate(self):
        is_invalid = {}

        if one_is_none([self.wavelength_low, self.wavelength_high, self.wavelength_step, self.wavelength_step_type]):
            entry = validation_message("A wavelength entry has not been set.",
                                       "Make sure that all entries are set.",
                                       {"wavelength_low": self.wavelength_low,
                                        "wavelength_high": self.wavelength_high,
                                        "wavelength_step": self.wavelength_step,
                                        "wavelength_step_type": self.wavelength_step_type})
            is_invalid.update(entry)

        if self.wavelength_step_type is RangeStepType.NOT_SET:
            entry = validation_message("A wavelength entry has not been set.",
                                       "Make sure that all entries are set.",
                                       {"wavelength_step_type": self.wavelength_step_type})
            is_invalid.update(entry)

        if is_not_none_and_first_larger_than_second([self.wavelength_low, self.wavelength_high]):
            entry = validation_message("Incorrect wavelength bounds.",
                                       "Make sure that lower wavelength bound is smaller then upper bound.",
                                       {"wavelength_low": self.wavelength_low,
                                        "wavelength_high": self.wavelength_high})
            is_invalid.update(entry)

        try:
            self.adjustment_files[DetectorType.LAB.value].validate()
            self.adjustment_files[DetectorType.HAB.value].validate()
        except ValueError as e:
            is_invalid.update({"adjustment_files": str(e)})

        if is_invalid:
            raise ValueError("StateWavelengthAndPixelAdjustment: The provided inputs are illegal. "
                             "Please see: {0}".format(json.dumps(is_invalid)))


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateWavelengthAndPixelAdjustmentBuilder(object):
    @automatic_setters(StateWavelengthAndPixelAdjustment, exclusions=["idf_path"])
    def __init__(self, data_info):
        super(StateWavelengthAndPixelAdjustmentBuilder, self).__init__()
        idf_file_path = data_info.idf_file_path
        self.state = StateWavelengthAndPixelAdjustment()
        self.state.idf_path = idf_file_path

    def build(self):
        self.state.validate()
        return copy.copy(self.state)

    def set_wavelength_step_type(self, val):
        self.state.wavelength_step_type = val


def get_wavelength_and_pixel_adjustment_builder(data_info):
    facility = data_info.facility
    if facility is SANSFacility.ISIS:
        return StateWavelengthAndPixelAdjustmentBuilder(data_info)
    else:
        raise NotImplementedError("StateWavelengthAndPixelAdjustmentBuilder: Could not find any valid "
                                  "wavelength and pixel adjustment builder for the specified "
                                  "StateData object {0}".format(str(data_info)))