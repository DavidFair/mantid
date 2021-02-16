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

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/** Constructor
 * @param view :: The view we are handling
 */
RoiPresenter::RoiPresenter(IRoiView *view) : m_view(view) {
  m_view->subscribe(this);
}

void RoiPresenter::acceptMainPresenter(IBatchPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
}

void RoiPresenter::notifyWorkspaceChanged() {
  auto const inputName = m_view->getWorkspaceName();

  // Load and reduce the workspace
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

  // Refresh the 2D plot from the original workspace, if the workspace exists
  // in the ADS. It may have had the TOF_ prefix added.
  auto const &ads = Mantid::API::AnalysisDataService::Instance();
  auto workspaceName = inputName;
  if (!ads.doesExist(workspaceName))
    workspaceName = std::string("TOF_") + workspaceName;
  if (ads.doesExist(workspaceName)) {
    auto workspace =
        ads.retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
    m_view->plot2D(workspace);
  }

  // Refresh 1D plot of the reduced workspace
  if (alg->isExecuted()) {
    Mantid::API::MatrixWorkspace_sptr reducedWorkspace =
        alg->getProperty("OutputWorkspaceBinned");
    if (reducedWorkspace)
      m_view->plot1D(reducedWorkspace, 0, "#000000",
                     reducedWorkspace->getName());
  }
}

void RoiPresenter::notifyHome() {
  m_view->zoomOut2D();
  m_view->zoomOut1D();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
