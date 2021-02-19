// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Plotting/AxisID.h"
#include "MantidQtWidgets/Plotting/Mpl/RangeSelector.h"
#include "MantidQtWidgets/Plotting/Mpl/SingleSelector.h"

#include <QWidget>

namespace MantidQt::Widgets::MplCpp {
class FigureCanvasQt;
} // namespace MantidQt::Widgets::MplCpp

namespace MantidQt::MantidWidgets {

/**
 * Base class for common functionality between different types of preview plots
 */
class EXPORT_OPT_MANTIDQT_PLOTTING PreviewPlotBase : public QWidget {

public:
  PreviewPlotBase(QWidget *parent);
  virtual ~PreviewPlotBase() = default;

  Widgets::MplCpp::FigureCanvasQt *canvas() const;
  std::tuple<double, double>
  getAxisRange(AxisID axisID = AxisID::XBottom) const;
  QPointF toDataCoords(const QPoint &point) const;

  RangeSelector *
  addRangeSelector(const QString &name,
                   RangeSelector::SelectType type = RangeSelector::XMINMAX);
  RangeSelector *getRangeSelector(const QString &name) const;

  SingleSelector *
  addSingleSelector(const QString &name,
                    SingleSelector::SelectType type = SingleSelector::XSINGLE,
                    double position = 0.0);
  SingleSelector *getSingleSelector(const QString &name) const;

  void setSelectorActive(bool active);
  bool selectorActive() const;

  virtual void replot();

protected:
  // Canvas objects
  Widgets::MplCpp::FigureCanvasQt *m_canvas;
  // Range selector widgets
  QMap<QString, RangeSelector *> m_rangeSelectors;
  // Single selector's
  QMap<QString, SingleSelector *> m_singleSelectors;
  // Whether or not a selector is currently being moved
  bool m_selectorActive;
};
} // namespace MantidQt::MantidWidgets
