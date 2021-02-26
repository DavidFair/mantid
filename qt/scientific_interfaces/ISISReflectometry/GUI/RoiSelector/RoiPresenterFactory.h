// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "IRoiPresenter.h"
#include "IRoiView.h"
#include "RoiPresenter.h"
#include <memory>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class RoiPresenterFactory {
public:
  RoiPresenterFactory(std::string const &loadAlgorithm)
      : m_loadAlgorithm(loadAlgorithm) {}

  std::unique_ptr<IRoiPresenter> make(IRoiView *view) {
    return std::make_unique<RoiPresenter>(view, m_loadAlgorithm);
  }

private:
  std::string m_loadAlgorithm;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
