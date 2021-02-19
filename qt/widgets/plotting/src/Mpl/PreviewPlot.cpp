// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Mpl/PreviewPlot.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/MplCpp/ColorConverter.h"
#include "MantidQtWidgets/MplCpp/MantidAxes.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QMenu>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

using Mantid::API::AnalysisDataService;
using Mantid::API::MatrixWorkspace;
using MantidQt::Widgets::MplCpp::Artist;
using MantidQt::Widgets::MplCpp::ColorConverter;
using MantidQt::Widgets::MplCpp::Figure;
using MantidQt::Widgets::MplCpp::FigureCanvasQt;
using MantidQt::Widgets::MplCpp::Line2D;
using MantidQt::Widgets::MplCpp::MantidAxes;
namespace Python = MantidQt::Widgets::Common::Python;

namespace {
Mantid::Kernel::Logger g_log("PreviewPlot");
constexpr auto DRAGGABLE_LEGEND = true;
constexpr auto PLOT_TOOL_NONE = "None";
constexpr auto PLOT_TOOL_PAN = "Pan";
constexpr auto PLOT_TOOL_ZOOM = "Zoom";
constexpr auto LINEAR_SCALE = "Linear";
constexpr auto LOG_SCALE = "Log";
constexpr auto SQUARE_SCALE = "Square";
constexpr auto SHOWALLERRORS = "Show all errors";
constexpr auto HIDEALLERRORS = "Hide all errors";
} // namespace

namespace MantidQt {

namespace MantidWidgets {

/**
 * Construct a plot object
 * @param parent The parent widget
 * @param watchADS If true then ADS observers are added
 */
PreviewPlot::PreviewPlot(QWidget *parent, bool observeADS)
    : PreviewPlotBase(parent),
      m_wsRemovedObserver(*this, &PreviewPlot::onWorkspaceRemoved),
      m_wsReplacedObserver(*this, &PreviewPlot::onWorkspaceReplaced), m_axis("both"), m_style("sci"), m_useOffset(true),
      m_xAxisScale("linear"), m_yAxisScale("linear") {
  createLayout();
  createActions();

  m_selectorActive = false;

  watchADS(observeADS);
}

/**
 * Destructor.
 * Removes ADS observers
 */
PreviewPlot::~PreviewPlot() { watchADS(false); }

/**
 * Enable/disable the ADS observers
 * @param on If true ADS observers are enabled else they are disabled
 */
void PreviewPlot::watchADS(bool on) {
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
 * Add a line for a given spectrum to the plot
 * @param lineName A string label for the line
 * @param ws A MatrixWorkspace that contains the data
 * @param wsIndex The index of the workspace to access
 * @param lineColour Defines the color of the line
 */
void PreviewPlot::addSpectrum(const QString &lineName, const Mantid::API::MatrixWorkspace_sptr &ws,
                              const size_t wsIndex, const QColor &lineColour,
                              const QHash<QString, QVariant> &plotKwargs) {
  if (lineName.isEmpty()) {
    g_log.warning("Cannot plot with empty line name");
    return;
  }
  if (!ws) {
    g_log.warning("Cannot plot null workspace");
    return;
  }
  removeSpectrum(lineName);

  auto axes = m_canvas->gca<MantidAxes>();
  axes.setXScale(m_xAxisScale.c_str());
  axes.setYScale(m_yAxisScale.c_str());
  if (m_linesErrorsCache.value(lineName)) {
    m_lines[lineName] = true;
    axes.errorbar(ws, wsIndex, lineColour.name(QColor::HexRgb), lineName, plotKwargs);
  } else {
    m_lines[lineName] = false;
    axes.plot(ws, wsIndex, lineColour.name(QColor::HexRgb), lineName, plotKwargs);
  }

  // Add line to stored line data
  auto plotCurveConfig =
      QSharedPointer<PlotCurveConfiguration>(new PlotCurveConfiguration(ws, lineName, wsIndex, lineColour, plotKwargs));
  m_plottedLines.insert(lineName, plotCurveConfig);
  if (auto const xLabel = overrideAxisLabel(AxisID::XBottom))
    setAxisLabel(AxisID::XBottom, xLabel.get());
  if (auto const yLabel = overrideAxisLabel(AxisID::YLeft))
    setAxisLabel(AxisID::YLeft, yLabel.get());

  regenerateLegend();
  axes.relim();

  replot();
}

/**
 * Add a line for a given spectrum to the plot
 * @param lineName A string label for the line
 * @param wsName A name of a MatrixWorkspace that contains the data
 * @param wsIndex The index of the workspace to access
 * @param lineColour Defines the color of the line
 */
void PreviewPlot::addSpectrum(const QString &lineName, const QString &wsName, const size_t wsIndex,
                              const QColor &lineColour, const QHash<QString, QVariant> &plotKwargs) {
  addSpectrum(lineName, AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName.toStdString()), wsIndex,
              lineColour, plotKwargs);
}

/**
 * Remove the named line from the plot
 * @param lineName A name of a given line on the plot. If the lineName is
 * not known then this does nothing
 */
void PreviewPlot::removeSpectrum(const QString &lineName) {
  auto axes = m_canvas->gca();
  axes.removeArtists("lines", lineName);
  m_lines.remove(lineName);
  regenerateLegend();
}

/**
 * Sets tight layout properties of the plot
 * @param args A hash of tight layout properties ("pad", "w_pad", "h_pad",
 * "rect")
 */
void PreviewPlot::setTightLayout(QHash<QString, QVariant> const &args) { m_canvas->setTightLayout(args); }

/**
 * Sets an override label for an axis.
 * @param axisID The axis ID (XBottom or YLeft).
 * @param label The override label.
 */
void PreviewPlot::setOverrideAxisLabel(AxisID const &axisID, char const *const label) { m_axisLabels[axisID] = label; }

/**
 * Returns the override label.
 * @param axisID The axis ID (XBottom or YLeft).
 * @return True if the axis should display an axis label.
 */
boost::optional<char const *> PreviewPlot::overrideAxisLabel(AxisID const &axisID) {
  auto const iter = m_axisLabels.find(axisID);
  if (iter != m_axisLabels.end())
    return iter.value();
  return boost::none;
}

/**
 * Sets the axis label on an axis.
 * @param axisID The axis ID (XBottom or YLeft).
 * @param label The label to place on a plots axis.
 */
void PreviewPlot::setAxisLabel(AxisID const &axisID, char const *const label) {
  switch (axisID) {
  case AxisID::XBottom:
    m_canvas->gca().setXLabel(label);
    return;
  case AxisID::YLeft:
    m_canvas->gca().setYLabel(label);
    return;
  }
  throw std::runtime_error("Incorrect AxisID provided. Axis types are XBottom and YLeft");
}

/**
 * Set the range of the specified axis
 * @param range The new range
 * @param axisID An enumeration defining the axis
 */
void PreviewPlot::setAxisRange(const QPair<double, double> &range, AxisID axisID) {
  switch (axisID) {
  case AxisID::XBottom:
    m_canvas->gca().setXLim(range.first, range.second);
    break;
  case AxisID::YLeft:
    m_canvas->gca().setYLim(range.first, range.second);
    break;
  }
}

void PreviewPlot::replot() {
  if (m_allowRedraws) {
    m_canvas->draw();
    emit redraw();
  }
}

// Called when the user turns errors on/off
void PreviewPlot::replotData() {
  m_allowRedraws = false;
  clear();
  for (const auto &curveConfig : m_plottedLines) {
    addSpectrum(curveConfig->lineName, curveConfig->ws, curveConfig->wsIndex, curveConfig->lineColour,
                curveConfig->plotKwargs);
  }
  m_allowRedraws = true;
  replot();
}

void PreviewPlot::allowRedraws(bool state) { m_allowRedraws = state; }

/**
 * Clear all lines from the plot
 */
void PreviewPlot::clear() {
  m_canvas->gca().clear();
  m_lines.clear();
}

/**
 * Resize the X axis to encompass all of the data
 */
void PreviewPlot::resizeX() { m_canvas->gca().autoscaleView(true, false); }

/**
 * Set the face colour for the canvas
 * @param colour A new colour for the figure facecolor
 */
void PreviewPlot::setCanvasColour(const QColor &colour) { m_canvas->gcf().setFaceColor(std::move(colour)); }

/**
 * @brief PreviewPlot::setLinesWithErrors
 * @param labels A list of line labels for which error should be shown
 */
void PreviewPlot::setLinesWithErrors(const QStringList &labels) {
  for (const QString &label : labels) {
    m_linesErrorsCache[label] = true;
  }
}
/**
 * @brief PreviewPlot::setLinesWithoutErrors
 * @param labels A list of line labels for which error bars should not be shown
 */
void PreviewPlot::setLinesWithoutErrors(const QStringList &labels) {
  for (const QString &label : labels) {
    m_linesErrorsCache[label] = false;
  }
}

/**
 * Toggle for programatic legend visibility toggle
 * @param visible If True the legend is visible on the canvas
 */
void PreviewPlot::showLegend(bool visible) { m_contextLegend->setChecked(visible); }

/**
 * @return The current colour of the canvas
 */
QColor PreviewPlot::canvasColour() const { return m_canvas->gcf().faceColor(); }

/**
 * Display the context menu for the canvas
 */
void PreviewPlot::showContextMenu(QMouseEvent *evt) {
  QMenu contextMenu{this};
  auto plotTools = contextMenu.addMenu("Plot Tools");
  plotTools->addActions(m_contextPlotTools->actions());
  contextMenu.addAction(m_contextResetView);

  contextMenu.addSeparator();
  auto xscale = contextMenu.addMenu("X Scale");
  xscale->addActions(m_contextXScale->actions());
  auto yScale = contextMenu.addMenu("Y Scale");
  yScale->addActions(m_contextYScale->actions());

  // Create the error bars option
  contextMenu.addSeparator();
  auto errors = contextMenu.addMenu("Error Bars:");
  errors->addActions(m_contextErrorBars->actions());

  contextMenu.addSeparator();
  contextMenu.addAction(m_contextLegend);

  contextMenu.exec(evt->globalPos());
}

/**
 * Initialize the layout for the widget
 */
void PreviewPlot::createLayout() {
  auto plotLayout = new QVBoxLayout(this);
  plotLayout->setContentsMargins(0, 0, 0, 0);
  plotLayout->setSpacing(0);
  plotLayout->addWidget(m_canvas, 0, 0);
  setLayout(plotLayout);
}

/**
 * Create the menu actions items
 */
void PreviewPlot::createActions() {
  // Create an exclusive group of checkable actions with
  auto createExclusiveActionGroup = [this](const std::initializer_list<const char *> &names) {
    auto group = new QActionGroup(this);
    group->setExclusive(true);
    for (const auto &name : names) {
      auto action = group->addAction(name);
      action->setCheckable(true);
    }
    group->actions()[0]->setChecked(true);
    return group;
  };
  // plot tools
  m_contextPlotTools = createExclusiveActionGroup({PLOT_TOOL_NONE, PLOT_TOOL_PAN, PLOT_TOOL_ZOOM});
  connect(m_contextPlotTools, &QActionGroup::triggered, this, &PreviewPlot::switchPlotTool);
  m_contextResetView = new QAction("Reset Plot", this);
  connect(m_contextResetView, &QAction::triggered, this, &PreviewPlot::resetView);

  // scales
  m_contextXScale = createExclusiveActionGroup({LINEAR_SCALE, LOG_SCALE, SQUARE_SCALE});
  connect(m_contextXScale, &QActionGroup::triggered, this, &PreviewPlot::setXScaleType);
  m_contextYScale = createExclusiveActionGroup({LINEAR_SCALE, LOG_SCALE});
  connect(m_contextYScale, &QActionGroup::triggered, this, &PreviewPlot::setYScaleType);
  m_contextXScale->actions()[0]->setChecked(true);
  m_contextYScale->actions()[0]->setChecked(true);

  // Error bars
  m_contextErrorBars = createExclusiveActionGroup({SHOWALLERRORS, HIDEALLERRORS});
  connect(m_contextErrorBars, &QActionGroup::triggered, this, &PreviewPlot::setErrorBars);

  // legend
  m_contextLegend = new QAction("Legend", this);
  m_contextLegend->setCheckable(true);
  m_contextLegend->setChecked(true);
  connect(m_contextLegend, &QAction::toggled, this, &PreviewPlot::toggleLegend);
}

/**
 * @return True if the legend is visible, false otherwise
 */
bool PreviewPlot::legendIsVisible() const { return m_contextLegend->isChecked(); }

/**
 * @return True if the PreviewPlot has a line with the specified name
 */
bool PreviewPlot::hasCurve(const QString &lineName) const { return m_lines.contains(lineName); }

/**
 * @return A list of labels whose line have errors attached
 */
QStringList PreviewPlot::linesWithErrors() const {
  QStringList visibleErrorLabels;
  auto iterator = m_lines.constBegin();
  while (iterator != m_lines.constEnd()) {
    if (iterator.value())
      visibleErrorLabels.append(iterator.key());
    ++iterator;
  }
  return visibleErrorLabels;
}

/**
 * Observer method called when a workspace is removed from the ADS
 * @param nf A pointer to the notification object
 */
void PreviewPlot::onWorkspaceRemoved(Mantid::API::WorkspacePreDeleteNotification_ptr nf) {
  if (m_lines.isEmpty()) {
    return;
  }
  // Ignore non matrix workspaces
  if (auto ws = std::dynamic_pointer_cast<MatrixWorkspace>(nf->object())) {
    // the artist may have already been removed. ignore the event is that is the
    // case
    bool removed = false;
    try {
      removed = m_canvas->gca<MantidAxes>().removeWorkspaceArtists(ws);
    } catch (Mantid::PythonInterface::PythonException &) {
    }
    if (removed) {
      this->replot();
    }
  }
}

/**
 * Observer method called when a workspace is replaced in the ADS
 * @param nf A pointer to the notification object
 */
void PreviewPlot::onWorkspaceReplaced(Mantid::API::WorkspaceBeforeReplaceNotification_ptr nf) {
  if (m_lines.isEmpty()) {
    return;
  }
  // Ignore non matrix workspaces
  if (auto oldWS = std::dynamic_pointer_cast<MatrixWorkspace>(nf->oldObject())) {
    if (auto newWS = std::dynamic_pointer_cast<MatrixWorkspace>(nf->newObject())) {
      if (m_canvas->gca<MantidAxes>().replaceWorkspaceArtists(newWS)) {
        this->replot();
      }
    }
  }
}

/**
 * If the legend is visible regenerate it based on the current content
 */
void PreviewPlot::regenerateLegend() {
  if (legendIsVisible()) {
    if (!m_lines.isEmpty()) {
      m_canvas->gca().legend(DRAGGABLE_LEGEND);
    }
  }
}

/**
 * If the legend is visible remove it from the canvas
 */
void PreviewPlot::removeLegend() {
  auto legend = m_canvas->gca().legendInstance();
  if (!legend.pyobj().is_none()) {
    m_canvas->gca().legendInstance().remove();
  }
}

/**
 * Set the X scale based on the given QAction
 * @param selected The action that triggered the slot
 */
void PreviewPlot::setXScaleType(QAction *selected) { setScaleType(AxisID::XBottom, selected->text()); }

/**
 * Set the error bars based on the given QAction
 * @param selected The action that triggered the slot
 */
void PreviewPlot::setErrorBars(QAction *selected) {
  if (selected->text().toStdString() == SHOWALLERRORS) {
    setLinesWithErrors(m_lines.keys());
  } else {
    setLinesWithoutErrors(m_lines.keys());
  }
  replotData();
}

/**
 * Set the X scale based on the given QAction
 * @param selected The action that triggered the slot
 */
void PreviewPlot::setYScaleType(QAction *selected) { setScaleType(AxisID::YLeft, selected->text()); }

void PreviewPlot::setXScaleLog() { setScaleType(AxisID::XBottom, LOG_SCALE); }

void PreviewPlot::setXScaleSquare() {
  setScaleType(AxisID::XBottom, SQUARE_SCALE);
}

void PreviewPlot::setXScaleLinear() {
  setScaleType(AxisID::XBottom, LINEAR_SCALE);
}

void PreviewPlot::setYScaleLog() { setScaleType(AxisID::YLeft, LOG_SCALE); }

void PreviewPlot::setYScaleLinear() {
  setScaleType(AxisID::YLeft, LINEAR_SCALE);
}

void PreviewPlot::setScaleType(AxisID id, const QString &actionName) {
  auto scaleType = actionName.toLower().toLatin1();
  auto axes = m_canvas->gca();
  switch (id) {
  case AxisID::XBottom:
    axes.setXScale(scaleType.constData());
    m_xAxisScale = actionName.toLower().toStdString();
    break;
  case AxisID::YLeft:
    axes.setYScale(scaleType.constData());
    m_yAxisScale = actionName.toLower().toStdString();
    break;
  default:
    break;
  }

  tickLabelFormat(m_axis, m_style, m_useOffset);

  this->replot();
}

/**
 * Toggle the legend visibility state
 * @param checked True if the state should be visible, false otherwise
 */
void PreviewPlot::toggleLegend(const bool checked) {
  if (m_lines.isEmpty()) {
    return;
  }
  if (checked) {
    regenerateLegend();
  } else {
    removeLegend();
  }
  this->replot();
}

/**
 * Format tick labels for linear scale
 * @param char* axis :: [ 'x' | 'y' | 'both' ]
 * @param char* style :: [ 'sci' (or 'scientific') | 'plain' ] plain turns off
 * scientific notation
 * @param bool useOffset :: True, the offset will be
 * calculated as needed, False no offset will be used
 */
void PreviewPlot::tickLabelFormat(char *axis, char *style, bool useOffset) {
  auto axes = m_canvas->gca();
  const auto formatXTicks = *axis != 'y' && axes.getXScale().toStdString() == "linear";
  const auto formatYTicks = *axis != 'x' && axes.getYScale().toStdString() == "linear";

  if (formatXTicks)
    axes.tickLabelFormat(std::string("x").c_str(), style, useOffset);

  if (formatYTicks)
    axes.tickLabelFormat(std::string("y").c_str(), style, useOffset);

  if (formatXTicks || formatYTicks) {
    // Need to save parameters to re-format on scale change
    m_axis = axis;
    m_style = style;
    m_useOffset = useOffset;
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
