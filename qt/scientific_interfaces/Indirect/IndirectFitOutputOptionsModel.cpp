// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitOutputOptionsModel.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/Workspace.h"

using namespace Mantid::API;

namespace {

MatrixWorkspace_sptr convertToMatrixWorkspace(Workspace_sptr workspace) {
  return boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
}

std::string oneDataPointErrorMessage() {
  return "The plotting of data in one of the result workspaces failed:\n\n "
         "Workspace has only one data point";
}

std::unordered_map<std::string, std::size_t> extractAxisLabels(Axis *axis) {
  auto const *textAxis = boost::static_pointer_cast<TextAxis>(axis);
  std::unordered_map<std::string, std::size_t> labels;

  for (auto i = 0u; i < textAxis->length(); ++i)
    labels[textAxis->label(i)] = i;

  return labels;
}

std::unordered_map<std::string, std::size_t>
extractAxisLabels(MatrixWorkspace_const_sptr workspace,
                  std::size_t const &axisIndex) {
  auto const axis = workspace->getAxis(axisIndex);
  if (axis->isText())
    return extractAxisLabels(axis);
  else
    return std::unordered_map<std::string, std::size_t>();
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

// IndirectFitOutputOptionsModel::IndirectFitOutputOptionsModel(
//    std::unique_ptr<IndirectFitAnalysisTab> tab)
//    : m_tab(std::move(tab)) {}

IndirectFitOutputOptionsModel::IndirectFitOutputOptionsModel() {}

void IndirectFitOutputOptionsModel::setActivePlotWorkspace(
    WorkspaceGroup_sptr workspace) {
  m_plotWorkspace = workspace;
}

void IndirectFitOutputOptionsModel::plotResult(std::string const &plotType) {
  if (m_plotWorkspace) {
    if (plotType == "All")
      plotAll(m_plotWorkspace);
    else
      plotParameter(m_plotWorkspace, plotType);
  }
}

void IndirectFitOutputOptionsModel::plotAll(WorkspaceGroup_sptr workspaces) {
  for (auto const &workspace : *workspaces)
    plotAll(convertToMatrixWorkspace(workspace));
}

void IndirectFitOutputOptionsModel::plotParameter(
    WorkspaceGroup_sptr workspaces, std::string const &parameter) {
  for (auto const &workspace : *workspaces)
    plotParameter(convertToMatrixWorkspace(workspace), parameter);
}

void IndirectFitOutputOptionsModel::plotAll(MatrixWorkspace_sptr workspace) {
  auto const numberOfDataPoints = workspace->blocksize();
  if (numberOfDataPoints > 1)
    plotSpectrum(workspace);
  else
    throw std::runtime_error(oneDataPointErrorMessage());
}

void IndirectFitOutputOptionsModel::plotParameter(
    MatrixWorkspace_sptr workspace, std::string const &parameterToPlot) {
  auto const numberOfDataPoints = workspace->blocksize();
  if (numberOfDataPoints > 1)
    plotSpectrum(workspace, parameterToPlot);
  else
    throw std::runtime_error(oneDataPointErrorMessage());
}

void IndirectFitOutputOptionsModel::plotSpectrum(
    MatrixWorkspace_sptr workspace, std::string const &parameterToPlot) {
  auto const labels = extractAxisLabels(workspace, 1);
  for (auto const &parameter : m_fitParameters) {
    if (parameter == parameterToPlot) {
      auto const param = labels.find(parameter);
      if (param != labels.end())
        break;
      // m_tab->plotSpectrum(workspace->getName(), param->second, true);
    }
  }
}

void IndirectFitOutputOptionsModel::plotSpectrum(
    MatrixWorkspace_sptr workspace) {
  for (auto index = 0u; index < workspace->getNumberHistograms(); ++index)
    break;
  // m_tab->plotSpectrum(workspace->getName(), index, true);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
