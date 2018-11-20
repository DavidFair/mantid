# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import systemtesting
from mantid.simpleapi import *


class ILL_D11_Test(systemtesting.MantidSystemTest):

    def __init__(self):
        super(ILL_D11_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D11'
        config.appendDataSearchSubDir('ILL/D11/')

    def tearDown(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-5
        return ['iq', 'ILL_SANS_D11_IQ.nxs']

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename='D11_mask.nxs', OutputWorkspace='mask')
        # Process the dark current Cd/B4C for water
        SANSILLReduction(Run='010455.nxs', ProcessAs='Absorber', OutputWorkspace='Cdw')
        # Process the empty beam for water
        SANSILLReduction(Run='010414.nxs', ProcessAs='Beam', AbsorberInputWorkspace='Cdw', OutputWorkspace='Dbw')
        # Water container transmission
        SANSILLReduction(Run='010446.nxs', ProcessAs='Transmission', AbsorberInputWorkspace='Cdw',
                         BeamInputWorkspace='Dbw', OutputWorkspace='wc_tr')
        # Water container
        SANSILLReduction(Run='010454.nxs', ProcessAs='Container', AbsorberInputWorkspace='Cdw',
                         BeamInputWorkspace='Dbw', TransmissionInputWorkspace='wc_tr', OutputWorkspace='wc')
        # Water transmission
        SANSILLReduction(Run='010445.nxs', ProcessAs='Transmission', AbsorberInputWorkspace='Cdw',
                         BeamInputWorkspace='Dbw', OutputWorkspace='w_tr')
        # Water
        SANSILLReduction(Run='010453.nxs', ProcessAs='Reference', AbsorberInputWorkspace='Cdw', MaskedInputWorkspace='mask',
                         ContainerInputWorkspace='wc', BeamInputWorkspace='Dbw', TransmissionInputWorkspace='wc_tr',
                         SensitivityOutputWorkspace='sens', OutputWorkspace='water')
        # Process the dark current Cd/B4C for sample
        SANSILLReduction(Run='010462.nxs', ProcessAs='Absorber', OutputWorkspace='Cd')
        # Process the empty beam for sample
        SANSILLReduction(Run='010413.nxs', ProcessAs='Beam', AbsorberInputWorkspace='Cd', OutputWorkspace='Db')
        # Sample container transmission
        SANSILLReduction(Run='010444.nxs', ProcessAs='Transmission', AbsorberInputWorkspace='Cd',
                         BeamInputWorkspace='Dbw', OutputWorkspace='sc_tr')
        # Sample container
        SANSILLReduction(Run='010460.nxs', ProcessAs='Container', AbsorberInputWorkspace='Cd', BeamInputWorkspace='Db',
                         TransmissionInputWorkspace='sc_tr', OutputWorkspace='sc')
        # Sample transmission
        SANSILLReduction(Run='010585.nxs', ProcessAs='Transmission', AbsorberInputWorkspace='Cd', BeamInputWorkspace='Dbw',
                         OutputWorkspace='s_tr')
        # Sample
        SANSILLReduction(Run='010569.nxs', ProcessAs='Sample', AbsorberInputWorkspace='Cd', ContainerInputWorkspace='sc',
                         BeamInputWorkspace='Db', SensitivityInputWorkspace='sens', MaskedInputWorkspace='mask',
                         TransmissionInputWorkspace='s_tr', OutputWorkspace='sample_flux')
        # Convert to I(Q)
        SANSILLIntegration(InputWorkspace='sample_flux', OutputWorkspace='iq')


class ILL_D22_Test(systemtesting.MantidSystemTest):

    def __init__(self):
        super(ILL_D22_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D22'
        config.appendDataSearchSubDir('ILL/D22/')

    def tearDown(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-5
        return ['iq', 'ILL_SANS_D22_IQ.nxs']

    def runTest(self):
        # Load the mask
        LoadNexusProcessed(Filename='D22_mask.nxs', OutputWorkspace='mask')

        # Absorber
        SANSILLReduction(Run='241238', ProcessAs='Absorber', OutputWorkspace='Cd')

        # Beam
        SANSILLReduction(Run='241226', ProcessAs='Beam', AbsorberInputWorkspace='Cd', OutputWorkspace='Db')

        # Container transmission known
        CreateSingleValuedWorkspace(DataValue=0.94638, ErrorValue=0.0010425, OutputWorkspace='ctr')
        AddSampleLog(Workspace='ctr', LogName='ProcessedAs', LogText='Transmission')

        # Container
        SANSILLReduction(Run='241239', ProcessAs='Container', AbsorberInputWorkspace='Cd', BeamInputWorkspace='Db', TransmissionInputWorkspace='ctr', OutputWorkspace='can')

        # Sample transmission known
        CreateSingleValuedWorkspace(DataValue=0.52163, ErrorValue=0.00090538, OutputWorkspace='str')
        AddSampleLog(Workspace='str', LogName='ProcessedAs', LogText='Transmission')

        # Reference
        # Actually this is not water, but a deuterated buffer, but is fine for the test
        SANSILLReduction(Run='241261', ProcessAs='Reference', MaskedInputWorkspace='mask',
                         AbsorberInputWorkspace='Cd', BeamInputWorkspace='Db', ContainerInputWorkspace='can',
                         OutputWorkspace='ref', SensitivityOutputWorkspace='sens')

        # remove the errors on the sensitivity, since they are too large because it is not water
        CreateWorkspace(DataX=mtd['sens'].extractX(), DataY=mtd['sens'].extractY(), NSpec=mtd['sens'].getNumberHistograms(), OutputWorkspace='sens', ParentWorkspace='sens')
        AddSampleLog(Workspace='sens', LogName='ProcessedAs', LogText='Reference')

        # Sample
        SANSILLReduction(Run='241240', ProcessAs='Sample', AbsorberInputWorkspace='Cd', BeamInputWorkspace='Db', TransmissionInputWorkspace='str',
                         ContainerInputWorkspace='can', MaskedInputWorkspace='mask', SensitivityInputWorkspace='sens', OutputWorkspace='sample')

        # Integration
        SANSILLIntegration(InputWorkspace='sample', OutputWorkspace='iq', CalculateResolution='None')
