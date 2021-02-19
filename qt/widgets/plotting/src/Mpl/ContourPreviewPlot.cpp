// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Mpl/ContourPreviewPlot.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include "MantidQtWidgets/MplCpp/MantidAxes.h"

#include <QVBoxLayout>

using namespace Mantid::API;
using namespace MantidQt::Widgets::MplCpp;

namespace {
Mantid::Kernel::Logger g_log("ContourPreviewPlot");
} // namespace

namespace MantidQt {
namespace MantidWidgets {

ContourPreviewPlot::ContourPreviewPlot(QWidget *parent, bool observeADS)
    : PreviewPlotBase(parent),
      m_wsRemovedObserver(*this, &ContourPreviewPlot::onWorkspaceRemoved),
      m_wsReplacedObserver(*this, &ContourPreviewPlot::onWorkspaceReplaced) {
  createLayout();
  watchADS(observeADS);
}

ContourPreviewPlot::~ContourPreviewPlot() { watchADS(false); }

/**
 * Initialize the layout for the widget
 */
void ContourPreviewPlot::createLayout() {
  auto plotLayout = new QVBoxLayout(this);
  plotLayout->setContentsMargins(0, 0, 0, 0);
  plotLayout->setSpacing(0);
  plotLayout->addWidget(m_canvas, 0, 0);
  setLayout(plotLayout);
}

/**
 * Enable/disable the ADS observers
 * @param on If true ADS observers are enabled else they are disabled
 */
void ContourPreviewPlot::watchADS(bool on) {
  auto &notificationCenter = AnalysisDataService::Instance().notificationCenter;
  if (on) {
    notificationCenter.addObserver(m_wsRemovedObserver);
    notificationCenter.addObserver(m_wsReplacedObserver);
  } else {
    notificationCenter.removeObserver(m_wsReplacedObserver);
    notificationCenter.removeObserver(m_wsRemovedObserver);
  }
}

/**
 * Observer method called when a workspace is removed from the ADS
 * @param nf A pointer to the notification object
 */
void ContourPreviewPlot::onWorkspaceRemoved(Mantid::API::WorkspacePreDeleteNotification_ptr nf) {
  if (auto workspace = std::dynamic_pointer_cast<MatrixWorkspace>(nf->object())) {
    // If the artist has already been removed, ignore.
    bool workspaceRemoved = false;
    try {
      workspaceRemoved = m_canvas->gca<MantidAxes>().removeWorkspaceArtists(workspace);
    } catch (Mantid::PythonInterface::PythonException &) {
    }
    if (workspaceRemoved) {
      m_canvas->draw();
    }
  }
}

/**
 * Observer method called when a workspace is replaced in the ADS
 * @param nf A pointer to the notification object
 */
void ContourPreviewPlot::onWorkspaceReplaced(Mantid::API::WorkspaceBeforeReplaceNotification_ptr nf) {
  if (auto oldWorkspace = std::dynamic_pointer_cast<MatrixWorkspace>(nf->oldObject())) {
    if (auto newWorkspace = std::dynamic_pointer_cast<MatrixWorkspace>(nf->newObject())) {
      if (m_canvas->gca<MantidAxes>().replaceWorkspaceArtists(newWorkspace)) {
        m_canvas->draw();
      }
    }
  }
}

/**
 * Set the face colour for the canvas
 * @param colour A new colour for the figure facecolor
 */
void ContourPreviewPlot::setCanvasColour(QColor const &colour) { m_canvas->gcf().setFaceColor(colour); }

/**
 * Sets the workspace for the contour plot
 * @param workspace The workspace to plot on the contour plot.
 */
void ContourPreviewPlot::setWorkspace(const MatrixWorkspace_sptr &workspace) {
  if (workspace) {
    auto axes = m_canvas->gca<MantidAxes>();
    axes.pcolormesh(workspace);

    axes.relim();
    m_canvas->draw();
  } else {
    g_log.warning("Cannot plot a null workspace.");
  }
}

void ContourPreviewPlot::replot() {
  m_canvas->draw();
  emit redraw();
}
} // namespace MantidWidgets
} // namespace MantidQt
