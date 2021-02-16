// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtRoiView.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/UsageService.h"
#include "MantidQtIcons/Icon.h"
#include "MantidQtWidgets/MplCpp/ColorbarWidget.h"
#include "MantidQtWidgets/MplCpp/Figure.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include "MantidQtWidgets/MplCpp/MantidAxes.h"
#include "MantidQtWidgets/MplCpp/RangeMarker.h"
//#include "MantidQtWidgets/MplCpp/ScalarMappable.h"

using Mantid::API::MatrixWorkspace_sptr;
using MantidQt::Widgets::MplCpp::FigureCanvasQt;
using MantidQt::Widgets::MplCpp::MantidAxes;
// using MantidQt::Widgets::MplCpp::Normalize;
using MantidQt::Widgets::MplCpp::PanZoomTool;
// using MantidQt::Widgets::MplCpp::ScalarMappable;

namespace {
constexpr auto PROJECTION = "mantid";
}

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/** Constructor
 * @param parent :: [input] The parent of this widget
 */
QtRoiView::QtRoiView(QWidget *parent)
    : QWidget(parent), m_canvas2D{new FigureCanvasQt(111, PROJECTION, this)},
      m_canvas1D{new FigureCanvasQt(111, PROJECTION, this)},
      m_zoom2D(m_canvas2D), m_zoom1D(m_canvas1D) {
  initLayout();
}

void QtRoiView::subscribe(RoiViewSubscriber *notifyee) {
  m_notifyee = notifyee;
}

void QtRoiView::initLayout() {
  m_ui.setupUi(this);
  m_ui.plotsLayout->addWidget(m_canvas2D);
  m_ui.plotsLayout->addWidget(m_canvas1D);
  m_canvas2D->installEventFilterToMplCanvas(this);
  m_canvas1D->installEventFilterToMplCanvas(this);
  m_zoom2D.enableZoom(true);
  m_zoom1D.enableZoom(true);
  m_ui.homeButton->setIcon(Icons::getIcon("mdi.home", "black", 1.3));
}

void QtRoiView::plot2D(MatrixWorkspace_sptr ws) {
  auto axes = m_canvas2D->gca<MantidAxes>();
  axes.clear();
  axes.pcolormesh(ws);

  // auto fig = m_canvas->gcf();
  // ScalarMappable mappable(Normalize(-1, 1), "jet");
  // auto colorbar = fig.colorbar(mappable, cax);
  m_canvas2D->draw();
}

void QtRoiView::plot1D(MatrixWorkspace_sptr ws, size_t wsIdx,
                       std::string const &color, std::string const &title) {
  auto axes = m_canvas1D->gca<MantidAxes>();
  axes.clear();
  axes.plot(ws, wsIdx, QString::fromStdString(color),
            QString::fromStdString(title));
  axes.setXScale("log");
  axes.setYScale("log");
  axes.relim();
  // axes.autoscaleView();
  m_canvas1D->draw();
}

void QtRoiView::on_actionUpdateWorkspace_triggered() {
  m_notifyee->notifyWorkspaceChanged();
}

void QtRoiView::on_actionHome_triggered() { m_notifyee->notifyHome(); }

std::string QtRoiView::getText(QLineEdit const &lineEdit) const {
  return lineEdit.text().toStdString();
}

void QtRoiView::setText(QLineEdit &lineEdit, std::string const &text) {
  auto textAsQString = QString::fromStdString(text);
  lineEdit.setText(textAsQString);
}

std::string QtRoiView::getWorkspaceName() const {
  return getText(*m_ui.textWorkspace);
}

void QtRoiView::setWorkspaceName(std::string const &workspaceName) {
  setText(*m_ui.textWorkspace, workspaceName);
}

void QtRoiView::zoomOut2D() { m_zoom2D.zoomOut(); }

void QtRoiView::zoomOut1D() { m_zoom1D.zoomOut(); }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
