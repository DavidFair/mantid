from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, WorkspaceUnitValidator

from mantid.kernel import Direction, FloatArrayProperty, FloatArrayBoundedValidator

import numpy as np

class DetectorFloodWeighting(DataProcessorAlgorithm):

    def __init__(self):
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        return 'Workflow\\SANS'


    def summary(self):
        return 'Generates a Detector flood weighting, or sensitivity workspace'


    def PyInit(self):

        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace', '',
                                                     direction=Direction.Input, validator=WorkspaceUnitValidator("Wavelength")),
                             doc='Flood weighting measurement')

        validator = FloatArrayBoundedValidator()
        validator.setLower(0.)
        self.declareProperty(FloatArrayProperty('Bands', [], direction=Direction.Input, validator=validator),
                             doc='Wavelength bands to use. Single pair min to max.')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                                                     direction=Direction.Output),
                             doc='Normalized flood weighting measurement')


    def validateInputs(self):
        """
        Validates input ranges.
        """
        issues = dict()

        bands = self.getProperty('Bands').value

        if not any(bands):
            issues['Bands'] = 'Bands must be supplied'


        if not len(bands)%2 == 0:
            issues['Bands'] = 'Even number of Bands boundaries expected'

        if len(bands) > 2:
            issues['Bands'] = 'Presently this algorithm only supports one pair of bands'

        all_limits=list()
        for i in range(0, len(bands), 2):
            lower = bands[i]
            upper = bands[i+1]
            limits = np.arange(lower, upper)
            unique = set(limits)
            for existing_lims in all_limits:
                if unique.intersection(set(existing_lims)):
                    issues['Bands'] = 'Bands must not intersect'
                    break

            all_limits.append(limits)
            if lower >= upper:
                issues['Bands'] = 'Bands should form lower, upper pairs'

        return issues


    def PyExec(self):

        in_ws = self.getProperty('InputWorkspace').value
        bands = self.getProperty('Bands').value

        # Formulate bands
        params = list()
        for i in range(0, len(bands), 2):
            lower = bands[i]
            upper = bands[i+1]
            step = upper - lower
            params.append((lower, step, upper))

        accumulated_output = None
        rebin = self.createChildAlgorithm("Rebin")
        rebin.setProperty("Params", params[0])
        rebin.setProperty("InputWorkspace", in_ws)
        rebin.execute()
        accumulated_output = rebin.getProperty("OutputWorkspace").value

        # Determine the max across all spectra
        y_values = accumulated_output.extractY()
        max_val = np.amax(y_values)

        # Create a workspace from the single max value
        create = self.createChildAlgorithm("CreateSingleValuedWorkspace")
        create.setProperty("DataValue", max_val)
        create.execute()
        max_ws = create.getProperty("OutputWorkspace").value

        # Divide each entry by max
        divide = self.createChildAlgorithm("Divide")
        divide.setProperty("LHSWorkspace", accumulated_output)
        divide.setProperty("RHSWorkspace", max_ws)
        divide.execute()
        normalized = divide.getProperty("OutputWorkspace").value

        self.setProperty('OutputWorkspace', normalized)


# Register alg
AlgorithmFactory.subscribe(DetectorFloodWeighting)
