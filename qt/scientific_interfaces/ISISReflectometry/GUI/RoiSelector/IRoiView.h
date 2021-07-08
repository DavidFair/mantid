// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include <string>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL RoiViewSubscriber {
public:
  virtual void notifyWorkspaceChanged() = 0;
  virtual void notifyHome() = 0;
  virtual void notifyApply() = 0;
  virtual void notifyRoiChanged() = 0;
};

/** @class IRoiView

IRoiView is the base view class for the tab "Regions of Interest" in the
Reflectometry Interface. It contains no QT specific functionality as that should
be handled by a subclass.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL IRoiView {
public:
  virtual void subscribe(RoiViewSubscriber *notifyee) = 0;
  virtual ~IRoiView() = default;
  virtual std::string getWorkspaceName() const = 0;
  virtual void setWorkspaceName(std::string const &workspaceName) = 0;
  virtual double getAngle() const = 0;
  virtual void setAngle(double angle) = 0;
  virtual void plot3D(Mantid::API::MatrixWorkspace_sptr ws) = 0;
  virtual void plot2D(Mantid::API::MatrixWorkspace_sptr ws) = 0;
  virtual void plot1D(Mantid::API::MatrixWorkspace_sptr ws, size_t wsIdx, std::string const &title) = 0;
  virtual void clear1DPlot() = 0;
  virtual void zoomOut2D() = 0;
  virtual void zoomOut1D() = 0;
  virtual void set1DPlotScaleLogLog() = 0;
  virtual void addRangeSelector(std::string const &name) = 0;
  virtual void setRangeSelectorBounds(std::string const &name, double min, double max) = 0;
  virtual std::pair<double, double> getRangeSelectorRange(std::string const &name) const = 0;
  virtual void setRangeSelectorRange(std::string const &name, std::pair<double, double> const &range) = 0;
  virtual void setBounds(double minValue, double maxValue) = 0;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
