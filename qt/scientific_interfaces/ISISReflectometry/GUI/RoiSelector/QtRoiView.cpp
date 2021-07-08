// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtRoiView.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidKernel/UsageService.h"
#include "MantidQtIcons/Icon.h"
#include "MantidQtWidgets/InstrumentView/InstrumentDisplay.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedCylinder.h"
#include "MantidQtWidgets/Plotting/ContourPreviewPlot.h"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"

#include <qwidget.h>

#include <memory>

using Mantid::API::MatrixWorkspace_sptr;
using MantidQt::MantidWidgets::ContourPreviewPlot;
using MantidQt::MantidWidgets::PreviewPlot;
using MantidQt::MantidWidgets::RangeSelector;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

using Mantid::Geometry::ComponentInfo;
using MantidQt::MantidWidgets::InstrumentActor;
using MantidQt::MantidWidgets::InstrumentDisplay;
using MantidQt::MantidWidgets::UnwrappedCylinder;

/** Constructor
 * @param parent :: [input] The parent of this widget
 */
QtRoiView::QtRoiView(QWidget *parent)
    : QWidget(parent), m_2DPlot(new ContourPreviewPlot(this)), m_1DPlot(new PreviewPlot(this)), m_instActor(nullptr),
      m_instDisplay(nullptr) {
  initLayout();
  setupInstDisplay();
}

void QtRoiView::subscribe(RoiViewSubscriber *notifyee) { m_notifyee = notifyee; }

void QtRoiView::initLayout() {
  m_ui.setupUi(this);
  m_ui.homeButton->setIcon(Icons::getIcon("mdi.home", "black", 1.3));
  m_ui.angleSpinBox->setSpecialValueText("Unset");

  m_ui.plotsLayout->addWidget(m_2DPlot);
  m_ui.plotsLayout->addWidget(m_1DPlot);
}

void QtRoiView::setupInstDisplay() {
  auto instDisplayParent = std::make_unique<QWidget>(this);
  m_instDisplay = std::make_unique<InstrumentDisplay>(instDisplayParent.get());
  m_ui.roiLayout->addWidget(instDisplayParent.release());
  // TODO: Instrument Actor -> Surface factory -> instDisplay::setSurface()
}

void QtRoiView::plot3D(MatrixWorkspace_sptr ws) {
  bool autoscaling = true;
  auto scaleMin = 0.0;
  auto scaleMax = 1.0;
  m_instActor =
      std::make_unique<InstrumentActor>(QString::fromStdString(ws->getName()), autoscaling, scaleMin, scaleMax);
  const auto &componentInfo = m_instActor->componentInfo();
  auto sample_pos = componentInfo.samplePosition();
  auto axis = Mantid::Kernel::V3D(0, 1, 0); // CYLINDRICAL_Y

  m_instDisplay->setSurface(std::make_shared<UnwrappedCylinder>(m_instActor.get(), sample_pos, axis));
}

void QtRoiView::plot2D(MatrixWorkspace_sptr ws) { m_2DPlot->setWorkspace(ws); }

void QtRoiView::plot1D(MatrixWorkspace_sptr ws, size_t wsIdx, std::string const &title) {
  m_1DPlot->addSpectrum(QString::fromStdString(title), ws, wsIdx);
}

void QtRoiView::clear1DPlot() { m_1DPlot->clear(); }

void QtRoiView::on_actionUpdateWorkspace_triggered() { m_notifyee->notifyWorkspaceChanged(); }

void QtRoiView::on_actionHome_triggered() { m_notifyee->notifyHome(); }

void QtRoiView::on_actionApply_triggered() { m_notifyee->notifyApply(); }

std::string QtRoiView::getText(QLineEdit const &lineEdit) const { return lineEdit.text().toStdString(); }

void QtRoiView::setText(QLineEdit &lineEdit, std::string const &text) {
  auto textAsQString = QString::fromStdString(text);
  lineEdit.setText(textAsQString);
}

std::string QtRoiView::getWorkspaceName() const { return getText(*m_ui.textWorkspace); }

void QtRoiView::setWorkspaceName(std::string const &workspaceName) { setText(*m_ui.textWorkspace, workspaceName); }

double QtRoiView::getAngle() const { return m_ui.angleSpinBox->value(); }

void QtRoiView::setAngle(double angle) { m_ui.angleSpinBox->setValue(angle); }

void QtRoiView::addRangeSelector(std::string const &name) {
  auto rangeSelector = m_2DPlot->addRangeSelector(QString::fromStdString(name), RangeSelector::SelectType::YMINMAX);
  connect(rangeSelector, SIGNAL(selectionChanged(double, double)), this, SLOT(onRangeSelectionChanged()));
}

void QtRoiView::onRangeSelectionChanged() { m_notifyee->notifyRoiChanged(); }

void QtRoiView::setRangeSelectorBounds(std::string const &name, double min, double max) {
  auto rangeSelector = m_2DPlot->getRangeSelector(QString::fromStdString(name));
  rangeSelector->setBounds(min, max);
}

std::pair<double, double> QtRoiView::getRangeSelectorRange(std::string const &name) const {
  auto rangeSelector = m_2DPlot->getRangeSelector(QString::fromStdString(name));
  return rangeSelector->getRange();
}

void QtRoiView::setRangeSelectorRange(std::string const &name, std::pair<double, double> const &range) {
  auto rangeSelector = m_2DPlot->getRangeSelector(QString::fromStdString(name));
  rangeSelector->setRange(range);
}

void QtRoiView::setBounds(double minValue, double maxValue) {
  m_2DPlot->setAxisRange(QPair(minValue, maxValue), MantidWidgets::AxisID::YLeft);
  m_2DPlot->replot();
}

void QtRoiView::zoomOut2D() {}

void QtRoiView::zoomOut1D() {}

void QtRoiView::set1DPlotScaleLogLog() {
  m_1DPlot->setXScaleLog();
  m_1DPlot->setYScaleLog();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
