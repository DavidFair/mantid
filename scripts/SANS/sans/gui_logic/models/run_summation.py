import SANSadd2

class RunSummation(object):
    def __call__(self, run_selection, instrument, settings):
        run_selection = self._run_selection_as_path_tuple(run_selection)
        binning = self._bin_settings_or_monitors(settings)
        additional_time_shifts = self._time_shifts_or_none(settings)
        overlay_event_workspaces = self._is_overlay_event_workspaces_enabled(settings)
        save_as_event = self._should_save_as_event_workspaces(settings)

        SANSadd2.add_runs( \
            run_selection, \
            instrument, \
            lowMem=True, \
            binning=binning, \
            isOverlay=overlay_event_workspaces, \
            saveAsEvent=save_as_event, \
            time_shifts=additional_time_shifts)

    def _run_selection_as_path_tuple(self, run_selection):
        return tuple(run.file_path().encode('utf-8') for run in run_selection)

    def _bin_settings_or_monitors(self, settings):
        return settings.bin_settings if settings.has_bin_settings() \
            else 'Monitors'

    def _time_shifts_or_none(self, settings):
        return settings.additional_time_shifts if settings.has_additional_time_shifts() \
            else None

    def _is_overlay_event_workspaces_enabled(self, settings):
        return settings.is_overlay_event_workspaces_enabled() \
            if settings.has_overlay_event_workspaces() \
                else False

    def _should_save_as_event_workspaces(self, settings):
        return settings.should_save_as_event_workspaces()