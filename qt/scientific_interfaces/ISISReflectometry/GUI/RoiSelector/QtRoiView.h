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
#include "MantidQtWidgets/MplCpp/PanZoomTool.h"
#include "ui_RoiWidget.h"
#include <memory>

namespace MantidQt::Widgets::MplCpp {
class FigureCanvasQt;
}

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

  void plot2D(Mantid::API::MatrixWorkspace_sptr ws) override;
  void plot1D(Mantid::API::MatrixWorkspace_sptr ws, size_t wsIdx,
              std::string const &color, std::string const &title) override;
  void zoomOut2D() override;
  void zoomOut1D() override;

private slots:
  void on_actionUpdateWorkspace_triggered();
  void on_actionHome_triggered();

private:
  void initLayout();
  std::string getText(QLineEdit const &lineEdit) const;
  void setText(QLineEdit &lineEdit, std::string const &text);

  Ui::RoiWidget m_ui;
  RoiViewSubscriber *m_notifyee;
  Widgets::MplCpp::FigureCanvasQt *m_canvas2D;
  Widgets::MplCpp::FigureCanvasQt *m_canvas1D;
  Widgets::MplCpp::PanZoomTool m_zoom2D;
  Widgets::MplCpp::PanZoomTool m_zoom1D;

  friend class Encoder;
  friend class Decoder;
  friend class CoderCommonTester;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
