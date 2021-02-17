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
#include "MantidQtWidgets/Plotting/ContourPreviewPlot.h"
#include "MantidQtWidgets/Plotting/PreviewPlot.h"

using Mantid::API::MatrixWorkspace_sptr;
using MantidQt::MantidWidgets::ContourPreviewPlot;
using MantidQt::MantidWidgets::PreviewPlot;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/** Constructor
 * @param parent :: [input] The parent of this widget
 */
QtRoiView::QtRoiView(QWidget *parent)
    : QWidget(parent), m_2DPlot(new ContourPreviewPlot(this)),
      m_1DPlot(new PreviewPlot(this)) {
  initLayout();
}

void QtRoiView::subscribe(RoiViewSubscriber *notifyee) {
  m_notifyee = notifyee;
}

void QtRoiView::initLayout() {
  m_ui.setupUi(this);
  m_ui.homeButton->setIcon(Icons::getIcon("mdi.home", "black", 1.3));
  m_ui.plotsLayout->addWidget(m_2DPlot);
  m_ui.plotsLayout->addWidget(m_1DPlot);
}

void QtRoiView::plot2D(MatrixWorkspace_sptr ws) { m_2DPlot->setWorkspace(ws); }

void QtRoiView::plot1D(MatrixWorkspace_sptr ws, size_t wsIdx,
                       std::string const &title) {
  m_1DPlot->clear();
  m_1DPlot->addSpectrum(QString::fromStdString(title), ws, wsIdx);
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

void QtRoiView::zoomOut2D() {}

void QtRoiView::zoomOut1D() {}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
