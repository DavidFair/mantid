from __future__ import (absolute_import, division, print_function)

from isis_powder import abstract_inst
from isis_powder.routines import common, InstrumentSettings, yaml_parser
from isis_powder.imat_routines import imat_advanced_config, imat_algs, imat_param_mapping


class Imat(abstract_inst.AbstractInst):
    def __init__(self, **kwargs):
        basic_config_dict = yaml_parser.open_yaml_file_as_dictionary(kwargs.get("config_file", None))

        self._inst_settings = InstrumentSettings.InstrumentSettings(
            param_map=imat_param_mapping.attr_mapping, adv_conf_dict=imat_advanced_config.get_all_adv_variables(),
            kwargs=kwargs, basic_conf_dict=basic_config_dict)

        super(Imat, self).__init__(user_name=self._inst_settings.user_name,
                                   calibration_dir=self._inst_settings.calibration_dir,
                                   output_dir=self._inst_settings.output_dir, inst_prefix="IMAT")

        self._cached_run_details = None
        self._cached_run_number = None

    def focus(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)
        return self._focus(run_number_string=self._inst_settings.run_number,
                           do_van_normalisation=self._inst_settings.do_van_norm)

    def create_vanadium(self, **kwargs):
        self._inst_settings.update_attributes(kwargs=kwargs)

        return self._create_vanadium(run_number_string=self._inst_settings.run_in_range,
                                     do_absorb_corrections=self._inst_settings.do_absorb_corrections)

    def _crop_banks_to_user_tof(self, focused_banks):
        return common.crop_banks_using_crop_list(focused_banks, self._inst_settings.focused_cropping_values)

    def _crop_raw_to_expected_tof_range(self, ws_to_crop):
        raw_cropping_values = self._inst_settings.raw_tof_cropping_values
        return common.crop_in_tof(ws_to_crop, raw_cropping_values[0], raw_cropping_values[1])

    def _crop_van_to_expected_tof_range(self, van_ws_to_crop):
        return common.crop_banks_using_crop_list(van_ws_to_crop, self._inst_settings.vanadium_cropping_values)

    def _get_run_details(self, run_number_string):
        return imat_algs.get_run_details(run_number_string=run_number_string,
                                         inst_settings=self._inst_settings,
                                         is_vanadium_run=self._is_vanadium)

    def _spline_vanadium_ws(self, focused_vanadium_banks):
        return common.spline_vanadium_workspaces(focused_vanadium_spectra=focused_vanadium_banks,
                                                 spline_coefficient=self._inst_settings.spline_coeff)