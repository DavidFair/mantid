// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "RoiPresenter.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "IRoiPresenter.h"
#include "IRoiView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <sstream>

using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;

namespace {
static constexpr const char *ROI_SELECTOR_NAME = "roi_selector";

std::string processingInstructionsFromRanges(
    std::vector<std::pair<double, double>> const &ranges) {
  std::ostringstream ss;
  bool isFirst = true;
  for (auto range : ranges) {
    if (!isFirst)
      ss << '+';
    ss << static_cast<int>(range.first) << '-'
       << static_cast<int>(range.second);
  }
  return ss.str();
}
}

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/** Constructor
 * @param view :: The view we are handling
 */
RoiPresenter::RoiPresenter(IRoiView *view) : m_view(view) {
  m_view->subscribe(this);
  m_view->addRangeSelector(ROI_SELECTOR_NAME);
}

void RoiPresenter::acceptMainPresenter(IBatchPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
}

/** Notification received when the user-specified input workspace has changed
 */
void RoiPresenter::notifyWorkspaceChanged() {
  auto const inputName = m_view->getWorkspaceName();
  auto reducedWorkspace = reduceWorkspace(inputName);
  refresh2DPlot(inputName);
  refresh1DPlot(reducedWorkspace);
}

/** Reduce the workspace using the default settings on the GUI
 *
 * @param inputName : the user-specified input workspace name
 * @returns : the reduced workspace, or null if the reduction failed
 */
MatrixWorkspace_sptr
RoiPresenter::reduceWorkspace(std::string const &inputName) {
  auto alg = Mantid::API::AlgorithmManager::Instance().create(
      "ReflectometryISISLoadAndProcess");
  alg->setChild(true);
  alg->setProperty("InputRunList", inputName);
  auto properties = m_mainPresenter->rowProcessingProperties();
  for (auto kvp : properties)
    alg->setProperty(kvp.first, kvp.second);
  try {
    alg->execute();
  } catch (...) {
  }

  MatrixWorkspace_sptr reducedWorkspace;
  if (alg->isExecuted()) {
    reducedWorkspace = alg->getProperty("OutputWorkspaceBinned");
  }
  return reducedWorkspace;
}

/** Refresh the 2D plot from the input workspace, if it exists in the ADS; does
 * nothing if not
 *
 * @param inputName : the user-specified input workspace name
 */
void RoiPresenter::refresh2DPlot(std::string const &inputName) {
  auto const &ads = Mantid::API::AnalysisDataService::Instance();
  auto workspaceName = inputName;
  // Check the input workspace exists. Note that it may have had the the prefix
  // "TOF_" applied when loaded by the reduction algorithm
  if (!ads.doesExist(workspaceName))
    workspaceName = std::string("TOF_") + workspaceName;
  if (ads.doesExist(workspaceName)) {
    auto workspace = ads.retrieveWS<MatrixWorkspace>(workspaceName);
    m_view->plot2D(workspace);
    m_view->setRangeSelectorBounds(
        ROI_SELECTOR_NAME, 1,
        static_cast<double>(workspace->getNumberHistograms() - 1));
  }
}

/** Refresh the 1D plot of the reduced workspace, if it exists; does nothing if
 * it is null
 *
 * @param workspace : the reduced workspace
 */
void RoiPresenter::refresh1DPlot(MatrixWorkspace_sptr workspace) {
  if (!workspace)
    return;

  m_view->clear1DPlot();
  for (size_t idx = 0; idx < workspace->getNumberHistograms(); ++idx)
    m_view->plot1D(workspace, idx, "IvsQ");
  m_view->set1DPlotScaleLogLog();
}

void RoiPresenter::notifyHome() {
  m_view->zoomOut2D();
  m_view->zoomOut1D();
}

void RoiPresenter::notifyRoiChanged() {
  // Convert the ROI to a string
  auto const range = m_view->getRangeSelectorRange(ROI_SELECTOR_NAME);
  auto processingInstructions = processingInstructionsFromRanges({range});
  // Notify main presenter to update settings
  m_mainPresenter->notifyProcessingInstructionsChanged(processingInstructions);
  // Refresh the reduced data plot
  auto const inputName = m_view->getWorkspaceName();
  auto reducedWorkspace = reduceWorkspace(inputName);
  refresh1DPlot(reducedWorkspace);
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
