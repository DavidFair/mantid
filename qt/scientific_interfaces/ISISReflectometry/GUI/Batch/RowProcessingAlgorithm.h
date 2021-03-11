// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include <boost/optional.hpp>
#include <map>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
class Batch;
class Row;
class IConfiguredAlgorithm;

using AlgorithmRuntimeProps = std::map<std::string, std::string>;

MANTIDQT_ISISREFLECTOMETRY_DLL MantidQt::API::IConfiguredAlgorithm_sptr
createConfiguredAlgorithm(Batch const &model, Row &row);
MANTIDQT_ISISREFLECTOMETRY_DLL MantidQt::API::IConfiguredAlgorithm_sptr
createConfiguredAlgorithm(Batch const &model, std::string const &workspaceName,
                          boost::optional<double> const &angle);

MANTIDQT_ISISREFLECTOMETRY_DLL AlgorithmRuntimeProps
createAlgorithmRuntimeProps(Batch const &model, Row const &row);
MANTIDQT_ISISREFLECTOMETRY_DLL AlgorithmRuntimeProps
createAlgorithmRuntimeProps(Batch const &model,
                            boost::optional<double> const &angle = boost::none);

MANTIDQT_ISISREFLECTOMETRY_DLL Mantid::API::MatrixWorkspace_sptr
getOutputWorkspace(const Mantid::API::IAlgorithm_sptr &algorithm);
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
