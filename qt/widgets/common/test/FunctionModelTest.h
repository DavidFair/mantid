// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidQtWidgets/Common/FunctionModel.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

class FunctionModelTest : public CxxTest::TestSuite {

public:
  static FunctionModelTest *createSuite() { return new FunctionModelTest; }
  static void destroySuite(FunctionModelTest *suite) { delete suite; }

  FunctionModelTest() {
    // To make sure API is initialized properly
    FrameworkManager::Instance();
  }

  void test_empty() {
    FunctionModel model;
    TS_ASSERT(!model.getFitFunction());
  }

  void test_simple() {
    FunctionModel model;
    model.setFunctionString("name=LinearBackground,A0=1,A1=2");
    auto fun = model.getFitFunction();
    TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
    TS_ASSERT_EQUALS(fun->getParameter("A0"), 1.0);
    TS_ASSERT_EQUALS(fun->getParameter("A1"), 2.0);
  }

  void test_simple_multidomain() {
    FunctionModel model;
    model.setFunctionString("name=LinearBackground,A0=1,A1=2");
    model.setNumberDomains(2);
    TS_ASSERT_EQUALS(model.getNumberDomains(), 2);
    TS_ASSERT_EQUALS(model.currentDomainIndex(), 0);
    model.setCurrentDomainIndex(1);
    TS_ASSERT_EQUALS(model.currentDomainIndex(), 1);
    TS_ASSERT_THROWS_EQUALS(model.setCurrentDomainIndex(2),
                            std::runtime_error & e, std::string(e.what()),
                            "Domain index is out of range: 2 out of 2");
    {
      auto fun = model.getCurrentFunction();
      TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
      TS_ASSERT_EQUALS(fun->getParameter("A0"), 1.0);
      TS_ASSERT_EQUALS(fun->getParameter("A1"), 2.0);
    }
    {
      auto fun = model.getSingleFunction(0);
      TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
      TS_ASSERT_EQUALS(fun->getParameter("A0"), 1.0);
      TS_ASSERT_EQUALS(fun->getParameter("A1"), 2.0);
    }
    {
      auto fun = model.getSingleFunction(1);
      TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
      TS_ASSERT_EQUALS(fun->getParameter("A0"), 1.0);
      TS_ASSERT_EQUALS(fun->getParameter("A1"), 2.0);
    }
    TS_ASSERT_THROWS_EQUALS(model.getSingleFunction(2), std::runtime_error & e,
                            std::string(e.what()),
                            "Domain index is out of range: 2 out of 2");
    {
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->name(), "MultiDomainFunction");
      TS_ASSERT_EQUALS(fun->getParameter("f0.A0"), 1.0);
      TS_ASSERT_EQUALS(fun->getParameter("f0.A1"), 2.0);
      TS_ASSERT_EQUALS(fun->getParameter("f1.A0"), 1.0);
      TS_ASSERT_EQUALS(fun->getParameter("f1.A1"), 2.0);
    }
  }

  void test_function_resolution_from_workspace() {
    FunctionModel model;
    auto algo = AlgorithmManager::Instance().create("Load");
    algo->setPropertyValue("Filename", "iris26173_graphite002_res");
    algo->setPropertyValue("OutputWorkspace", "iris26173_graphite002_res");
    algo->execute();
    auto initialFunString =
        "composite=Convolution,NumDeriv=true,FixResolution=true;name="
        "Resolution,"
        "Workspace=iris26173_graphite002_res,WorkspaceIndex=0,X=(),Y=();name="
        "Lorentzian,Amplitude=1,PeakCentre=0,FWHM=1,constraints=(0<Amplitude,0<"
        "FWHM)";
    auto correctedFunString =
        "composite=Convolution,NumDeriv=true,FixResolution=true;name="
        "Resolution,"
        "Workspace=iris26173_graphite002_res,WorkspaceIndex=0,X=(),Y=();name="
        "Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0.0175,constraints=(0<Amplitude,0<"
        "FWHM)";
    model.setFunctionString(initialFunString);
    auto fun = model.getFitFunction();
    auto funString = fun->asString();
    TS_ASSERT(funString==correctedFunString);
  }

  void test_globals() {
    FunctionModel model;
    model.setFunctionString("name=LinearBackground,A0=1,A1=2");
    model.setNumberDomains(3);
    QStringList globals("A1");
    model.setGlobalParameters(globals);
    auto fun = model.getFitFunction();
    TS_ASSERT(!fun->getTie(1));
    TS_ASSERT_EQUALS(fun->getTie(3)->asString(), "f1.A1=f0.A1");
    TS_ASSERT_EQUALS(fun->getTie(5)->asString(), "f2.A1=f0.A1");
    auto locals = model.getLocalParameters();
    TS_ASSERT_EQUALS(locals[0], "A0");
    globals.clear();
    globals << "A0";
    model.setGlobalParameters(globals);
    fun = model.getFitFunction();
    TS_ASSERT(!fun->getTie(0));
    TS_ASSERT(!fun->getTie(1));
    TS_ASSERT(!fun->getTie(3));
    TS_ASSERT(!fun->getTie(5));
    TS_ASSERT_EQUALS(fun->getTie(2)->asString(), "f1.A0=f0.A0");
    TS_ASSERT_EQUALS(fun->getTie(4)->asString(), "f2.A0=f0.A0");
    locals = model.getLocalParameters();
    TS_ASSERT_EQUALS(locals[0], "A1");
  }

  void test_set_number_domains_after_clear() {
    FunctionModel model;
    model.clear();
    model.setNumberDomains(1);
    TS_ASSERT_EQUALS(model.getNumberDomains(), 1);
  }

  void test_add_function_top_level() {
    FunctionModel model;
    {
      model.addFunction("", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 2);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
    }
    {
      model.addFunction("", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;name=LinearBackground,A0=5,A1=6");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 4);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
    }
    {
      model.addFunction("", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;name=LinearBackground,A0=5,A1=6;"
          "name=LinearBackground,A0=7,A1=8");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 6);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
      TS_ASSERT_EQUALS(fun->getParameter(4), 7.0);
      TS_ASSERT_EQUALS(fun->getParameter(5), 8.0);
    }
  }

  void test_add_function_nested() {
    FunctionModel model;
    model.addFunction(
        "", "name=LinearBackground,A0=1,A1=2;(composite=CompositeFunction)");
    {
      model.addFunction("f1.", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;name=LinearBackground,A0=5,A1=6");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 4);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
    }
    {
      model.addFunction("f1.", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;"
          "(name=LinearBackground,A0=5,A1=6;name=LinearBackground,A0=7,A1=8)");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 6);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
      TS_ASSERT_EQUALS(fun->getParameter(4), 7.0);
      TS_ASSERT_EQUALS(fun->getParameter(5), 8.0);
    }
    {
      model.addFunction("f1.", "name=LinearBackground,A0=1,A1=2");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;"
          "(name=LinearBackground,A0=5,A1=6;name=LinearBackground,A0=7,A1=8;"
          "name=LinearBackground,A0=9,A1=10)");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 8);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
      TS_ASSERT_EQUALS(fun->getParameter(4), 7.0);
      TS_ASSERT_EQUALS(fun->getParameter(5), 8.0);
      TS_ASSERT_EQUALS(fun->getParameter(6), 9.0);
      TS_ASSERT_EQUALS(fun->getParameter(7), 10.0);
    }
  }

  void test_remove_function() {
    FunctionModel model;
    model.addFunction("", "name=LinearBackground,A0=1,A1=2;name="
                          "LinearBackground,A0=1,A1=2;name=LinearBackground,A0="
                          "1,A1=2");
    {
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;name=LinearBackground,A0=5,A1=6;"
          "name=LinearBackground,A0=7,A1=8");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 6);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
      TS_ASSERT_EQUALS(fun->getParameter(4), 7.0);
      TS_ASSERT_EQUALS(fun->getParameter(5), 8.0);
    }
    {
      model.removeFunction("f1.");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4;name=LinearBackground,A0=5,A1=6");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 4);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
      TS_ASSERT_EQUALS(fun->getParameter(2), 5.0);
      TS_ASSERT_EQUALS(fun->getParameter(3), 6.0);
    }
    {
      model.removeFunction("f1.");
      auto testFun = FunctionFactory::Instance().createInitialized(
          "name=LinearBackground,A0=3,A1=4");
      model.updateMultiDatasetParameters(*testFun);
      auto fun = model.getFitFunction();
      TS_ASSERT_EQUALS(fun->nParams(), 2);
      TS_ASSERT_EQUALS(fun->getParameter(0), 3.0);
      TS_ASSERT_EQUALS(fun->getParameter(1), 4.0);
    }
  }
  void test_getAttributeNames_returns_correctly() {
    FunctionModel model;
    model.addFunction("", "name = TeixeiraWaterSQE, Q = 3.14,"
                          "WorkspaceIndex = 4, Height = 1,"
                          "DiffCoeff=2.3, Tau=1.25, Centre=0, "
                          "constraints=(Height>0, DiffCoeff>0, "
                          "Tau>0);name=FlatBackground;name=LinearBackground");
    QStringList expectedAttributes = {QString("NumDeriv"), QString("f0.Q"),
                                      QString("f0.WorkspaceIndex")};

    auto attributes = model.getAttributeNames();

    TS_ASSERT_EQUALS(expectedAttributes.size(), attributes.size());

    for (int i = 0; i < attributes.size(); ++i) {
      TS_ASSERT_EQUALS(attributes[i], expectedAttributes[i]);
    }
  }
  void test_setAttribute_correctly_updates_stored_function() {
    FunctionModel model;
    model.addFunction("", "name = TeixeiraWaterSQE, Q = 3.14,"
                          "WorkspaceIndex = 4, Height = 1,"
                          "DiffCoeff=2.3, Tau=1.25, Centre=0, "
                          "constraints=(Height>0, DiffCoeff>0, "
                          "Tau>0);name=FlatBackground;name=LinearBackground");
    model.setAttribute("f0.Q", IFunction::Attribute(41.3));

    TS_ASSERT_EQUALS(
        model.getCurrentFunction()->getAttribute("f0.Q").asDouble(), 41.3);
  }
  void test_getAttribute_correctly_retrives_attributes() {
    FunctionModel model;
    model.addFunction("", "name=TeixeiraWaterSQE, Q = 3.14,"
                          "WorkspaceIndex = 4, Height = 1,"
                          "DiffCoeff=2.3, Tau=1.25, Centre=0, "
                          "constraints=(Height>0, DiffCoeff>0, "
                          "Tau>0);name=FlatBackground;name=LinearBackground");
    TS_ASSERT_EQUALS(model.getAttribute("f0.Q").asDouble(), 3.14);
    TS_ASSERT_EQUALS(model.getAttribute("NumDeriv").asBool(), false);
  }
  void test_getAttribute_throws_for_non_exisitng_attribute() {
    FunctionModel model;
    model.addFunction("", "name=TeixeiraWaterSQE, Q = 3.14,"
                          "WorkspaceIndex = 4, Height = 1,"
                          "DiffCoeff=2.3, Tau=1.25, Centre=0, "
                          "constraints=(Height>0, DiffCoeff>0, "
                          "Tau>0);name=FlatBackground;name=LinearBackground");
    TS_ASSERT_THROWS(model.getAttribute("f0.B").asDouble(),
                     std::invalid_argument &);
  }
  void test_updateMultiDatasetAttributes_correctly_updates_stored_attributes() {
    FunctionModel model;
    model.setNumberDomains(3);
    model.addFunction("", "name=TeixeiraWaterSQE, Q = 3.14,"
                          "WorkspaceIndex = 4, Height = 1,"
                          "DiffCoeff=2.3, Tau=1.25, Centre=0, "
                          "constraints=(Height>0, DiffCoeff>0, "
                          "Tau>0);name=FlatBackground;name=LinearBackground");
    auto function =
        FunctionFactory::Instance().createInitializedMultiDomainFunction(
            "name=TeixeiraWaterSQE, Q=41.3, "
            "Height=1, DiffCoeff=2.3, Tau=1.25, Centre=0, "
            "constraints=(Height>0, DiffCoeff>0, "
            "Tau>0);name=FlatBackground;name=LinearBackground",
            3);
    function->setAttribute("f0.f0.Q", IFunction::Attribute(11.3));
    function->setAttribute("f1.f0.Q", IFunction::Attribute(21.6));
    function->setAttribute("f2.f0.Q", IFunction::Attribute(32.9));

    model.updateMultiDatasetAttributes(*function);

    TS_ASSERT_EQUALS(model.getFitFunction()->getAttribute("f0.f0.Q").asDouble(),
                     11.3);
    TS_ASSERT_EQUALS(model.getFitFunction()->getAttribute("f1.f0.Q").asDouble(),
                     21.6);
    TS_ASSERT_EQUALS(model.getFitFunction()->getAttribute("f2.f0.Q").asDouble(),
                     32.9);
  }
};
