// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Batch/IBatchPresenter.h"
#include <string>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/** @class IRoiPresenter

IRoiPresenter is an interface which defines the functions that need
to be implemented by a concrete 'Regions of Interest' presenter
*/

class IRoiPresenter {
public:
  virtual ~IRoiPresenter() = default;
  virtual void acceptMainPresenter(IBatchPresenter *mainPresenter) = 0;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
