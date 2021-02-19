// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Mpl/PreviewPlotBase.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"

#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QTimer>

using MantidQt::Widgets::MplCpp::FigureCanvasQt;

namespace {
Mantid::Kernel::Logger g_log("PreviewPlotBase");
constexpr auto MANTID_PROJECTION = "mantid";
constexpr auto PLOT_TOOL_NONE = "None";
constexpr auto PLOT_TOOL_PAN = "Pan";
constexpr auto PLOT_TOOL_ZOOM = "Zoom";
} // namespace

namespace MantidQt::MantidWidgets {

PreviewPlotBase::PreviewPlotBase(QWidget *parent)
    : QWidget(parent),
      m_canvas(new FigureCanvasQt(111, MANTID_PROJECTION, parent)),
      m_selectorActive{false}, m_panZoomTool(m_canvas) {
  createActions();
  m_canvas->installEventFilterToMplCanvas(this);
}

/**
 * Create the menu actions items
 */
void PreviewPlotBase::createActions() {
  // Create an exclusive group of checkable actions with
  auto createExclusiveActionGroup =
      [this](const std::initializer_list<const char *> &names) {
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
  m_contextPlotTools = createExclusiveActionGroup(
      {PLOT_TOOL_NONE, PLOT_TOOL_PAN, PLOT_TOOL_ZOOM});
  connect(m_contextPlotTools, &QActionGroup::triggered, this,
          &PreviewPlotBase::switchPlotTool);
  m_contextResetView = new QAction("Reset Plot", this);
  connect(m_contextResetView, &QAction::triggered, this,
          &PreviewPlotBase::resetView);
}

/**
 * Called when a different plot tool is selected. Enables the
 * appropriate mode on the canvas
 * @param selected A QAction pointer denoting the desired tool
 */
void PreviewPlotBase::switchPlotTool(QAction *selected) {
  QString toolName = selected->text();
  if (toolName == PLOT_TOOL_NONE) {
    m_panZoomTool.enableZoom(false);
    m_panZoomTool.enablePan(false);
    replot();
  } else if (toolName == PLOT_TOOL_PAN) {
    m_panZoomTool.enablePan(true);
    m_canvas->draw();
  } else if (toolName == PLOT_TOOL_ZOOM) {
    m_panZoomTool.enableZoom(true);
    m_canvas->draw();
  } else {
    // if a tool is added to the menu but no handler is added
    g_log.warning("Unknown plot tool selected.");
  }
}

/**
 * Reset the whole view to show all of the data
 */
void PreviewPlotBase::resetView() {
  m_panZoomTool.zoomOut();
  if (!m_panZoomTool.isPanEnabled() && !m_panZoomTool.isZoomEnabled())
    QTimer::singleShot(0, this, SLOT(replot()));
}

/**
 * Capture events destined for the canvas
 * @param watched Target object (Unused)
 * @param evt A pointer to the event object
 * @return True if the event should be stopped, false otherwise
 */
bool PreviewPlotBase::eventFilter(QObject *watched, QEvent *evt) {
  Q_UNUSED(watched);
  bool stopEvent{false};
  switch (evt->type()) {
  case QEvent::ContextMenu:
    // handled by mouse press events below as we need to
    // stop the canvas getting mouse events in some circumstances
    // to disable zooming/panning
    stopEvent = true;
    break;
  case QEvent::MouseButtonPress:
    stopEvent = handleMousePressEvent(static_cast<QMouseEvent *>(evt));
    break;
  case QEvent::MouseButtonRelease:
    stopEvent = handleMouseReleaseEvent(static_cast<QMouseEvent *>(evt));
    break;
  case QEvent::MouseMove:
    stopEvent = handleMouseMoveEvent(static_cast<QMouseEvent *>(evt));
    break;
  case QEvent::Resize:
    stopEvent = handleWindowResizeEvent();
    break;
  default:
    break;
  }
  return stopEvent;
}

/**
 * Display the context menu for the canvas
 */
void PreviewPlotBase::showContextMenu(QMouseEvent *evt) {
  QMenu contextMenu{this};
  auto plotTools = contextMenu.addMenu("Plot Tools");
  plotTools->addActions(m_contextPlotTools->actions());
  contextMenu.addAction(m_contextResetView);

  contextMenu.exec(evt->globalPos());
}

/**
 * Handler called when the event filter recieves a mouse press event
 * @param evt A pointer to the event
 * @return True if the event propagation should be stopped, false otherwise
 */
bool PreviewPlotBase::handleMousePressEvent(QMouseEvent *evt) {
  bool stopEvent(false);
  // right-click events are reserved for the context menu
  // show when the mouse click is released
  if (evt->buttons() & Qt::RightButton) {
    stopEvent = true;
  } else if (evt->buttons() & Qt::LeftButton) {
    const auto position = evt->pos();
    if (!position.isNull())
      emit mouseDown(position);
  }
  return stopEvent;
}

/**
 * Handler called when the event filter recieves a mouse release event
 * @param evt A pointer to the event
 * @return True if the event propagation should be stopped, false otherwise
 */
bool PreviewPlotBase::handleMouseReleaseEvent(QMouseEvent *evt) {
  bool stopEvent(false);
  if (evt->button() == Qt::RightButton) {
    stopEvent = true;
    showContextMenu(evt);
  } else if (evt->button() == Qt::LeftButton) {
    const auto position = evt->pos();
    if (!position.isNull())
      emit mouseUp(position);
    QTimer::singleShot(0, this, SLOT(replot()));
  }
  return stopEvent;
}

/**
 * Handler called when the event filter recieves a mouse move event
 * @param evt A pointer to the event
 * @return True if the event propagation should be stopped, false otherwise
 */
bool PreviewPlotBase::handleMouseMoveEvent(QMouseEvent *evt) {
  bool stopEvent(false);
  if (evt->buttons() == Qt::LeftButton) {
    const auto position = evt->pos();
    if (!position.isNull())
      emit mouseMove(position);
  }
  return stopEvent;
}

/**
 * Handler called when the event filter recieves a window resize event
 * @return True if the event propagation should be stopped, false otherwise
 */
bool PreviewPlotBase::handleWindowResizeEvent() {
  QTimer::singleShot(0, this, SLOT(replot()));
  return false;
}

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

} // namespace MantidQt::MantidWidgets
