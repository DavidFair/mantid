#include "MantidAPI/AnalysisDataService.h"
#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/MantidTreeModel.h"
#include "MantidQtWidgets/Common/MantidWSIndexDialog.h"
#include <Poco/ActiveResult.h>
#include <qcoreapplication.h>
#include <QMessageBox>

using namespace std;
using namespace MantidQt;
using namespace MantidWidgets;
using namespace Mantid::API;

namespace {
	Mantid::Kernel::Logger g_log("WorkspaceWidget");
}

MantidTreeModel::MantidTreeModel() {
}

// Data display and saving methods
void MantidTreeModel::updateRecentFilesList(const QString &fname){ /*Not require until tool bar is created*/ }
void MantidTreeModel::enableSaveNexus(const QString &wsName) { /*handled by widget*/ }
void MantidTreeModel::disableSaveNexus(){ /* handled by widget*/ }
void MantidTreeModel::deleteWorkspaces(const QStringList &wsNames) {
	try {
		if (!wsNames.isEmpty()) {
			auto alg = createAlgorithm("DeleteWorkspaces");
			alg->setLogging(false);
			std::vector<std::string> vecWsNames;
			vecWsNames.reserve(wsNames.size());
			foreach(auto wsName, wsNames) {
				vecWsNames.push_back(wsName.toStdString());
			}
			alg->setProperty("WorkspaceList", vecWsNames);
			executeAlgorithmAsync(alg);
		}
	} catch (...) {
		QMessageBox::warning(nullptr, "", "Could not delete selected workspaces.");
	}
}


void MantidTreeModel::importWorkspace(){ throw runtime_error("Not implemented"); }
MantidMatrix *
	MantidTreeModel::importMatrixWorkspace(const Mantid::API::MatrixWorkspace_sptr workspace,
                    int lower, int upper,bool showDlg){
	throw runtime_error("Not implemented");
}

void MantidTreeModel::importWorkspace(const QString &wsName, bool showDlg,
                            bool makeVisible){
	throw runtime_error("Not implemented");
}
void MantidTreeModel::renameWorkspace(QStringList wsName){ 
	// Determine the algorithm
	QString algName("RenameWorkspace");
	if (wsName.size() > 1)
		algName = "RenameWorkspaces";

	QHash<QString, QString> presets;
	if (wsName.size() > 1) {
		presets["InputWorkspaces"] = wsName.join(",");
	}
	else {
		presets["InputWorkspace"] = wsName[0];
	}
	showAlgorithmDialog(algName, presets);
}

void MantidTreeModel::showMantidInstrumentSelected(){ throw runtime_error("Not implemented"); }
Table *MantidTreeModel::createDetectorTable(const QString &wsName,
                                    const std::vector<int> &indices,
                                    bool include_data){
	throw runtime_error("Not implemented");
}
void MantidTreeModel::importBoxDataTable(){ throw runtime_error("Not implemented"); }
void MantidTreeModel::showListData(){ throw runtime_error("Not implemented"); }
void MantidTreeModel::importTransposed(){ throw runtime_error("Not implemented"); }

// Algorithm Display and Execution Methods
Mantid::API::IAlgorithm_sptr MantidTreeModel::createAlgorithm(const QString &algName,
                                                    int version){
	Mantid::API::IAlgorithm_sptr alg;
	try {
		alg = Mantid::API::AlgorithmManager::Instance().create(
			algName.toStdString(), version);
	}
	catch (...) {
		QString message = "Cannot create algorithm \"" + algName + "\"";
		if (version != -1) {
			message += " version " + QString::number(version);
		}
		QMessageBox::warning(nullptr, "MantidPlot", message);
		alg = Mantid::API::IAlgorithm_sptr();
	}
	return alg;
}
void MantidTreeModel::showAlgorithmDialog(const QString &algName, int version){
	// Check if Alg is valid
	Mantid::API::IAlgorithm_sptr alg = this->createAlgorithm(algName, version);
	if (!alg)
		return;
	MantidQt::API::InterfaceManager interfaceManager;
	MantidQt::API::AlgorithmDialog *dlg = interfaceManager.createDialog(
		alg, nullptr, false);
	dlg->show();
	dlg->raise();
	dlg->activateWindow();
}

void
	MantidTreeModel::showAlgorithmDialog(const QString &algName, QHash<QString, QString> paramList,
                    Mantid::API::AlgorithmObserver *obs,
                    int version){
	// Get latest version of the algorithm
	Mantid::API::IAlgorithm_sptr alg = this->createAlgorithm(algName, version);
	if (!alg)
		return;

	for (QHash<QString, QString>::Iterator it = paramList.begin();
		it != paramList.end(); ++it) {
		alg->setPropertyValue(it.key().toStdString(), it.value().toStdString());
	}
	MantidQt::API::AlgorithmDialog *dlg = createAlgorithmDialog(alg);
	if (obs) {
		dlg->addAlgorithmObserver(obs);
	}

	dlg->show();
	dlg->raise();
	dlg->activateWindow();
}

/**
* This creates an algorithm dialog (the default property entry thingie).
*/
MantidQt::API::AlgorithmDialog *
MantidTreeModel::createAlgorithmDialog(Mantid::API::IAlgorithm_sptr alg) {
	QHash<QString, QString> presets;
	QStringList enabled;

	// If a property was explicitly set show it as preset in the dialog
	const std::vector<Mantid::Kernel::Property *> props = alg->getProperties();
	std::vector<Mantid::Kernel::Property *>::const_iterator p = props.begin();
	for (; p != props.end(); ++p) {
		if (!(**p).isDefault()) {
			QString property_name = QString::fromStdString((**p).name());
			presets.insert(property_name, QString::fromStdString((**p).value()));
			enabled.append(property_name);
		}
	}
	// If a workspace is selected in the dock then set this as a preset for the
	// dialog
	/*QString selected = getSelectedWorkspaceName();
	if (!selected.isEmpty()) {
		QString property_name = findInputWorkspaceProperty(alg);
		if (!presets.contains(property_name)) {
			presets.insert(property_name, selected);
			// Keep it enabled
			enabled.append(property_name);
		}
	}*/

	// Check if a workspace is selected in the dock and set this as a preference
	// for the input workspace
	// This is an optional message displayed at the top of the GUI.
	QString optional_msg(alg->summary().c_str());

	MantidQt::API::InterfaceManager interfaceManager;
	MantidQt::API::AlgorithmDialog *dlg = interfaceManager.createDialog(
		alg, nullptr, false, presets, optional_msg, enabled);
	return dlg;
}


void MantidTreeModel::executeAlgorithm(Mantid::API::IAlgorithm_sptr alg) {
	executeAlgorithmAsync(alg);
}

bool MantidTreeModel::executeAlgorithmAsync(Mantid::API::IAlgorithm_sptr alg,
                                    const bool wait) {
	if (wait) {
		Poco::ActiveResult<bool> result(alg->executeAsync());
		while (!result.available()) {
			QCoreApplication::processEvents();
		}
		result.wait();

		try {
			return result.data();
		}
		catch (Poco::NullPointerException &) {
			return false;
		}
	}
	else {
		try {
			alg->executeAsync();
		}
		catch (Poco::NoThreadAvailableException &) {
			g_log.error() << "No thread was available to run the " << alg->name()
				<< " algorithm in the background.\n";
			return false;
		}
		return true;
	}
}

Workspace_const_sptr
	MantidTreeModel::getWorkspace(const QString &workspaceName){
	if (AnalysisDataService::Instance().doesExist(workspaceName.toStdString())) {
		return AnalysisDataService::Instance().retrieve(
			workspaceName.toStdString());
	}
	Workspace_const_sptr empty;
	return empty; //??
}

QWidget *MantidTreeModel::getParent(){ throw runtime_error("Not implemented"); }

// Plotting Methods
MultiLayer *
	MantidTreeModel::plot1D(const QMultiMap<QString, std::set<int>> &toPlot, bool spectrumPlot,
        MantidQt::DistributionFlag distr,
        bool errs, MultiLayer *plotWindow,
        bool clearWindow, bool waterfallPlot,
        const QString &log,
        const std::set<double> &customLogValues){ throw runtime_error("Not implemented");}

void MantidTreeModel::drawColorFillPlots(
    const QStringList &wsNames,
    GraphOptions::CurveType curveType){ throw runtime_error("Not implemented");}

void MantidTreeModel::showMDPlot(){ throw runtime_error("Not implemented");}

MultiLayer *
	MantidTreeModel::plotSubplots(const QMultiMap<QString, std::set<int>> &toPlot,
            MantidQt::DistributionFlag distr,
            bool errs, MultiLayer *plotWindow){ throw runtime_error("Not implemented");}

void MantidTreeModel::plotSurface(bool accepted, int plotIndex,
                        const QString &axisName, const QString &logName,
                        const std::set<double> &customLogValues,
                        const QList<QString> &workspaceNames){ throw runtime_error("Not implemented");}
void MantidTreeModel::plotContour(bool accepted, int plotIndex,
                                  const QString &axisName, const QString &logName,
                                  const std::set<double> &customLogValues,
                                  const QList<QString> &workspaceNames){ throw runtime_error("Not implemented");}

// Interface Methods
void MantidTreeModel::showVatesSimpleInterface(){ throw runtime_error("Not implemented");}
void MantidTreeModel::showSpectrumViewer(){ throw runtime_error("Not implemented");}
void MantidTreeModel::showSliceViewer(){ throw runtime_error("Not implemented");}
void MantidTreeModel::showLogFileWindow(){}
void MantidTreeModel::showSampleMaterialWindow(){}
void MantidTreeModel::showAlgorithmHistory(){ throw runtime_error("Not implemented");}

MantidQt::MantidWidgets::MantidWSIndexDialog *
	MantidTreeModel::createWorkspaceIndexDialog(int flags, const QStringList &wsNames,
                            bool showWaterfall, bool showPlotAll,
                            bool showTiledOpt, bool isAdvanced){ throw runtime_error("Not implemented");}

void MantidTreeModel::updateProject() { /*Currently unrequired in Workbench*/ }
void MantidTreeModel::showCritical(const QString &) { throw runtime_error("Not implemented"); }