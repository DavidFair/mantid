// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidCrystal/SCDCalibratePanels2.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCrystal/SCDCalibratePanels2ObjFunc.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Logger.h"
#include <boost/container/flat_set.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <boost/filesystem.hpp>
#include <boost/math/special_functions/round.hpp>
#include <cmath>
#include <fstream>
#include <iostream>

namespace Mantid {
namespace Crystal {

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

/// Config logger
namespace {
Logger logger("SCDCalibratePanels2");
}

DECLARE_ALGORITHM(SCDCalibratePanels2)

/**
 * @brief Initialization
 *
 */
void SCDCalibratePanels2::init() {
  // Input peakworkspace
  declareProperty(std::make_unique<WorkspaceProperty<IPeaksWorkspace>>("PeakWorkspace", "", Kernel::Direction::Input),
                  "Workspace of Indexed Peaks");

  // Lattice constant group
  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("RecalculateUB", true, "Recalculate UB matrix using given lattice constants");
  declareProperty("a", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter a (Leave empty to use lattice constants "
                  "in peaks workspace)");
  declareProperty("b", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter b (Leave empty to use lattice constants "
                  "in peaks workspace)");
  declareProperty("c", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter c (Leave empty to use lattice constants "
                  "in peaks workspace)");
  declareProperty("alpha", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter alpha in degrees (Leave empty to use "
                  "lattice constants in peaks workspace)");
  declareProperty("beta", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter beta in degrees (Leave empty to use "
                  "lattice constants in peaks workspace)");
  declareProperty("gamma", EMPTY_DBL(), mustBePositive,
                  "Lattice Parameter gamma in degrees (Leave empty to use "
                  "lattice constants in peaks workspace)");
  const std::string LATTICE("Lattice Constants");
  setPropertyGroup("RecalculateUB", LATTICE);
  setPropertyGroup("a", LATTICE);
  setPropertyGroup("b", LATTICE);
  setPropertyGroup("c", LATTICE);
  setPropertyGroup("alpha", LATTICE);
  setPropertyGroup("beta", LATTICE);
  setPropertyGroup("gamma", LATTICE);
  setPropertySettings("a", std::make_unique<EnabledWhenProperty>("RecalculateUB", IS_DEFAULT));
  setPropertySettings("b", std::make_unique<EnabledWhenProperty>("RecalculateUB", IS_DEFAULT));
  setPropertySettings("c", std::make_unique<EnabledWhenProperty>("RecalculateUB", IS_DEFAULT));
  setPropertySettings("alpha", std::make_unique<EnabledWhenProperty>("RecalculateUB", IS_DEFAULT));
  setPropertySettings("beta", std::make_unique<EnabledWhenProperty>("RecalculateUB", IS_DEFAULT));
  setPropertySettings("gamma", std::make_unique<EnabledWhenProperty>("RecalculateUB", IS_DEFAULT));

  // Calibration options group
  // NOTE:
  //  The general workflow of calibration is
  //  - calibrate L1 using all peaks
  //  - calibrate each bank
  //  - calibrate/update L1 again since bank movement will affect L1
  //  - calibrate T0
  //  - calibrate samplePos
  const std::string CALIBRATION("Calibration Options");
  // --------------
  // ----- L1 -----
  // --------------
  declareProperty("CalibrateL1", true, "Change the L1(source to sample) distance");
  declareProperty("ToleranceL1", 5e-4, mustBePositive, "Delta L1 below this value (in meter) is treated as 0.0");
  declareProperty("SearchRadiusL1", 0.1, mustBePositive,
                  "Search radius of delta L1 in meters, which is used to constrain optimization search space"
                  "when calibrating L1");
  // editability
  setPropertySettings("ToleranceL1", std::make_unique<EnabledWhenProperty>("CalibrateL1", IS_EQUAL_TO, "1"));
  setPropertySettings("SearchRadiusL1", std::make_unique<EnabledWhenProperty>("CalibrateL1", IS_EQUAL_TO, "1"));
  // grouping
  setPropertyGroup("CalibrateL1", CALIBRATION);
  setPropertyGroup("ToleranceL1", CALIBRATION);
  setPropertyGroup("SearchRadiusL1", CALIBRATION);
  // ----------------
  // ----- bank -----
  // ----------------
  declareProperty("CalibrateBanks", false, "Calibrate position and orientation of each bank.");
  declareProperty("ToleranceTransBank", 1e-6, mustBePositive,
                  "Delta translation of bank (in meter) below this value is treated as 0.0");
  declareProperty(
      "SearchRadiusTransBank", 5e-2, mustBePositive,
      "This is the search radius (in meter) when calibrating component translations, used to constrain optimization"
      "search space when calibration translation of banks");
  declareProperty("ToleranceRotBank", 1e-3, mustBePositive,
                  "Misorientation of bank (in deg) below this value is treated as 0.0");
  declareProperty("SearchradiusRotBank", 1.0, mustBePositive,
                  "This is the search radius (in deg) when calibrating component reorientation, used to constrain "
                  "optimization search space");
  // editability
  setPropertySettings("ToleranceTransBank", std::make_unique<EnabledWhenProperty>("CalibrateBanks", IS_EQUAL_TO, "1"));
  setPropertySettings("SearchRadiusTransBank",
                      std::make_unique<EnabledWhenProperty>("CalibrateBanks", IS_EQUAL_TO, "1"));
  setPropertySettings("ToleranceRotBank", std::make_unique<EnabledWhenProperty>("CalibrateBanks", IS_EQUAL_TO, "1"));
  setPropertySettings("SearchradiusRotBank", std::make_unique<EnabledWhenProperty>("CalibrateBanks", IS_EQUAL_TO, "1"));
  // grouping
  setPropertyGroup("CalibrateBanks", CALIBRATION);
  setPropertyGroup("ToleranceTransBank", CALIBRATION);
  setPropertyGroup("SearchRadiusTransBank", CALIBRATION);
  setPropertyGroup("ToleranceRotBank", CALIBRATION);
  setPropertyGroup("SearchradiusRotBank", CALIBRATION);
  // --------------
  // ----- T0 -----
  // --------------
  declareProperty("CalibrateT0", false, "Calibrate the T0 (initial TOF)");
  declareProperty("ToleranceT0", 1e-3, mustBePositive,
                  "Shift of initial TOF (in ms) below this value is treated as 0.0");
  declareProperty("SearchRadiusT0", 10.0, mustBePositive,
                  "Search radius of T0 (in ms), used to constrain optimization search space");
  // editability
  setPropertySettings("ToleranceT0", std::make_unique<EnabledWhenProperty>("CalibrateT0", IS_EQUAL_TO, "1"));
  setPropertySettings("SearchRadiusT0", std::make_unique<EnabledWhenProperty>("CalibrateT0", IS_EQUAL_TO, "1"));
  // grouping
  setPropertyGroup("CalibrateT0", CALIBRATION);
  setPropertyGroup("ToleranceT0", CALIBRATION);
  setPropertyGroup("SearchRadiusT0", CALIBRATION);
  // ---------------------
  // ----- samplePos -----
  // ---------------------
  declareProperty("TuneSamplePosition", false, "Fine tunning sample position");
  declareProperty("ToleranceSamplePos", 1e-6, mustBePositive,
                  "Sample position change (in meter) below this value is treated as 0.0");
  declareProperty("SearchRadiusSamplePos", 0.1, mustBePositive,
                  "Search radius of sample position change (in meters), used to constrain optimization search space");
  // editability
  setPropertySettings("ToleranceSamplePos",
                      std::make_unique<EnabledWhenProperty>("TuneSamplePosition", IS_EQUAL_TO, "1"));
  setPropertySettings("SearchRadiusSamplePos",
                      std::make_unique<EnabledWhenProperty>("TuneSamplePosition", IS_EQUAL_TO, "1"));
  // grouping
  setPropertyGroup("TuneSamplePosition", CALIBRATION);
  setPropertyGroup("ToleranceSamplePos", CALIBRATION);
  setPropertyGroup("SearchRadiusSamplePos", CALIBRATION);

  // Output options group
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The workspace containing the calibration table.");
  const std::vector<std::string> detcalExts{".DetCal", ".Det_Cal"};
  declareProperty(
      std::make_unique<FileProperty>("DetCalFilename", "SCDCalibrate2.DetCal", FileProperty::OptionalSave, detcalExts),
      "Path to an ISAW-style .detcal file to save.");
  declareProperty(
      std::make_unique<FileProperty>("XmlFilename", "SCDCalibrate2.xml", FileProperty::OptionalSave, ".xml"),
      "Path to an Mantid .xml description(for LoadParameterFile) file to "
      "save.");
  declareProperty(
      std::make_unique<FileProperty>("CSVFilename", "SCDCalibrate2.csv", FileProperty::OptionalSave, ".csv"),
      "Path to an .csv file which contains the Calibration Table");
  // group into Output group
  const std::string OUTPUT("Output");
  setPropertyGroup("OutputWorkspace", OUTPUT);
  setPropertyGroup("DetCalFilename", OUTPUT);
  setPropertyGroup("XmlFilename", OUTPUT);
  setPropertyGroup("CSVFilename", OUTPUT);

  // Add new section for advanced control of the calibration/optimization
  // NOTE: profiling is expensive, think twice before start
  declareProperty("VerboseOutput", false, "Toggle of child algorithm console output.");
  declareProperty("ProfileL1", false, "Perform profiling of objective function with given input for L1");
  declareProperty("ProfileBanks", false, "Perform profiling of objective function with given input for Banks");
  declareProperty("ProfileT0", false, "Perform profiling of objective function with given input for T0");
  // grouping into one category
  const std::string ADVCNTRL("Advanced Option");
  setPropertyGroup("VerboseOutput", ADVCNTRL);
  setPropertyGroup("ProfileL1", ADVCNTRL);
  setPropertyGroup("ProfileBanks", ADVCNTRL);
  setPropertyGroup("ProfileT0", ADVCNTRL);
}

/**
 * @brief validate inputs
 *
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> SCDCalibratePanels2::validateInputs() {
  std::map<std::string, std::string> issues;

  // Lattice constants are required if no UB is attached to the input
  // peak workspace
  IPeaksWorkspace_sptr pws = getProperty("PeakWorkspace");
  double a = getProperty("a");
  double b = getProperty("b");
  double c = getProperty("c");
  double alpha = getProperty("alpha");
  double beta = getProperty("beta");
  double gamma = getProperty("gamma");
  if ((a == EMPTY_DBL() || b == EMPTY_DBL() || c == EMPTY_DBL() || alpha == EMPTY_DBL() || beta == EMPTY_DBL() ||
       gamma == EMPTY_DBL()) &&
      (!pws->sample().hasOrientedLattice())) {
    issues["RecalculateUB"] = "Lattice constants are needed for peak "
                              "workspace without a UB mattrix";
  }

  return issues;
}

/**
 * @brief execute calibration
 *
 */
void SCDCalibratePanels2::exec() {
  // parse all inputs
  IPeaksWorkspace_sptr m_pws = getProperty("PeakWorkspace");

  // recalculate UB with given lattice constant
  // if required
  if (getProperty("RecalculateUB")) {
    // parse lattice constants
    parseLatticeConstant(m_pws);

    // recalculate UB and index peaks
    updateUBMatrix(m_pws);
  }

  // remove unindexed peaks
  m_pws = removeUnindexedPeaks(m_pws);

  bool calibrateT0 = getProperty("CalibrateT0");
  bool calibrateL1 = getProperty("CalibrateL1");
  bool calibrateBanks = getProperty("CalibrateBanks");

  bool profL1 = getProperty("ProfileL1");
  bool profBanks = getProperty("ProfileBanks");
  bool profT0 = getProperty("ProfileT0");

  const std::string DetCalFilename = getProperty("DetCalFilename");
  const std::string XmlFilename = getProperty("XmlFilename");
  const std::string CSVFilename = getProperty("CSVFilename");

  LOGCHILDALG = getProperty("VerboseOutput");

  // STEP_0: sort the peaks
  std::vector<std::pair<std::string, bool>> criteria{{"BankName", true}};
  m_pws->sort(criteria);
  // need to keep a copy of the peak workspace at its input state
  IPeaksWorkspace_sptr pws_original = m_pws->clone();

  // STEP_2: preparation
  // get names of banks that can be calibrated
  getBankNames(m_pws);

  // DEV ONLY
  if (profL1) {
    profileL1(m_pws, pws_original);
  }
  if (profBanks) {
    profileBanks(m_pws, pws_original);
  }
  if (profT0) {
    profileT0(m_pws, pws_original);
  }

  // STEP_3: optimize
  //  - L1 (with or without T0 cali attached)
  //  - Banks
  //  - sample position
  if (calibrateL1) {
    // NOTE:
    //    L1 and T0 can be calibrated together to provide stable calibration results.
    g_log.notice() << "** Calibrating L1 (moderator) as requested\n";
    optimizeL1(m_pws, pws_original);
  }

  if (calibrateBanks) {
    g_log.notice() << "** Calibrating L2 and orientation (bank) as requested\n";
    optimizeBanks(m_pws, pws_original);
  }

  if (calibrateL1 && calibrateBanks) {
    g_log.notice() << "** Calibrating L1 (moderator) after bank adjusted\n";
    optimizeL1(m_pws, pws_original);
    // NOTE:
    //    Turns out 1 pass is sufficient (tested with the following block)
    //
    // double delta = 1;
    // int cnt = 0;
    // while (delta > 0.01) {
    //   double L1_pre = m_pws->getInstrument()->getSource()->getPos().Z();
    //   optimizeBanks(m_pws, pws_original);
    //   optimizeL1(m_pws, pws_original);
    //   double L1_post = m_pws->getInstrument()->getSource()->getPos().Z();
    //   delta = std::abs((L1_pre - L1_post) / L1_pre);
    //   cnt += 1;
    //   g_log.notice() << "@pass_" << cnt << "\n" << L1_pre << "-->" << L1_post << "\n";
    // }
  }

  if (calibrateT0 && !calibrateL1) {
    // NOTE:
    //    L1 and T0 can be calibrated together to provide a stable results, which is the
    //    recommended way.
    //    However, one can still calibrate T0 only if desired.
    g_log.notice() << "** Calibrating T0 only as requested\n";
    optimizeT0(m_pws, pws_original);
  }

  // STEP_4: generate a table workspace to save the calibration results
  g_log.notice() << "-- Generate calibration table\n";
  Instrument_sptr instCalibrated = std::const_pointer_cast<Geometry::Instrument>(m_pws->getInstrument());
  ITableWorkspace_sptr tablews = generateCalibrationTable(instCalibrated);

  // STEP_5: Write to disk if required
  if (!XmlFilename.empty())
    saveXmlFile(XmlFilename, m_BankNames, instCalibrated);

  if (!DetCalFilename.empty())
    saveIsawDetCal(DetCalFilename, m_BankNames, instCalibrated, m_T0);

  if (!CSVFilename.empty())
    saveCalibrationTable(CSVFilename, tablews);

  // STEP_4: Cleanup
}

/// ------------------------------------------- ///
/// Core functions for Calibration&Optimizatoin ///
/// ------------------------------------------- ///

/**
 * @brief
 *
 * @param pws
 * @param pws_original
 */
void SCDCalibratePanels2::optimizeL1(IPeaksWorkspace_sptr pws, IPeaksWorkspace_sptr pws_original) {
  // cache starting L1 position
  double original_L1 = std::abs(pws->getInstrument()->getSource()->getPos().Z());
  // T0 can be calibrate along with L1 to provide a more stable results
  bool caliT0 = getProperty("CalibrateT0");

  MatrixWorkspace_sptr l1ws = getIdealQSampleAsHistogram1D(pws);

  // fit algorithm for the optimization of L1
  IAlgorithm_sptr fitL1_alg = createChildAlgorithm("Fit", -1, -1, false);
  auto objf = std::make_shared<SCDCalibratePanels2ObjFunc>();
  // NOTE: always use the original pws to get the tofs
  std::vector<double> tofs = captureTOF(pws_original);
  objf->setPeakWorkspace(pws, "moderator", tofs);
  fitL1_alg->setProperty("Function", std::dynamic_pointer_cast<IFunction>(objf));

  //-- bounds&constraints def
  std::ostringstream tie_str;
  if (caliT0) {
    tie_str << "DeltaX=0.0,DeltaY=0.0,Theta=1.0,Phi=0.0,DeltaRotationAngle=0.0";
  } else {
    tie_str << "DeltaX=0.0,DeltaY=0.0,Theta=1.0,Phi=0.0,DeltaRotationAngle=0.0,DeltaT0=" << m_T0;
  }
  std::ostringstream constraint_str;
  double r_L1 = getProperty("SearchRadiusL1"); // get search radius
  r_L1 = std::abs(r_L1);
  constraint_str << -r_L1 << "<DeltaZ<" << r_L1;
  // throw in the constrain for T0 cali if needed
  if (caliT0) {
    double r_dT0 = getProperty("SearchRadiusT0");
    r_dT0 = std::abs(r_dT0);
    constraint_str << "," << -r_dT0 << "<DeltaT0<" << r_dT0;
  }
  //-- set and go
  fitL1_alg->setProperty("Ties", tie_str.str());
  fitL1_alg->setProperty("Constraints", constraint_str.str());
  fitL1_alg->setProperty("InputWorkspace", l1ws);
  fitL1_alg->setProperty("CreateOutput", true);
  fitL1_alg->setProperty("Output", "fit");
  fitL1_alg->executeAsChildAlg();

  //-- parse output
  std::ostringstream calilog;
  double chi2OverDOF = fitL1_alg->getProperty("OutputChi2overDoF");
  ITableWorkspace_sptr rst = fitL1_alg->getProperty("OutputParameters");
  // get results for L1
  double dL1_optimized = rst->getRef<double>("Value", 2);
  double tor_dL1 = getProperty("ToleranceL1");
  if (std::abs(dL1_optimized) < std::abs(tor_dL1)) {
    calilog << "-- Fit L1 results below tolerance, zero it\n";
    dL1_optimized = 0.0;
  }
  // get results for T0 (optional)
  double dT0_optimized = rst->getRef<double>("Value", 6);
  double tor_dT0 = getProperty("ToleranceT0");
  if (caliT0) {
    if (std::abs(dT0_optimized) < std::abs(tor_dT0)) {
      calilog << "-- Fit dT0 = " << dT0_optimized << " is below tolerance(" << tor_dT0 << "), zero it\n";
      dT0_optimized = 0.0;
    }
  }
  // apply the cali results (for output cali table and file)
  adjustComponent(0.0, 0.0, dL1_optimized, 1.0, 0.0, 0.0, 0.0, pws->getInstrument()->getSource()->getName(), pws);
  m_T0 = dT0_optimized;
  // logging
  int npks = pws->getNumberPeaks();
  calilog << "-- Fit L1 results using " << npks << " peaks:\n"
          << "    dL1: " << dL1_optimized << " \n"
          << "    L1 " << original_L1 << " -> " << -pws->getInstrument()->getSource()->getPos().Z() << " \n"
          << "    dT0 = " << m_T0 << " \n"
          << "    chi2/DOF = " << chi2OverDOF << "\n";
  g_log.notice() << calilog.str();
}

/**
 * @brief Calibrate the position and rotation of each Bank, one at a time
 *
 * @param pws
 * @param pws_original
 */
void SCDCalibratePanels2::optimizeBanks(IPeaksWorkspace_sptr pws, IPeaksWorkspace_sptr pws_original) {

  PARALLEL_FOR_IF(Kernel::threadSafe(*pws))
  for (int i = 0; i < static_cast<int>(m_BankNames.size()); ++i) {
    PARALLEL_START_INTERUPT_REGION
    // prepare local copies to work with
    const std::string bankname = *std::next(m_BankNames.begin(), i);
    const std::string pwsBankiName = "_pws_" + bankname;

    //-- step 0: extract peaks that lies on the current bank
    IPeaksWorkspace_sptr pwsBanki = selectPeaksByBankName(pws, bankname, pwsBankiName);
    //   get tofs from the original subset of pws
    IPeaksWorkspace_sptr pwsBanki_original = selectPeaksByBankName(pws_original, bankname, pwsBankiName);
    std::vector<double> tofs = captureTOF(pwsBanki_original);

    // Do not attempt correct panels with less than 6 peaks as the system will
    // be under-determined
    int nBankPeaks = pwsBanki->getNumberPeaks();
    if (nBankPeaks < MINIMUM_PEAKS_PER_BANK) {
      // use ostringstream to prevent OPENMP breaks log info
      std::ostringstream msg_npeakCheckFail;
      msg_npeakCheckFail << "-- Bank " << bankname << " have only " << nBankPeaks << " (<" << MINIMUM_PEAKS_PER_BANK
                         << ") Peaks, skipping\n";
      g_log.notice() << msg_npeakCheckFail.str();
      continue;
    }

    //-- step 1: prepare a mocked workspace with QSample as its yValues
    MatrixWorkspace_sptr wsBankCali = getIdealQSampleAsHistogram1D(pwsBanki);

    //-- step 2&3: invoke fit to find both traslation and rotation
    IAlgorithm_sptr fitBank_alg = createChildAlgorithm("Fit", -1, -1, false);
    //---- setup obj fun def
    auto objf = std::make_shared<SCDCalibratePanels2ObjFunc>();
    objf->setPeakWorkspace(pwsBanki, bankname, tofs);
    fitBank_alg->setProperty("Function", std::dynamic_pointer_cast<IFunction>(objf));

    //---- bounds&constraints def
    std::ostringstream tie_str;
    tie_str << "DeltaT0=" << m_T0;
    std::ostringstream constraint_str;
    double brb = getProperty("SearchradiusRotBank");
    brb = std::abs(brb);
    constraint_str << "0.0<Theta<3.1415926,0<Phi<6.28318530718," << -brb << "<DeltaRotationAngle<" << brb << ",";
    double btb = getProperty("SearchRadiusTransBank");
    btb = std::abs(btb);
    constraint_str << -btb << "<DeltaX<" << btb << "," << -btb << "<DeltaY<" << btb << "," << -btb << "<DeltaZ<" << btb;

    //---- set&go
    fitBank_alg->setProperty("Ties", tie_str.str());
    fitBank_alg->setProperty("Constraints", constraint_str.str());
    fitBank_alg->setProperty("InputWorkspace", wsBankCali);
    fitBank_alg->setProperty("CreateOutput", true);
    fitBank_alg->setProperty("Output", "fit");
    fitBank_alg->executeAsChildAlg();

    //---- cache results
    double chi2OverDOF = fitBank_alg->getProperty("OutputChi2overDoF");
    ITableWorkspace_sptr rstFitBank = fitBank_alg->getProperty("OutputParameters");
    double dx = rstFitBank->getRef<double>("Value", 0);
    double dy = rstFitBank->getRef<double>("Value", 1);
    double dz = rstFitBank->getRef<double>("Value", 2);
    double theta = rstFitBank->getRef<double>("Value", 3);
    double phi = rstFitBank->getRef<double>("Value", 4);
    double rotang = rstFitBank->getRef<double>("Value", 5);

    //-- step 4: update the instrument with optimization results
    //           if the fit results are above the tolerance/threshold
    std::string bn = bankname;
    std::ostringstream calilog;
    if (pws->getInstrument()->getName().compare("CORELLI") == 0)
      bn.append("/sixteenpack");
    double tolerance_translation = getProperty("ToleranceTransBank");
    tolerance_translation = std::abs(tolerance_translation);
    double tolerance_rotation = getProperty("ToleranceRotBank");
    tolerance_rotation = std::abs(tolerance_rotation);
    if ((std::abs(dx) < tolerance_translation) && (std::abs(dy) < tolerance_translation) &&
        (std::abs(dz) < tolerance_translation) && (std::abs(rotang) < tolerance_rotation)) {
      calilog << "-- Fit " << bn << " results below tolerance, zero all\n";
      dx = 0.0;
      dy = 0.0;
      dz = 0.0;
      rotang = 0.0;
    }
    double rvx = sin(theta) * cos(phi);
    double rvy = sin(theta) * sin(phi);
    double rvz = cos(theta);
    adjustComponent(dx, dy, dz, rvx, rvy, rvz, rotang, bn, pws);
    // logging
    calilog << "-- Fit " << bn << " results using " << nBankPeaks << " peaks:\n "
            << "    d(x,y,z) = (" << dx << "," << dy << "," << dz << ")\n"
            << "    rotang(rx,ry,rz) =" << rotang << "(" << rvx << "," << rvy << "," << rvz << ")\n"
            << "    chi2/DOF = " << chi2OverDOF << "\n";
    g_log.notice() << calilog.str();

    // -- cleanup
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

/**
 * @brief adjusting the deltaT0 to match the qSample_calculated and
 *        qSameple_measured
 *
 * @note this function currently only returns dT0=0, and the reason
 *       is still unkown.
 *
 * @param pws
 * @param pws_original
 */
void SCDCalibratePanels2::optimizeT0(IPeaksWorkspace_sptr pws, IPeaksWorkspace_sptr pws_original) {
  // create child Fit alg to optimize T0
  IAlgorithm_sptr fitT0_alg = createChildAlgorithm("Fit", -1, -1, false);
  //-- obj func def
  //  dl;dr;
  //    Fit algorithm requires a IFunction1D to fit
  //  details
  //    Fit algorithm requires a class derived from IFunction1D as its
  //    input, so we have to implement the objective function as a separate
  //    class just to get Fit serving as an optimizer.
  //    For this particular case, we are constructing an objective function
  //    based on IFunction1D that outputs a fake histogram consist of
  //    qSample calculated based on perturbed instrument positions and
  //    orientations.
  MatrixWorkspace_sptr t0ws = getIdealQSampleAsHistogram1D(pws);

  auto objf = std::make_shared<SCDCalibratePanels2ObjFunc>();
  // NOTE: always use the original pws to get the tofs
  std::vector<double> tofs = captureTOF(pws_original);
  objf->setPeakWorkspace(pws, "none", tofs);
  fitT0_alg->setProperty("Function", std::dynamic_pointer_cast<IFunction>(objf));

  //-- bounds&constraints def
  std::ostringstream tie_str;
  tie_str << "DeltaX=0.0,DeltaY=0.0,DeltaZ=0.0,Theta=0.0,Phi=0.0,DeltaRotationAngle=0.0";
  std::ostringstream constraint_str;
  double r_dT0 = getProperty("SearchRadiusT0");
  r_dT0 = std::abs(r_dT0);
  constraint_str << -r_dT0 << "<DeltaT0<" << r_dT0;

  //-- set&go
  fitT0_alg->setProperty("Ties", tie_str.str());
  fitT0_alg->setProperty("Constraints", constraint_str.str());
  fitT0_alg->setProperty("InputWorkspace", t0ws);
  fitT0_alg->setProperty("CreateOutput", true);
  fitT0_alg->setProperty("Output", "fit");
  fitT0_alg->executeAsChildAlg();

  //-- parse output
  double chi2OverDOF = fitT0_alg->getProperty("OutputChi2overDoF");
  ITableWorkspace_sptr rst = fitT0_alg->getProperty("OutputParameters");
  double dT0_optimized = rst->getRef<double>("Value", 6);
  double tor_dT0 = getProperty("ToleranceT0");
  std::ostringstream calilog;
  if (std::abs(dT0_optimized) < std::abs(tor_dT0)) {
    calilog << "-- Fit dT0 = " << dT0_optimized << " is below tolerance(" << tor_dT0 << "), zero it\n";
    dT0_optimized = 0.0;
  }

  // apply calibration results (for output file and caliTable)
  m_T0 = dT0_optimized;
  int npks = pws->getNumberPeaks();
  // logging
  calilog << "-- Fit T0 results using " << npks << " peaks:\n"
          << "    dT0 = " << m_T0 << " \n"
          << "    chi2/DOF = " << chi2OverDOF << "\n";
  g_log.notice() << calilog.str();
}

/// ---------------- ///
/// helper functions ///
/// ---------------- ///

/**
 * @brief get lattice constants from either inputs or the
 *        input peak workspace
 *
 */
void SCDCalibratePanels2::parseLatticeConstant(IPeaksWorkspace_sptr pws) {
  m_a = getProperty("a");
  m_b = getProperty("b");
  m_c = getProperty("c");
  m_alpha = getProperty("alpha");
  m_beta = getProperty("beta");
  m_gamma = getProperty("gamma");
  // if any one of the six lattice constants is missing, try to get
  // one from the workspace
  if ((m_a == EMPTY_DBL() || m_b == EMPTY_DBL() || m_c == EMPTY_DBL() || m_alpha == EMPTY_DBL() ||
       m_beta == EMPTY_DBL() || m_gamma == EMPTY_DBL()) &&
      (pws->sample().hasOrientedLattice())) {
    OrientedLattice lattice = pws->mutableSample().getOrientedLattice();
    m_a = lattice.a();
    m_b = lattice.b();
    m_c = lattice.c();
    m_alpha = lattice.alpha();
    m_beta = lattice.beta();
    m_gamma = lattice.gamma();
  }
}

/**
 * @brief update UB matrix embeded in the peakworkspace using lattice constants
 *        and redo the peak indexation afterwards
 *
 * @param pws
 */
void SCDCalibratePanels2::updateUBMatrix(IPeaksWorkspace_sptr pws) {
  IAlgorithm_sptr calcUB_alg = createChildAlgorithm("CalculateUMatrix", -1, -1, false);
  calcUB_alg->setLogging(LOGCHILDALG);
  calcUB_alg->setProperty("PeaksWorkspace", pws);
  calcUB_alg->setProperty("a", m_a);
  calcUB_alg->setProperty("b", m_b);
  calcUB_alg->setProperty("c", m_c);
  calcUB_alg->setProperty("alpha", m_alpha);
  calcUB_alg->setProperty("beta", m_beta);
  calcUB_alg->setProperty("gamma", m_gamma);
  calcUB_alg->executeAsChildAlg();

  // Since UB is updated, we need to redo the indexation
  IAlgorithm_sptr idxpks_alg = createChildAlgorithm("IndexPeaks", -1, -1, false);
  idxpks_alg->setLogging(LOGCHILDALG);
  idxpks_alg->setProperty("PeaksWorkspace", pws);
  idxpks_alg->setProperty("RoundHKLs", true); // both are using default
  idxpks_alg->setProperty("Tolerance", 0.15); // values
  idxpks_alg->executeAsChildAlg();
}

/**
 * @brief
 *
 * @param pws
 * @return IPeaksWorkspace_sptr
 */
IPeaksWorkspace_sptr SCDCalibratePanels2::removeUnindexedPeaks(Mantid::API::IPeaksWorkspace_sptr pws) {
  IAlgorithm_sptr fltpk_alg = createChildAlgorithm("FilterPeaks");
  fltpk_alg->setLogging(LOGCHILDALG);
  fltpk_alg->setProperty("InputWorkspace", pws);
  fltpk_alg->setProperty("FilterVariable", "h^2+k^2+l^2");
  fltpk_alg->setProperty("Operator", ">");
  fltpk_alg->setProperty("FilterValue", 0.0);
  fltpk_alg->setProperty("OutputWorkspace", "pws_filtered");
  fltpk_alg->executeAsChildAlg();

  IPeaksWorkspace_sptr outWS = fltpk_alg->getProperty("OutputWorkspace");
  IPeaksWorkspace_sptr ows = std::dynamic_pointer_cast<IPeaksWorkspace>(outWS);
  return outWS;
}

/**
 * @brief Capture TOFs that are equivalent to thoes measured from experiment.
 *        This step should be carried out after the indexation (if required)
 *
 * @param pws
 */
std::vector<double> SCDCalibratePanels2::captureTOF(Mantid::API::IPeaksWorkspace_sptr pws) {
  std::vector<double> tofs;

  for (int i = 0; i < pws->getNumberPeaks(); ++i) {
    tofs.emplace_back(pws->getPeak(i).getTOF());
  }

  return tofs;
}

/**
 * @brief Gather names for bank for calibration
 *
 * @param pws
 */
void SCDCalibratePanels2::getBankNames(IPeaksWorkspace_sptr pws) {
  auto peaksWorkspace = std::dynamic_pointer_cast<DataObjects::PeaksWorkspace>(pws);
  if (!peaksWorkspace)
    throw std::invalid_argument("a PeaksWorkspace is required to retrieve bank names");
  int npeaks = static_cast<int>(pws->getNumberPeaks());
  for (int i = 0; i < npeaks; ++i) {
    std::string bname = peaksWorkspace->getPeak(i).getBankName();
    if (bname != "None")
      m_BankNames.insert(bname);
  }
}

/**
 * @brief Select peaks with give bankname
 *
 * @param pws
 * @param bankname
 * @param outputwsn
 * @return DataObjects::PeaksWorkspace_sptr
 */
IPeaksWorkspace_sptr SCDCalibratePanels2::selectPeaksByBankName(IPeaksWorkspace_sptr pws, const std::string bankname,
                                                                const std::string outputwsn) {
  IAlgorithm_sptr fltpk_alg = createChildAlgorithm("FilterPeaks");
  fltpk_alg->setLogging(LOGCHILDALG);
  fltpk_alg->setProperty("InputWorkspace", pws);
  fltpk_alg->setProperty("BankName", bankname);
  fltpk_alg->setProperty("Criterion", "=");
  fltpk_alg->setProperty("OutputWorkspace", outputwsn);
  fltpk_alg->executeAsChildAlg();

  IPeaksWorkspace_sptr outWS = fltpk_alg->getProperty("OutputWorkspace");
  IPeaksWorkspace_sptr ows = std::dynamic_pointer_cast<IPeaksWorkspace>(outWS);
  return outWS;
}

/**
 * @brief Return a 1D histogram consists of ideal qSample calculated from
 *        integer HKL directly
 *
 * @param pws
 * @return MatrixWorkspace_sptr
 */
MatrixWorkspace_sptr SCDCalibratePanels2::getIdealQSampleAsHistogram1D(IPeaksWorkspace_sptr pws) {
  int npeaks = pws->getNumberPeaks();

  // prepare workspace to store qSample as Histogram1D
  MatrixWorkspace_sptr mws = std::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", // use workspace 2D to mock a histogram
                                          1,             // one vector
                                          3 * npeaks,    // X :: anything is fine
                                          3 * npeaks));  // Y :: flattened Q vector
  auto &spectrum = mws->getSpectrum(0);
  auto &xvector = spectrum.mutableX();
  auto &yvector = spectrum.mutableY();
  auto &evector = spectrum.mutableE();

  // directly compute qsample from UBmatrix and HKL
  auto ubmatrix = pws->sample().getOrientedLattice().getUB();
  for (int i = 0; i < npeaks; ++i) {

    V3D qv = ubmatrix * pws->getPeak(i).getIntHKL();
    qv *= 2 * PI;
    // qv = qv / qv.norm();
    for (int j = 0; j < 3; ++j) {
      xvector[i * 3 + j] = i * 3 + j;
      yvector[i * 3 + j] = qv[j];
      evector[i * 3 + j] = 1;
    }
  }

  return mws;
}

/**
 * @brief shift T0 for both peakworkspace and all peaks
 *
 * @param dT0
 * @param pws
 */
void SCDCalibratePanels2::adjustT0(double dT0, IPeaksWorkspace_sptr &pws) {
  // update the T0 record in peakworkspace
  Mantid::API::Run &run = pws->mutableRun();
  double T0 = 0.0;
  if (run.hasProperty("T0")) {
    T0 = run.getPropertyValueAsType<double>("T0");
  }
  T0 += dT0;
  run.addProperty<double>("T0", T0, true);

  // update wavelength of each peak using new T0
  for (int i = 0; i < pws->getNumberPeaks(); ++i) {
    IPeak &pk = pws->getPeak(i);
    Units::Wavelength wl;
    wl.initialize(pk.getL1(), 0,
                  {{UnitParams::l2, pk.getL2()},
                   {UnitParams::twoTheta, pk.getScattering()},
                   {UnitParams::efixed, pk.getInitialEnergy()}});
    pk.setWavelength(wl.singleFromTOF(pk.getTOF() + dT0));
  }
}

/**
 * @brief adjust instrument component position and orientation
 *
 * @param dx
 * @param dy
 * @param dz
 * @param rvx
 * @param rvy
 * @param rvz
 * @param rang
 * @param cmptName
 * @param pws
 */
void SCDCalibratePanels2::adjustComponent(double dx, double dy, double dz, double rvx, double rvy, double rvz,
                                          double rang, std::string cmptName, IPeaksWorkspace_sptr &pws) {

  // orientation
  IAlgorithm_sptr rot_alg = createChildAlgorithm("RotateInstrumentComponent", -1, -1, false);
  rot_alg->setLogging(LOGCHILDALG);
  rot_alg->setProperty<Workspace_sptr>("Workspace", pws);
  rot_alg->setProperty("ComponentName", cmptName);
  rot_alg->setProperty("X", rvx);
  rot_alg->setProperty("Y", rvy);
  rot_alg->setProperty("Z", rvz);
  rot_alg->setProperty("Angle", rang);
  rot_alg->setProperty("RelativeRotation", true);
  rot_alg->executeAsChildAlg();

  // translation
  IAlgorithm_sptr mv_alg = createChildAlgorithm("MoveInstrumentComponent", -1, -1, false);
  mv_alg->setLogging(LOGCHILDALG);
  mv_alg->setProperty<Workspace_sptr>("Workspace", pws);
  mv_alg->setProperty("ComponentName", cmptName);
  mv_alg->setProperty("X", dx);
  mv_alg->setProperty("Y", dy);
  mv_alg->setProperty("Z", dz);
  mv_alg->setProperty("RelativePosition", true);
  mv_alg->executeAsChildAlg();
}

/**
 * @brief Generate a tableworkspace to store the calibration results
 *
 * @param instrument  :: calibrated instrument
 * @return DataObjects::TableWorkspace_sptr
 */
ITableWorkspace_sptr SCDCalibratePanels2::generateCalibrationTable(std::shared_ptr<Geometry::Instrument> &instrument) {
  g_log.notice() << "Generate a TableWorkspace to store calibration results.\n";

  // Create table workspace
  ITableWorkspace_sptr itablews = WorkspaceFactory::Instance().createTable();
  // TableWorkspace_sptr tablews =
  //     std::dynamic_pointer_cast<TableWorkspace>(itablews);

  for (int i = 0; i < 8; ++i)
    itablews->addColumn(calibrationTableColumnTypes[i], calibrationTableColumnNames[i]);

  // The first row is always the source
  IComponent_const_sptr source = instrument->getSource();
  V3D sourceRelPos = source->getRelativePos();
  Mantid::API::TableRow sourceRow = itablews->appendRow();
  // NOTE: source should not have any rotation, so we pass a zero
  //       rotation with a fixed axis
  sourceRow << instrument->getSource()->getName() << sourceRelPos.X() << sourceRelPos.Y() << sourceRelPos.Z() << 1.0
            << 0.0 << 0.0 << 0.0;

  // Loop through banks and set row values
  for (auto bankName : m_BankNames) {
    // CORELLLI instrument has one extra layer that pack tubes into
    // banks, which is what we need here
    if (instrument->getName().compare("CORELLI") == 0)
      bankName.append("/sixteenpack");

    std::shared_ptr<const IComponent> bank = instrument->getComponentByName(bankName);

    Quat relRot = bank->getRelativeRot();
    V3D pos1 = bank->getRelativePos();

    // Calculate cosines using relRot
    double deg, xAxis, yAxis, zAxis;
    relRot.getAngleAxis(deg, xAxis, yAxis, zAxis);

    // Append a new row
    Mantid::API::TableRow bankRow = itablews->appendRow();
    // Row and positions
    bankRow << bankName << pos1.X() << pos1.Y() << pos1.Z() << xAxis << yAxis << zAxis << deg;
  }

  g_log.notice() << "finished generating tables\n";
  setProperty("OutputWorkspace", itablews);

  return itablews;
}

/**
 * Saves the new instrument to an xml file that can be used with the
 * LoadParameterFile Algorithm. If the filename is empty, nothing gets
 * done.
 *
 * @param FileName     The filename to save this information to
 *
 * @param AllBankNames The names of the banks in each group whose values
 * are to be saved to the file
 *
 * @param instrument   The instrument with the new values for the banks
 * in Groups
 *
 * TODO:
 *  - Need to find a way to add the information regarding calibrated T0
 */
void SCDCalibratePanels2::saveXmlFile(const std::string &FileName,
                                      boost::container::flat_set<std::string> &AllBankNames,
                                      std::shared_ptr<Instrument> &instrument) {
  g_log.notice() << "Generating xml tree"
                 << "\n";

  using boost::property_tree::ptree;
  ptree root;
  ptree parafile;

  // configure root node
  parafile.put("<xmlattr>.instrument", instrument->getName());
  parafile.put("<xmlattr>.valid-from", instrument->getValidFromDate().toISO8601String());

  // configure and add each bank
  for (auto bankName : AllBankNames) {
    // Prepare data for node
    if (instrument->getName().compare("CORELLI") == 0)
      bankName.append("/sixteenpack");

    std::shared_ptr<const IComponent> bank = instrument->getComponentByName(bankName);

    Quat relRot = bank->getRelativeRot();
    std::vector<double> relRotAngles = relRot.getEulerAngles("XYZ");
    V3D pos1 = bank->getRelativePos();
    // TODO: no handling of scaling for now, will add back later
    double scalex = 1.0;
    double scaley = 1.0;

    // prepare node
    ptree bank_root;
    ptree bank_dx, bank_dy, bank_dz;
    ptree bank_dx_val, bank_dy_val, bank_dz_val;
    ptree bank_drotx, bank_droty, bank_drotz;
    ptree bank_drotx_val, bank_droty_val, bank_drotz_val;
    ptree bank_sx, bank_sy;
    ptree bank_sx_val, bank_sy_val;

    // add data to node
    bank_dx_val.put("<xmlattr>.val", pos1.X());
    bank_dy_val.put("<xmlattr>.val", pos1.Y());
    bank_dz_val.put("<xmlattr>.val", pos1.Z());
    bank_dx.put("<xmlattr>.name", "x");
    bank_dy.put("<xmlattr>.name", "y");
    bank_dz.put("<xmlattr>.name", "z");

    bank_drotx_val.put("<xmlattr>.val", relRotAngles[0]);
    bank_droty_val.put("<xmlattr>.val", relRotAngles[1]);
    bank_drotz_val.put("<xmlattr>.val", relRotAngles[2]);
    bank_drotx.put("<xmlattr>.name", "rotx");
    bank_droty.put("<xmlattr>.name", "roty");
    bank_drotz.put("<xmlattr>.name", "rotz");

    bank_sx_val.put("<xmlattr>.val", scalex);
    bank_sy_val.put("<xmlattr>.val", scaley);
    bank_sx.put("<xmlattr>.name", "scalex");
    bank_sy.put("<xmlattr>.name", "scaley");

    bank_root.put("<xmlattr>.name", bankName);

    // configure structure
    bank_dx.add_child("value", bank_dx_val);
    bank_dy.add_child("value", bank_dy_val);
    bank_dz.add_child("value", bank_dz_val);

    bank_drotx.add_child("value", bank_drotx_val);
    bank_droty.add_child("value", bank_droty_val);
    bank_drotz.add_child("value", bank_drotz_val);

    bank_sx.add_child("value", bank_sx_val);
    bank_sy.add_child("value", bank_sy_val);

    bank_root.add_child("parameter", bank_drotx);

    bank_root.add_child("parameter", bank_droty);
    bank_root.add_child("parameter", bank_drotz);
    bank_root.add_child("parameter", bank_dx);
    bank_root.add_child("parameter", bank_dy);
    bank_root.add_child("parameter", bank_dz);
    bank_root.add_child("parameter", bank_sx);
    bank_root.add_child("parameter", bank_sy);

    parafile.add_child("component-link", bank_root);
  }

  // get L1 info for source
  ptree src;
  ptree src_dx, src_dy, src_dz;
  ptree src_dx_val, src_dy_val, src_dz_val;
  // -- get positional data from source
  IComponent_const_sptr source = instrument->getSource();
  V3D sourceRelPos = source->getRelativePos();
  // -- add date to node
  src_dx_val.put("<xmlattr>.val", sourceRelPos.X());
  src_dy_val.put("<xmlattr>.val", sourceRelPos.Y());
  src_dz_val.put("<xmlattr>.val", sourceRelPos.Z());
  src_dx.put("<xmlattr>.name", "x");
  src_dy.put("<xmlattr>.name", "y");
  src_dz.put("<xmlattr>.name", "z");
  src.put("<xmlattr>.name", source->getName());

  src_dx.add_child("value", src_dx_val);
  src_dy.add_child("value", src_dy_val);
  src_dz.add_child("value", src_dz_val);
  src.add_child("parameter", src_dx);
  src.add_child("parameter", src_dy);
  src.add_child("parameter", src_dz);

  parafile.add_child("component-link", src);

  // give everything to root
  root.add_child("parameter-file", parafile);
  // write the xml tree to disk
  g_log.notice() << "\tSaving parameter file as " << FileName << "\n";
  boost::property_tree::write_xml(FileName, root, std::locale(),
                                  boost::property_tree::xml_writer_settings<std::string>(' ', 2));
}

/**
 * Really this is the operator SaveIsawDetCal but only the results of the given
 * banks are saved.  L1 and T0 are also saved.
 *
 * @param filename     -The name of the DetCal file to save the results to
 * @param AllBankName  -the set of the NewInstrument names of the banks(panels)
 * @param instrument   -The instrument with the correct panel geometries
 *                      and initial path length
 * @param T0           -The time offset from the DetCal file
 */
void SCDCalibratePanels2::saveIsawDetCal(const std::string &filename,
                                         boost::container::flat_set<std::string> &AllBankName,
                                         std::shared_ptr<Instrument> &instrument, double T0) {
  g_log.notice() << "Saving DetCal file in " << filename << "\n";

  // create a workspace to pass to SaveIsawDetCal
  const size_t number_spectra = instrument->getNumberDetectors();
  Workspace2D_sptr wksp =
      std::dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", number_spectra, 2, 1));
  wksp->setInstrument(instrument);
  wksp->rebuildSpectraMapping(true /* include monitors */);

  // convert the bank names into a vector
  std::vector<std::string> banknames(AllBankName.begin(), AllBankName.end());

  // call SaveIsawDetCal
  API::IAlgorithm_sptr alg = createChildAlgorithm("SaveIsawDetCal");
  alg->setProperty("InputWorkspace", wksp);
  alg->setProperty("Filename", filename);
  alg->setProperty("TimeOffset", T0);
  alg->setProperty("BankNames", banknames);
  alg->executeAsChildAlg();
}

/**
 * @brief Save the CORELLI calibration table into a CSV file
 *
 * @param FileName
 * @param tws
 */
void SCDCalibratePanels2::saveCalibrationTable(const std::string &FileName, ITableWorkspace_sptr &tws) {
  API::IAlgorithm_sptr alg = createChildAlgorithm("SaveAscii");
  alg->setProperty("InputWorkspace", tws);
  alg->setProperty("Filename", FileName);
  alg->setPropertyValue("CommentIndicator", "#");
  alg->setPropertyValue("Separator", "CSV");
  alg->setProperty("ColumnHeader", true);
  alg->setProperty("AppendToFile", false);
  alg->executeAsChildAlg();
}

/**
 * @brief Profile obj func along L1 axis
 *
 * @param pws
 * @param pws_original
 */
void SCDCalibratePanels2::profileL1(Mantid::API::IPeaksWorkspace_sptr &pws,
                                    Mantid::API::IPeaksWorkspace_sptr pws_original) {
  g_log.notice() << "START of profiling objective func along L1\n";

  // control option
  bool verbose = getProperty("VerboseOutput");
  if (verbose) {
    // header to console
    g_log.notice() << "deltaL1 -- residual\n";
  }

  // prepare container for profile information
  std::ostringstream msgrst;
  msgrst.precision(12);
  msgrst << "dL1\tresidual\n";

  // setting up as if we are doing optimization
  auto objf = std::make_shared<SCDCalibratePanels2ObjFunc>();
  // NOTE: always use the original pws to get the tofs
  std::vector<double> tofs = captureTOF(pws_original);
  objf->setPeakWorkspace(pws, "moderator", tofs);

  // call the obj to perform evaluation
  const int n_peaks = pws->getNumberPeaks();
  std::unique_ptr<double[]> target(new double[n_peaks * 3]);

  // generate the target
  auto ubmatrix = pws->sample().getOrientedLattice().getUB();
  for (int i = 0; i < n_peaks; ++i) {
    V3D qv = ubmatrix * pws->getPeak(i).getIntHKL();
    qv *= 2 * PI;
    for (int j = 0; j < 3; ++j) {
      target[i * 3 + j] = qv[j];
    }
  }

  double xValues[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; // xValues is not used

  // scan from -4cm to 4cm along dL1 where the minimum is supposed to be at 0 for null
  // case with instrument at the engineering position
  double deltaL1 = -4e-2;
  while (deltaL1 < 4e-2) {
    std::unique_ptr<double[]> out(new double[n_peaks * 3]);
    objf->setParameter("DeltaZ", deltaL1);
    objf->setParameter("DeltaT0", 0.0); // need to set dT0 to 0.0 if we are not cali it
    objf->function1D(out.get(), xValues, 1);

    // calc residual
    double residual = 0.0;
    for (int i = 0; i < n_peaks * 3; ++i) {
      residual += (out[i] - target[i]) * (out[i] - target[i]);
    }
    residual = std::sqrt(residual) / (n_peaks - 1); // only 1 deg of freedom here
    // log rst
    msgrst << deltaL1 << "\t" << residual << "\n";

    if (verbose) {
      g_log.notice() << deltaL1 << " -- " << residual << "\n";
    }

    // increment
    deltaL1 += 1e-4; // 0.1mm step size
  }

  // output to file
  auto filenamebase = boost::filesystem::temp_directory_path();
  filenamebase /= boost::filesystem::unique_path("profileSCDCalibratePanels2_L1.csv");
  std::ofstream profL1File;
  profL1File.open(filenamebase.string());
  profL1File << msgrst.str();
  profL1File.close();
  g_log.notice() << "Profile data is saved at:\n"
                 << filenamebase << "\n"
                 << "END of profiling objective func along L1\n";
}

/**
 * @brief Profiling obj func along six degree of freedom, which can very slow.
 *
 * @param pws
 * @param pws_original
 */
void SCDCalibratePanels2::profileBanks(Mantid::API::IPeaksWorkspace_sptr &pws,
                                       Mantid::API::IPeaksWorkspace_sptr pws_original) {
  g_log.notice() << "START of profiling all banks along six degree of freedom\n";

  // control option
  bool verbose = getProperty("VerboseOutput");
  if (verbose) {
    // header to console
    g_log.notice() << "--bankname: residual\n";
  }

  // Use OPENMP to speed up the profiling
  PARALLEL_FOR_IF(Kernel::threadSafe(*pws))
  for (int i = 0; i < static_cast<int>(m_BankNames.size()); ++i) {
    PARALLEL_START_INTERUPT_REGION
    // prepare local copies to work with
    const std::string bankname = *std::next(m_BankNames.begin(), i);
    const std::string pwsBankiName = "_pws_" + bankname;

    //-- step 0: extract peaks that lies on the current bank
    IPeaksWorkspace_sptr pwsBanki = selectPeaksByBankName(pws, bankname, pwsBankiName);
    //   get tofs from the original subset of pws
    IPeaksWorkspace_sptr pwsBanki_original = selectPeaksByBankName(pws_original, bankname, pwsBankiName);
    std::vector<double> tofs = captureTOF(pwsBanki_original);

    // Do not attempt correct panels with less than 6 peaks as the system will
    // be under-determined
    int nBankPeaks = pwsBanki->getNumberPeaks();
    if (nBankPeaks < MINIMUM_PEAKS_PER_BANK) {
      // use ostringstream to prevent OPENMP breaks log info
      std::ostringstream msg_npeakCheckFail;
      msg_npeakCheckFail << "-- Cannot profile Bank " << bankname << " have only " << nBankPeaks << " (<"
                         << MINIMUM_PEAKS_PER_BANK << ") Peaks, skipping\n";
      g_log.notice() << msg_npeakCheckFail.str();
      continue;
    }

    //
    MatrixWorkspace_sptr wsBankCali = getIdealQSampleAsHistogram1D(pwsBanki);
    std::ostringstream msgrst;
    msgrst.precision(12);
    msgrst << "dx\tdy\tdz\ttheta\tphi\trogang\tresidual\n";
    //
    auto objf = std::make_shared<SCDCalibratePanels2ObjFunc>();
    objf->setPeakWorkspace(pwsBanki, bankname, tofs);
    //
    const int n_peaks = pwsBanki->getNumberPeaks();
    std::unique_ptr<double[]> target(new double[n_peaks * 3]);
    // generate the target
    auto ubmatrix = pwsBanki->sample().getOrientedLattice().getUB();
    for (int i = 0; i < n_peaks; ++i) {
      V3D qv = ubmatrix * pwsBanki->getPeak(i).getIntHKL();
      qv *= 2 * PI;
      for (int j = 0; j < 3; ++j) {
        target[i * 3 + j] = qv[j];
      }
    }
    //
    double xValues[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; // xValues is not used

    // NOTE: very expensive scan of the parameter space
    for (double dx = -1e-2; dx < 1e-2; dx += 2e-2 / 20.0) {
      // deltaX: meter
      for (double dy = -1e-2; dy < 1e-2; dy += 2e-2 / 20.0) {
        // deltaY: meter
        for (double dz = -1e-2; dz < 1e-2; dz += 2e-2 / 20.0) {
          // deltaZ: meter
          for (double theta = 0.0; theta < PI; theta += PI / 20.0) {
            // theta: rad
            for (double phi = 0.0; phi < 2 * PI; phi += 2 * PI / 20.0) {
              // phi: rad
              for (double ang = -5.0; ang < 5.0; ang += 5.0 / 20.0) {
                // ang: degrees
                // configure the objfunc
                std::unique_ptr<double[]> out(new double[n_peaks * 3]);
                objf->setParameter("DeltaX", dx);
                objf->setParameter("DeltaY", dy);
                objf->setParameter("DeltaZ", dz);
                objf->setParameter("Theta", theta);
                objf->setParameter("Phi", phi);
                objf->setParameter("DeltaRotationAngle", ang);
                objf->setParameter("DeltaT0", 0.0); // need to set dT0 to 0.0 if we are not cali it
                objf->function1D(out.get(), xValues, 1);
                // calc residual
                double residual = 0.0;
                for (int i = 0; i < n_peaks * 3; ++i) {
                  residual += (out[i] - target[i]) * (out[i] - target[i]);
                }
                residual = std::sqrt(residual) / (n_peaks - 6);
                // record
                msgrst << dx << "\t" << dy << "\t" << dz << "\t" << theta << "\t" << phi << "\t" << ang << "\t"
                       << residual << "\n";

                if (verbose) {
                  g_log.notice() << "--" << bankname << ": " << residual << "\n";
                }
              }
            }
          }
        }
      }
    }

    // output to file
    auto filenamebase = boost::filesystem::temp_directory_path();
    std::string fnbase = "profileSCDCalibratePanels2_" + bankname + ".csv";
    filenamebase /= boost::filesystem::unique_path(fnbase);
    std::ofstream profBankFile;
    profBankFile.open(filenamebase.string());
    profBankFile << msgrst.str();
    profBankFile.close();

    // notify at the terminal
    std::ostringstream msg;
    msg << "Profile of " << bankname << " is saved at:\n"
        << filenamebase << "\n"
        << "END of profiling objective func for " << bankname << "\n";
    g_log.notice() << msg.str();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

/**
 * @brief Profile obj func along T0 axis
 *
 * @param pws
 * @param pws_original
 */
void SCDCalibratePanels2::profileT0(Mantid::API::IPeaksWorkspace_sptr &pws,
                                    Mantid::API::IPeaksWorkspace_sptr pws_original) {
  g_log.notice() << "START of profiling objective func along T0\n";

  // control option
  bool verbose = getProperty("VerboseOutput");
  if (verbose) {
    // print the header to console
    g_log.notice() << "deltaT0 -- residual\n";
  }

  // prepare container for profile information
  std::ostringstream msgrst;
  msgrst.precision(12);
  msgrst << "dT0\tresidual\n";

  // setting up as if we are doing optimization
  auto objf = std::make_shared<SCDCalibratePanels2ObjFunc>();
  // NOTE: always use the original pws to get the tofs
  std::vector<double> tofs = captureTOF(pws_original);
  objf->setPeakWorkspace(pws, "none", tofs);

  // call the obj to perform evaluation
  const int n_peaks = pws->getNumberPeaks();
  std::unique_ptr<double[]> target(new double[n_peaks * 3]);

  // generate the target
  auto ubmatrix = pws->sample().getOrientedLattice().getUB();
  for (int i = 0; i < n_peaks; ++i) {
    V3D qv = ubmatrix * pws->getPeak(i).getIntHKL();
    qv *= 2 * PI;
    for (int j = 0; j < 3; ++j) {
      target[i * 3 + j] = qv[j];
    }
  }

  double xValues[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; // xValues is not used

  // scan from -10 ~ 10 ms along dT0
  double deltaT0 = -10;
  while (deltaT0 < 10) {
    std::unique_ptr<double[]> out(new double[n_peaks * 3]);
    objf->setParameter("DeltaT0", deltaT0);
    objf->function1D(out.get(), xValues, 1);

    // calc residual
    double residual = 0.0;
    for (int i = 0; i < n_peaks * 3; ++i) {
      residual += (out[i] - target[i]) * (out[i] - target[i]);
    }
    residual = std::sqrt(residual) / (n_peaks - 1); // only 1 deg of freedom here
    // log rst
    msgrst << deltaT0 << "\t" << residual << "\n";

    if (verbose) {
      g_log.notice() << deltaT0 << " -- " << residual << "\n";
    }

    // increment
    deltaT0 += 0.01; // 20/2000.0
  }

  // output to file
  auto filenamebase = boost::filesystem::temp_directory_path();
  filenamebase /= boost::filesystem::unique_path("profileSCDCalibratePanels2_T0.csv");
  std::ofstream profL1File;
  profL1File.open(filenamebase.string());
  profL1File << msgrst.str();
  profL1File.close();
  g_log.notice() << "Profile data is saved at:\n"
                 << filenamebase << "\n"
                 << "END of profiling objective func along T0\n";
}

} // namespace Crystal
} // namespace Mantid
