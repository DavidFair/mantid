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

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/** @class RoiPresenter

RoiPresenter is a presenter class for the widget 'Regions of Interest' in the
ISIS Reflectometry Interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL RoiPresenter : public IRoiPresenter,
                                                    public RoiViewSubscriber {
public:
  explicit RoiPresenter(IRoiView *view);
  void acceptMainPresenter(IBatchPresenter *mainPresenter) override;
  void notifyWorkspaceChanged() override;
  void notifyHome() override;

private:
  IBatchPresenter *m_mainPresenter;
  IRoiView *m_view;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
