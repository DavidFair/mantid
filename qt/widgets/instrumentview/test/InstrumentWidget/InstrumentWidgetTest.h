// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IGLDisplay.h"
#include "IQtDisplay.h"
#include "InstrumentWidget.h"

#include "MockGLDisplay.h"
#include "MockInstrumentDisplay.h"
#include "MockProjectionSurface.h"
#include "MockQtConnect.h"
#include "MockQtDisplay.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <QObject>

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <memory>

using namespace MantidQt::MantidWidgets;
using namespace testing;
using Mantid::API::AnalysisDataService;
using Mantid::API::FrameworkManager;

class InstrumentWidgetTest : public CxxTest::TestSuite {
public:
  static InstrumentWidgetTest *createSuite() { return new InstrumentWidgetTest(); }
  static void destroySuite(InstrumentWidgetTest *suite) { delete suite; }

  using QtMock = StrictMock<MockQtDisplay>;
  using GLMock = StrictMock<MockGLDisplay>;
  using ConnectMock = StrictMock<MockQtConnect>;
  using DisplayMock = NiceMock<MockInstrumentDisplay>;

  void setUp() override {
    FrameworkManager::Instance();
    auto ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 2);
    AnalysisDataService::Instance().addOrReplace("test_ws", ws);
    m_glEnabledOriginal = Mantid::Kernel::ConfigService::Instance()
                              .getValue<bool>("MantidOptions.InstrumentView.UseOpenGL")
                              .get_value_or(true);
    setGl(true);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    setGl(m_glEnabledOriginal);
  }

  void test_constructor() {
    auto qtMock = makeQtDisplay();
    auto glMock = makeGL();
    auto instance = construct(makeDisplay(), qtMock.get(), glMock.get(), makeConnect());
  }

  void test_constructor_gl_disabled() {
    setGl(false);
    auto qtMock = makeQtDisplay();
    auto glMock = makeGL();
    auto instance = construct(makeDisplay(), qtMock.get(), glMock.get(), makeConnect());
  }

  void test_save_image_gl_enabled() {
    const auto inputName = QString::fromStdString("testFilename");
    const auto expectedName = inputName + ".png";

    auto qtMock = makeQtDisplay();
    auto glMock = makeGL();
    EXPECT_CALL(*glMock, saveToFile(expectedName)).Times(1);

    auto widget = construct(makeDisplay(), qtMock.get(), glMock.get(), makeConnect());
    widget.saveImage(inputName);
  }

  void test_save_image_gl_disabled() {
    setGl(false);
    const auto inputName = QString::fromStdString("testFilename");
    const auto expectedName = inputName + ".png";

    auto qtMock = makeQtDisplay();
    auto glMock = makeGL();
    EXPECT_CALL(*qtMock, saveToFile(expectedName)).Times(1);

    auto widget = construct(makeDisplay(), qtMock.get(), glMock.get(), makeConnect());
    widget.saveImage(inputName);
  }

  void test_update_instrument_detectors_gl_display_selected() {
    auto qtMock = makeQtDisplay();
    auto glMock = makeGL();
    auto displayMock = makeDisplay();
    EXPECT_CALL(*glMock, updateDetectors()).Times(1);
    EXPECT_CALL(*displayMock, currentWidget()).Times(1).WillOnce(Return(glMock.get()));

    auto widget = construct(std::move(displayMock), qtMock.get(), glMock.get(), makeConnect());
    widget.updateInstrumentDetectors();
  }

  void test_update_instrument_detectors_qt_display_selected() {
    auto qtMock = makeQtDisplay();
    auto glMock = makeGL();
    auto displayMock = makeDisplay();
    EXPECT_CALL(*qtMock, updateDetectors()).Times(1);
    EXPECT_CALL(*displayMock, currentWidget()).Times(1).WillOnce(Return(qtMock.get()));
    auto widget = construct(std::move(displayMock), qtMock.get(), glMock.get(), makeConnect());
    widget.updateInstrumentDetectors();
  }

  void test_update_instrument_detectors_gl_disabled() {
    // When GL is disabled, but somehow we still have a GL Display, we expect an update on that
    // This is likely a bug, but we are ensuring bug-compatibility
    setGl(false);
    auto qtMock = makeQtDisplay();
    auto glMock = makeGL();
    auto displayMock = makeDisplay();
    EXPECT_CALL(*glMock, updateDetectors()).Times(1);
    EXPECT_CALL(*displayMock, currentWidget()).Times(1).WillOnce(Return(glMock.get()));

    auto widget = construct(std::move(displayMock), qtMock.get(), glMock.get(), makeConnect());
    widget.updateInstrumentDetectors();
  }

  void test_update_instrument_detectors_gl_disabled_qt_display_selected() {
    setGl(false);
    auto qtMock = makeQtDisplay();
    auto glMock = makeGL();
    auto displayMock = makeDisplay();
    EXPECT_CALL(*qtMock, updateDetectors()).Times(1);
    EXPECT_CALL(*displayMock, currentWidget()).Times(1).WillOnce(Return(qtMock.get()));

    auto widget = construct(std::move(displayMock), qtMock.get(), glMock.get(), makeConnect());
    widget.updateInstrumentDetectors();
  }

  void test_update_instrument_view_gl_display_selected() {
    for (bool expected : {true, false}) {
      auto qtMock = makeQtDisplay();
      auto glMock = makeGL();
      auto displayMock = makeDisplay();
      EXPECT_CALL(*glMock, updateView(expected)).Times(1);
      EXPECT_CALL(*displayMock, currentWidget()).Times(1).WillOnce(Return(glMock.get()));
      auto widget = construct(std::move(displayMock), qtMock.get(), glMock.get(), makeConnect());
      widget.updateInstrumentView(expected);
    }
  }

  void test_update_instrument_view_qt_display_selected() {
    for (bool expected : {true, false}) {
      auto qtMock = makeQtDisplay();
      auto glMock = makeGL();
      auto displayMock = makeDisplay();
      EXPECT_CALL(*qtMock, updateView(expected)).Times(1);
      EXPECT_CALL(*displayMock, currentWidget()).Times(1).WillOnce(Return(qtMock.get()));
      auto widget = construct(std::move(displayMock), qtMock.get(), glMock.get(), makeConnect());
      widget.updateInstrumentView(expected);
    }
  }

  void test_update_instrument_view_gl_disabled() {
    // When GL is disabled, but somehow we still have a GL Display, we expect an update on that
    for (bool expected : {true, false}) {
      setGl(false);
      auto qtMock = makeQtDisplay();
      auto glMock = makeGL();
      auto displayMock = makeDisplay();
      EXPECT_CALL(*glMock, updateView(expected)).Times(1);
      EXPECT_CALL(*displayMock, currentWidget()).Times(1).WillOnce(Return(glMock.get()));

      auto widget = construct(std::move(displayMock), qtMock.get(), glMock.get(), makeConnect());
      widget.updateInstrumentView(expected);
    }
  }

  void test_update_instrument_view_gl_disabled_qt_display_selected() {
    for (bool expected : {true, false}) {
      setGl(false);
      auto qtMock = makeQtDisplay();
      auto glMock = makeGL();
      auto displayMock = makeDisplay();
      EXPECT_CALL(*qtMock, updateView(expected)).Times(1);
      EXPECT_CALL(*displayMock, currentWidget()).Times(1).WillOnce(Return(qtMock.get()));

      auto widget = construct(std::move(displayMock), qtMock.get(), glMock.get(), makeConnect());
      widget.updateInstrumentView(expected);
    }
  }

private:
  bool m_glEnabledOriginal = true;
  bool m_glEnabled = true;

  std::unique_ptr<QtMock> makeQtDisplay() const { return std::make_unique<QtMock>(); }
  std::unique_ptr<GLMock> makeGL() const { return std::make_unique<GLMock>(); }
  std::unique_ptr<DisplayMock> makeDisplay() const { return std::make_unique<DisplayMock>(); }

  void setGl(bool state) {
    m_glEnabled = state;
    auto const stateStr = state ? "On" : "Off";
    Mantid::Kernel::ConfigService::Instance().setString("MantidOptions.InstrumentView.UseOpenGL", stateStr);
  }

  void mockConnect(MockQtConnect &mock, const char *signal, const char *slot) const {
    EXPECT_CALL(mock, connect(_, StrEq(signal), _, StrEq(slot))).Times(1);
  }

  std::unique_ptr<ConnectMock> makeConnect() const {
    auto mock = std::make_unique<ConnectMock>();
    mockConnect(*mock, SIGNAL(enableLighting(bool)), SLOT(enableLighting(bool)));

    mockConnect(*mock, SIGNAL(changed(double, double)), SLOT(setIntegrationRange(double, double)));
    mockConnect(*mock, SIGNAL(clicked()), SLOT(helpClicked()));
    mockConnect(*mock, SIGNAL(setAutoscaling(bool)), SLOT(setColorMapAutoscaling(bool)));
    mockConnect(*mock, SIGNAL(rescaleColorMap()), SLOT(setupColorMap()));
    mockConnect(*mock, SIGNAL(executeAlgorithm(const QString &, const QString &)),
                SLOT(executeAlgorithm(const QString &, const QString &)));
    mockConnect(*mock, SIGNAL(changed(double, double)), SLOT(changedIntegrationRange(double, double)));
    mockConnect(*mock, SIGNAL(currentChanged(int)), SLOT(tabChanged(int)));
    mockConnect(*mock, SIGNAL(triggered()), SLOT(clearPeakOverlays()));
    mockConnect(*mock, SIGNAL(triggered()), SLOT(clearAlignmentPlane()));

    EXPECT_CALL(*mock, connect(_, StrEq(SIGNAL(needSetIntegrationRange(double, double))), _,
                               StrEq(SLOT(setIntegrationRange(double, double))), Qt::QueuedConnection))
        .Times(1);
    mockConnect(*mock, SIGNAL(executeAlgorithm(Mantid::API::IAlgorithm_sptr)),
                SLOT(executeAlgorithm(Mantid::API::IAlgorithm_sptr)));

    EXPECT_CALL(*mock,
                connect(_, StrEq(SIGNAL(updateInfoText())), _, StrEq(SLOT(updateInfoText())), Qt::QueuedConnection))
        .Times(1);
    return mock;
  }

  InstrumentWidget construct(std::unique_ptr<DisplayMock> displayMock, QtMock *qtMock, GLMock *glMock,
                             std::unique_ptr<ConnectMock> connectMock) const {
    ON_CALL(*displayMock, getGLDisplay()).WillByDefault(Return(glMock));
    ON_CALL(*displayMock, getQtDisplay()).WillByDefault(Return(qtMock));

    auto surfaceMock = std::make_shared<MockProjectionSurface>();
    EXPECT_CALL(*qtMock, setSurface(_)).Times(1);
    EXPECT_CALL(*qtMock, qtUpdate()).Times(1);
    EXPECT_CALL(*glMock, setBackgroundColor(_)).Times(1);

    const int getSurfaceCalls = m_glEnabled ? 22 : 24;

    EXPECT_CALL(*glMock, getSurface()).Times(getSurfaceCalls).WillRepeatedly(Return(surfaceMock));
    EXPECT_CALL(*glMock, setSurface(_)).Times(1);
    EXPECT_CALL(*glMock, currentBackgroundColor()).Times(1);

    EXPECT_CALL(*displayMock, installEventFilter(NotNull())).Times(1);

    InstrumentWidget::Dependencies deps{std::move(displayMock), nullptr, nullptr, std::move(connectMock)};

    return InstrumentWidget("test_ws", nullptr, true, true, 0.0, 0.0, true, std::move(deps));
  }
};
