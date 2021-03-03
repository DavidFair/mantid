// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/MplCpp/Line2D.h"
#include "MantidQtWidgets/Plotting/DllOption.h"
#include "MantidQtWidgets/Plotting/Mpl/PreviewPlotBase.h"

#include <Poco/NObserver.h>

#include <QHash>
#include <QPair>
#include <QVariant>
#include <list>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {
class Axes;
class FigureCanvasQt;
} // namespace MplCpp
} // namespace Widgets
namespace MantidWidgets {

/**
 * Displays several workpaces on a matplotlib figure
 */
class EXPORT_OPT_MANTIDQT_PLOTTING PreviewPlot : public PreviewPlotBase {
  Q_OBJECT

  Q_PROPERTY(QColor canvasColour READ canvasColour WRITE setCanvasColour)
  Q_PROPERTY(bool showLegend READ legendIsVisible WRITE showLegend)
  Q_PROPERTY(QStringList curveErrorBars READ linesWithErrors WRITE setLinesWithErrors)

public:
  PreviewPlot(QWidget *parent = nullptr, bool observeADS = true);
  ~PreviewPlot();

  void watchADS(bool on);

  void setTightLayout(QHash<QString, QVariant> const &args);

  void addSpectrum(const QString &lineLabel, const Mantid::API::MatrixWorkspace_sptr &ws, const size_t wsIndex = 0,
                   const QColor &lineColour = QColor(),
                   const QHash<QString, QVariant> &plotKwargs = QHash<QString, QVariant>());
  void addSpectrum(const QString &lineName, const QString &wsName, const size_t wsIndex = 0,
                   const QColor &lineColour = QColor(),
                   const QHash<QString, QVariant> &plotKwargs = QHash<QString, QVariant>());
  void removeSpectrum(const QString &lineName);

  bool hasCurve(const QString &lineName) const;

  void setOverrideAxisLabel(AxisID const &axisID, char const *const label);
  void tickLabelFormat(char *axis, char *style, bool useOffset);

  void allowRedraws(bool state);
  void replotData();

  void setXScaleLog();
  void setXScaleSquare();
  void setXScaleLinear();
  void setYScaleLog();
  void setYScaleLinear();

public slots:
  void clear();
  void resizeX();
  void setCanvasColour(const QColor &colour);
  void setLinesWithErrors(const QStringList &labels);
  void setLinesWithoutErrors(const QStringList &labels);
  void showLegend(bool visible);
  void replot() override;

signals:
  void resetSelectorBounds();

public:
  QColor canvasColour() const;
  bool legendIsVisible() const;
  QStringList linesWithErrors() const;

private:
  void createActions() override;
  void showContextMenu(QMouseEvent *evt) override;

  void createLayout();

  void onWorkspaceRemoved(Mantid::API::WorkspacePreDeleteNotification_ptr nf);
  void onWorkspaceReplaced(Mantid::API::WorkspaceBeforeReplaceNotification_ptr nf);

  void regenerateLegend();
  void removeLegend();

  void setXScaleType(QAction *selected);
  void setYScaleType(QAction *selected);
  void setErrorBars(QAction *selected);
  void setScaleType(AxisID id, const QString &actionName);
  void toggleLegend(const bool checked);

  boost::optional<char const *> overrideAxisLabel(AxisID const &axisID);
  void setAxisLabel(AxisID const &axisID, char const *const label);

  // Block redrawing from taking place
  bool m_allowRedraws = true;

  // Curve configuration
  struct PlotCurveConfiguration {
    Mantid::API::MatrixWorkspace_sptr ws;
    QString lineName;
    size_t wsIndex;
    QColor lineColour;
    QHash<QString, QVariant> plotKwargs;

    PlotCurveConfiguration(Mantid::API::MatrixWorkspace_sptr ws, QString lineName, size_t wsIndex, QColor lineColour,
                           QHash<QString, QVariant> plotKwargs)
        : ws(ws), lineName(lineName), wsIndex(wsIndex), lineColour(lineColour), plotKwargs(plotKwargs){};
  };

  // Map a line label to the boolean indicating whether error bars are shown
  QHash<QString, bool> m_lines;
  // Map a line name to a plot configuration
  QMap<QString, QSharedPointer<PlotCurveConfiguration>> m_plottedLines;
  // Cache of line names which always have errors
  QHash<QString, bool> m_linesErrorsCache;
  // Map an axis to an override axis label
  QMap<AxisID, char const *> m_axisLabels;

  // Observers for ADS Notifications
  Poco::NObserver<PreviewPlot, Mantid::API::WorkspacePreDeleteNotification> m_wsRemovedObserver;
  Poco::NObserver<PreviewPlot, Mantid::API::WorkspaceBeforeReplaceNotification> m_wsReplacedObserver;

  // Tick label style
  char *m_axis;
  char *m_style;
  bool m_useOffset;

  // Axis scales
  std::string m_xAxisScale;
  std::string m_yAxisScale;

  // Context menu actions
  QActionGroup *m_contextXScale, *m_contextYScale;
  QAction *m_contextLegend;
  QActionGroup *m_contextErrorBars;
};

} // namespace MantidWidgets
} // namespace MantidQt
