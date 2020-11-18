// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/CorelliPowderCalibrationApply.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
    namespace Algorithms {
        namespace {
        Kernel::Logger logger("CorelliPowderCalibrationApply");
        }

        DECLARE_ALGORITHM(CorelliPowderCalibrationApply)

        using namespace Kernel;
        using namespace API;

        /// Init method
        void CorelliPowderCalibrationApply::init() {
            
            //
            // InputWorkspace
            // [Input, Required, MatrixWorkspace or EventsWorkspace]
            // workspace to which the calibration should be applied
            declareProperty(
                std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                    "InputWorkspace",
                    "",
                    Direction::InOut,
                    PropertyMode::Mandatory),
                "Input workspace for calibration");

            //
            // CalibrationTable
            // [Input, Optional, TableWorkspace]
            // workspace resulting from uploading
            declareProperty(
                std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>(
                    "CalibrationTable",
                    "",
                    Direction::Input,
                    PropertyMode::Optional),
                "TableWorkspace containing calibration table");

            //
            // DatabaseDirectory
            // [Input, Optional, string]
            // absolute path to the database.

            //
            // OutputWorkspace
            // if emtpy, InputWorkspace will be calibrated.
            declareOrReplaceProperty(
                std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                    "OutputWorkspace",
                    "",
                    Direction::Output,
                    PropertyMode::Optional),
                    "Calibrated input workspace clone");
        }
    
    /**
     * @brief Executes the algorithm.
     * 
     */
    void CorelliPowderCalibrationApply::exec(){

    }

    } // namespace Algorithm

} // namespace Mantid
