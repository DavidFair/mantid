// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Mpl/PreviewPlotBase.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"

using MantidQt::Widgets::MplCpp::FigureCanvasQt;

namespace {
constexpr auto MANTID_PROJECTION = "mantid";
} // namespace

namespace MantidQt::MantidWidgets {

PreviewPlotBase::PreviewPlotBase(QWidget *parent)
    : QWidget(parent),
      m_canvas(new FigureCanvasQt(111, MANTID_PROJECTION, parent)),
      m_selectorActive{false} {}

/**
 * Gets the canvas used by the preview plot
 * @return The canvas
 */
Widgets::MplCpp::FigureCanvasQt *PreviewPlotBase::canvas() const {
  return m_canvas;
}

/**
 * Gets the range of the supplied axis
 * @param axisID The axis to get the range for
 * @return The axis range
 */
std::tuple<double, double> PreviewPlotBase::getAxisRange(AxisID axisID) const {
  switch (axisID) {
  case AxisID::XBottom:
    return m_canvas->gca().getXLim();
  case AxisID::YLeft:
    return m_canvas->gca().getYLim();
  }
  throw std::runtime_error(
      "Incorrect AxisID provided. Axis types are XBottom and YLeft");
}

/**
 * Converts the QPoint in pixels to axes coordinates
 * @return The axes coordinates of the QPoint
 */
QPointF PreviewPlotBase::toDataCoords(const QPoint &point) const {
  return m_canvas->toDataCoords(point);
}

/**
 * Add a range selector to a preview plot
 * @param name The name to give the range selector
 * @param type The type of the range selector
 * @return The range selector
 */
RangeSelector *
PreviewPlotBase::addRangeSelector(const QString &name,
                                  RangeSelector::SelectType type) {
  if (m_rangeSelectors.contains(name))
    throw std::runtime_error("RangeSelector already exists on preview plot.");

  m_rangeSelectors[name] = new MantidWidgets::RangeSelector(this, type);
  return m_rangeSelectors[name];
}

/**
 * Gets a range selector from the preview plot
 * @param name The name of the range selector
 * @return The range selector
 */
RangeSelector *PreviewPlotBase::getRangeSelector(const QString &name) const {
  if (!m_rangeSelectors.contains(name))
    throw std::runtime_error("RangeSelector was not found on preview plot.");
  return m_rangeSelectors[name];
}

/**
 * Add a single selector to a preview plot
 * @param name The name to give the single selector
 * @param type The type of the single selector
 * @return The single selector
 */
SingleSelector *PreviewPlotBase::addSingleSelector(
    const QString &name, SingleSelector::SelectType type, double position) {
  if (m_singleSelectors.contains(name))
    throw std::runtime_error("SingleSelector already exists on preview plot.");

  m_singleSelectors[name] =
      new MantidWidgets::SingleSelector(this, type, position);
  return m_singleSelectors[name];
}

/**
 * Gets a single selector from the preview plot
 * @param name The name of the single selector
 * @return The single selector
 */
SingleSelector *PreviewPlotBase::getSingleSelector(const QString &name) const {
  if (!m_singleSelectors.contains(name))
    throw std::runtime_error("SingleSelector was not found on preview plot.");
  return m_singleSelectors[name];
}

/**
 * Set whether or not one of the selectors on the preview plot is being moved or
 * not. This is required as we only want the user to be able to move one marker
 * at a time, otherwise the markers could get 'stuck' together.
 * @param active True if a selector is being moved.
 */
void PreviewPlotBase::setSelectorActive(bool active) {
  m_selectorActive = active;
}

/**
 * Returns True if a selector is currently being moved on the preview plot.
 * @return True if a selector is currently being moved on the preview plot.
 */
bool PreviewPlotBase::selectorActive() const { return m_selectorActive; }

void PreviewPlotBase::replot() { m_canvas->draw(); }
} // namespace MantidQt::MantidWidgets
