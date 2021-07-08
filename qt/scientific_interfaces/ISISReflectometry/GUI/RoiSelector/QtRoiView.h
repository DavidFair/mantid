// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/QWidgetGroup.h"
#include "IRoiView.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/InstrumentDisplay.h"
#include "ui_RoiWidget.h"

#include <algorithm>
#include <memory>

namespace MantidQt::MantidWidgets {
class InstrumentDisplay;
}

namespace MantidQt::MantidWidgets {
class ContourPreviewPlot;
class PreviewPlot;
} // namespace MantidQt::MantidWidgets

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/** QtRoiView : Provides an interface for the "Regions of Interest" widget
in the ISIS Reflectometry interface.
*/
class QtRoiView : public QWidget, public IRoiView {
  Q_OBJECT
public:
  /// Constructor
  explicit QtRoiView(QWidget *parent = nullptr);
  void subscribe(RoiViewSubscriber *notifyee) override;

  std::string getWorkspaceName() const override;
  void setWorkspaceName(std::string const &workspaceName) override;
  double getAngle() const override;
  void setAngle(double angle) override;
  void addRangeSelector(std::string const &name) override;
  void setRangeSelectorBounds(std::string const &name, double min, double max) override;
  std::pair<double, double> getRangeSelectorRange(std::string const &name) const override;
  void setRangeSelectorRange(std::string const &name, std::pair<double, double> const &range) override;
  void setBounds(double minValue, double maxValue) override;

  void plot3D(Mantid::API::MatrixWorkspace_sptr ws) override;
  void plot2D(Mantid::API::MatrixWorkspace_sptr ws) override;
  void plot1D(Mantid::API::MatrixWorkspace_sptr ws, size_t wsIdx, std::string const &title) override;
  void clear1DPlot() override;
  void zoomOut2D() override;
  void zoomOut1D() override;
  void set1DPlotScaleLogLog() override;

private slots:
  void on_actionUpdateWorkspace_triggered();
  void on_actionHome_triggered();
  void on_actionApply_triggered();
  void onRangeSelectionChanged();

private:
  void initLayout();
  void setupInstDisplay();
  std::string getText(QLineEdit const &lineEdit) const;
  void setText(QLineEdit &lineEdit, std::string const &text);

  Ui::RoiWidget m_ui;
  RoiViewSubscriber *m_notifyee;
  MantidQt::MantidWidgets::ContourPreviewPlot *m_2DPlot; // TODO use unique_ptrs
  MantidQt::MantidWidgets::PreviewPlot *m_1DPlot;

  std::unique_ptr<MantidQt::MantidWidgets::InstrumentActor> m_instActor;
  std::unique_ptr<MantidQt::MantidWidgets::InstrumentDisplay> m_instDisplay;

  friend class Encoder;
  friend class Decoder;
  friend class CoderCommonTester;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
