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
#include "MantidKernel/Tolerance.h"
#include "MantidKernel/UserStringParser.h"

#include <sstream>

using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;

namespace {
Mantid::Kernel::Logger g_log("Reflectometry GUI");

static constexpr const char *ROI_SELECTOR_NAME = "roi_selector";

/** Convert a vector of ranges to a string
 *
 * @param ranges : a vector of ranges, where each range is a pair of doubles
 * @returns : a grouping pattern string (see GroupDetectors for details)
 */
std::string
roiStringFromRanges(std::vector<std::pair<double, double>> const &ranges) {
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

/** Convert a string to a vector of ranges
 *
 * @param roiString : a grouping pattern string (see GroupDetectors for details)
 * @returns : a vector of ranges, where each range is a pair of doubles
 */
std::vector<std::pair<double, double>>
roiRangesFromString(std::string const &roiString) {
  auto result = std::vector<std::pair<double, double>>();
  auto parser = Mantid::Kernel::UserStringParser();
  auto const ranges = parser.parse(roiString);
  std::transform(ranges.cbegin(), ranges.cend(), std::back_inserter(result),
                 [](auto const &range) {
                   auto minmax =
                       std::minmax_element(range.cbegin(), range.cend());
                   return std::make_pair(static_cast<double>(*minmax.first),
                                         static_cast<double>(*minmax.second));
                 });
  return result;
}
}

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/** Constructor
 * @param view :: The view we are handling
 */
RoiPresenter::RoiPresenter(IRoiView *view, std::string const &loadAlgorithm)
    : m_view(view), m_loadAlgorithm(loadAlgorithm), m_loaded(false) {
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
  loadWorkspace(inputName);
  refresh2DPlot(inputName);
  m_mainPresenter->notifyRoiDataUpdated();
  refresh1DPlot();
}

void RoiPresenter::loadWorkspace(std::string const &inputName) {
  if (Mantid::API::AnalysisDataService::Instance().doesExist(inputName))
    return;

  auto alg = Mantid::API::AlgorithmManager::Instance().create(m_loadAlgorithm);
  alg->setProperty("Filename", inputName);
  alg->setProperty("OutputWorkspace", inputName);
  try {
    alg->execute();
    m_loaded = true;
  } catch (std::runtime_error const &ex) {
    g_log.error(ex.what());
  }
}

/** Refresh the 2D plot from the input workspace, if it exists in the ADS; does
 * nothing if not
 *
 * @param inputName : the user-specified input workspace name
 */
void RoiPresenter::refresh2DPlot(std::string const &inputName) {
  auto const &ads = Mantid::API::AnalysisDataService::Instance();
  if (!ads.doesExist(inputName))
    return;

  auto workspace = ads.retrieveWS<MatrixWorkspace>(inputName);
  m_view->plot2D(workspace);

  auto const nSpec = workspace->getNumberHistograms();
  auto const minSpec =
      static_cast<double>(workspace->getSpectrum(0).getSpectrumNo());
  auto const maxSpec =
      static_cast<double>(workspace->getSpectrum(nSpec - 1).getSpectrumNo());
  m_view->setBounds(minSpec, maxSpec);
  m_view->setRangeSelectorBounds(ROI_SELECTOR_NAME, minSpec, maxSpec);
  m_view->setRangeSelectorRange(ROI_SELECTOR_NAME,
                                std::make_pair(minSpec + 1, maxSpec - 1));
}

/** Refresh the 1D plot of the reduced workspace, if it exists; does nothing if
 * it is null
 *
 * @param workspace : the reduced workspace
 */
void RoiPresenter::refresh1DPlot() {
  auto workspace = m_mainPresenter->reduceWorkspace(m_view->getWorkspaceName());
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

void RoiPresenter::notifyApply() { m_mainPresenter->notifyRoiSaved(); }

void RoiPresenter::notifyRoiChanged() { refresh1DPlot(); }

boost::optional<double> RoiPresenter::getAngle() const {
  auto const angle = m_view->getAngle();
  if (angle > Mantid::Kernel::Tolerance)
    return angle;

  return boost::none;
}

/** Get the currently selected ROI range
 *
 * returns : the ROI as a grouping pattern string (see the GroupDetectors
 * algorithm)
 */
std::string RoiPresenter::getSelectedRoi() const {
  // TODO expand to support multiple ranges
  auto const range = m_view->getRangeSelectorRange(ROI_SELECTOR_NAME);
  return roiStringFromRanges({range});
}

void RoiPresenter::setSelectedRoi(std::string const &roi) {
  auto ranges = roiRangesFromString(roi);
  if (ranges.size() < 1)
    throw std::runtime_error(
        std::string("No valid ranges found parsing regions of interest: ") +
        roi);
  // TODO expand to support multiple ranges
  auto range = ranges[0];
  m_view->setRangeSelectorRange(ROI_SELECTOR_NAME, range);
  refresh1DPlot();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
