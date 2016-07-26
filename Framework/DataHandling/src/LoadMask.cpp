#include "MantidDataHandling/LoadMask.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FileFinder.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidKernel/Strings.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IDTypes.h"

#include <fstream>
#include <sstream>
#include <map>

#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/Exception.h>

#include <boost/algorithm/string.hpp>

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::NodeList;
using Poco::XML::NodeIterator;
using Poco::XML::NodeFilter;

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace std;

namespace {
// service routines
//-------------------------------------------------------------------------------------------
/** Convert ranged vectors to single-valued vector
* @param singles -- input vector of single numbers to copy to result without
*                   changes.
* @param ranges  -- input vector of data ranges -- pairs of min-max values,
*                   expanded to result as numbers from min to max
*                   inclusively, with step 1
* @param tot_singles -- on input contains range of single values already
*                   copied into the result by previous call to the routine,
*                   on output, all signles and expanded pairs
*                   from the input are added to it.
*/
template <typename T>
void convertToVector(const std::vector<T> &singles,
                     const std::vector<T> &ranges,
                     std::vector<T> &tot_signles) {

  size_t n_total(singles.size() + tot_signles.size());
  for (size_t i = 0; i < ranges.size(); i += 2) {
    n_total += ranges[i + 1] - ranges[i] + 1;
  }
  tot_signles.reserve(n_total);
  // add singles to the existing singles
  tot_signles.insert(tot_signles.end(), singles.begin(), singles.end());
  // expand pairs
  for (size_t i = 0; i < ranges.size(); i += 2) {
    for (T obj_id = ranges[i]; obj_id < ranges[i + 1] + 1; ++obj_id) {
      tot_signles.push_back(obj_id);
    }
  }
}

/*
* Parse index range text to singles and pairs
* Example: 3,4,9-10,33
*
* @param inputstr -- input string to process in the format as above
* @param sinvles -- vector of obects, defined as singles
* @param pairs   -- vector of obects, defined as pairs, in the form min,max
*                   value
*/
template <typename T>
void parseRangeText(const std::string &inputstr, std::vector<T> &singles,
                    std::vector<T> &pairs) {
  // 1. Split ','
  std::vector<std::string> rawstrings;
  boost::split(rawstrings, inputstr, boost::is_any_of(","),
               boost::token_compress_on);

  for (auto &rawstring : rawstrings) {
    // a) Find '-':
    boost::trim(rawstring);
    bool containDash(true);
    if (rawstring.find_first_of("-") == std::string::npos) {
      containDash = false;
    }

    // Process appropriately
    if (containDash) { // 4. Treat pairs
      std::vector<std::string> ptemp;
      boost::split(ptemp, rawstring, boost::is_any_of("-"),
                   boost::token_compress_on);
      if (ptemp.size() != 2) {
        std::string error =
            "Range string " + rawstring + " has a wrong format!";
        throw std::invalid_argument(error);
      }
      // b) parse
      T intstart = boost::lexical_cast<T>(ptemp[0]);
      T intend = boost::lexical_cast<T>(ptemp[1]);
      if (intstart >= intend) {
        std::string error =
            "Range string " + rawstring + " has wrong order of detectors ID!";
        throw std::invalid_argument(error);
      }
      pairs.push_back(intstart);
      pairs.push_back(intend);

    } else { // 3. Treat singles
      T itemp = boost::lexical_cast<T>(rawstring);
      singles.push_back(itemp);
    }
  } // ENDFOR i
}

/*
* Parse a line in an ISIS mask file string to vector
* Combination of 5 types of format for unit
* (1) a (2) a-b (3) a - b (4) a- b (5) a- b
* separated by space(s)
* @param  ins    -- input string in ISIS ASCII format
* @return ranges -- vector of a,b pairs converted from input
*/
void parseISISStringToVector(const std::string &ins,
                             std::vector<Mantid::specnum_t> &ranges) {
  // 1. Split by space
  std::vector<string> splitstrings;
  boost::split(splitstrings, ins, boost::is_any_of(" "),
               boost::token_compress_on);

  // 2. Replace a-b to a - b, remove a-b and insert a, -, b
  bool tocontinue = true;
  size_t index = 0;
  while (tocontinue) {
    // a) Determine end of loop.  Note that loop size changes
    if (index == splitstrings.size() - 1) {
      tocontinue = false;
    }

    // b) Need to split?
    vector<string> temps;
    boost::split(temps, splitstrings[index], boost::is_any_of("-"),
                 boost::token_compress_on);
    if (splitstrings[index].compare("-") == 0 || temps.size() == 1) {
      // Nothing to split
      index++;
    } else if (temps.size() == 2) {
      // Has a '-' inside.  Delete and Replace
      temps.insert(temps.begin() + 1, "-");
      splitstrings.erase(splitstrings.begin() + index);
      for (size_t ic = 0; ic < 3; ic++) {
        if (temps[ic].size() > 0) {
          splitstrings.insert(splitstrings.begin() + index, temps[ic]);
          index++;
        }
      }
    } else {
      // Exception
      std::string err = "String " + splitstrings[index] + " has Too many '-'";
      throw std::invalid_argument(err);
    }

    if (index >= splitstrings.size())
      tocontinue = false;

  } // END WHILE

  // 3. Put to output integer vector
  tocontinue = true;
  index = 0;
  while (tocontinue) {
    // i)   push to the starting vector
    ranges.push_back(
        boost::lexical_cast<Mantid::specnum_t>(splitstrings[index]));

    // ii)  push the ending vector
    if (index == splitstrings.size() - 1 ||
        splitstrings[index + 1].compare("-") != 0) {
      // the next one is not '-'
      ranges.push_back(
          boost::lexical_cast<Mantid::specnum_t>(splitstrings[index]));
      index++;
    } else {
      // the next one is '-', thus read '-', next
      ranges.push_back(
          boost::lexical_cast<Mantid::specnum_t>(splitstrings[index + 2]));
      index += 3;
    }

    if (index >= splitstrings.size())
      tocontinue = false;
  } // END-WHILE

  splitstrings.clear();
}
/*
* Load and parse an ISIS masking file
@param isisfilename :: the string containing full path to an ISIS mask file
@param SpectraMasks :: output list of the spectra numbers to mask.
*/
void loadISISMaskFile(const std::string &isisfilename,
                      std::vector<Mantid::specnum_t> &spectraMasks) {

  std::vector<Mantid::specnum_t> ranges;

  std::ifstream ifs;
  ifs.open(isisfilename.c_str(), std::ios::in);
  if (!ifs.is_open()) {
    throw std::invalid_argument("Cannot open ISIS mask file" + isisfilename);
  }

  std::string isisline;
  while (getline(ifs, isisline)) {
    boost::trim(isisline);

    // a. skip empty line
    if (isisline.size() == 0)
      continue;

    // b. skip comment line
    if (isisline.c_str()[0] < '0' || isisline.c_str()[0] > '9')
      continue;

    // c. parse
    parseISISStringToVector(isisline, ranges);
  }
  ifs.close();

  // dummy helper vector as ISIS mask is always processed as pairs.
  std::vector<Mantid::specnum_t> dummy;
  convertToVector(dummy, ranges, spectraMasks);
}

/** Parse bank IDs (string name)
* Sample:            bank2
* @param valuetext:  must be bank name
* @param tomask:     if true, mask, if not unmask
* @param toMask:     vector of string containing component names for masking
* @param toUnmask    vector of strings containing component names for unmasking
*/
void parseComponent(const std::string &valuetext, bool tomask,
                    std::vector<std::string> &toMask,
                    std::vector<std::string> &toUnmask) {

  // 1. Parse bank out
  if (tomask) {
    toMask.push_back(valuetext);
  } else {
    toUnmask.push_back(valuetext);
  }
}
}

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(LoadMask)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadMask::LoadMask()
    : m_maskWS(), m_instrumentPropValue(""), m_sourceMapWS(), m_pDoc(nullptr),
      m_pRootElem(nullptr), m_defaultToUse(true) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadMask::~LoadMask() {
  // note Poco::XML::Document and Poco::XML::Element declare their constructors
  // as protected
  if (m_pDoc)
    m_pDoc->release();
  // note that m_pRootElem does not need a release(), and that can
  // actually cause a double free corruption, as
  // Poco::DOM::Document::documentElement() does not require a
  // release(). So just to be explicit that they're gone:
  m_pDoc = nullptr;
  m_pRootElem = nullptr;
}

/// Initialise the properties
void LoadMask::init() {

  // 1. Declare property
  declareProperty("Instrument", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The name of the instrument to apply the mask.");

  const std::vector<std::string> maskExts{".xml", ".msk"};
  declareProperty(Kernel::make_unique<FileProperty>(
                      "InputFile", "", FileProperty::Load, maskExts),
                  "Masking file for masking. Supported file format is XML and "
                  "ISIS ASCII. ");
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
          "RefWorkspace", "", Direction::Input, PropertyMode::Optional),
      "The name of the workspace with defines insrtument and spectra, "
      "used as the source of the spectra-detector map for the mask to load. "
      "The instrument, attached to this workspace has to be the same as the "
      "one specified by 'Instrument' property");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<DataObjects::MaskWorkspace>>(
          "OutputWorkspace", "Masking", Direction::Output),
      "Output Masking Workspace");
}

//----------------------------------------------------------------------------------------------
/** Main execution body of this algorithm
  */
void LoadMask::exec() {
  // 1. Load Instrument and create output Mask workspace
  const std::string instrumentname = getProperty("Instrument");
  m_sourceMapWS = getProperty("RefWorkspace");

  m_instrumentPropValue = instrumentname;
  setProperty("Instrument", instrumentname);

  this->intializeMaskWorkspace();
  setProperty("OutputWorkspace", m_maskWS);

  m_defaultToUse = true;

  // 2. Parse Mask File
  std::string filename = getProperty("InputFile");

  if (boost::ends_with(filename, "l") || boost::ends_with(filename, "L")) {
    // 2.1 XML File
    this->initializeXMLParser(filename);
    this->parseXML();
  } else if (boost::ends_with(filename, "k") ||
             boost::ends_with(filename, "K")) {
    // 2.2 ISIS Masking file
    loadISISMaskFile(filename, m_maskSpecID);
    m_defaultToUse = true;
  } else {
    g_log.error() << "File " << filename << " is not in supported format. \n";
    return;
  }
  // 3. Translate and set geometry
  g_log.information() << "To Mask: \n";

  this->componentToDetectors(m_maskCompIdSingle, m_maskDetID);

  // unmasking is not implemented
  // g_log.information() << "To UnMask: \n";

  // As m_uMaskCompIdSingle os empty, this never works
  this->bankToDetectors(m_uMaskCompIdSingle, m_unMaskDetID);

  // convert spectra ID to corresponet det-id-s
  this->processMaskOnWorkspaceIndex(true, m_maskSpecID, m_maskDetID);

  // 4. Apply
  this->initDetectors();
  const detid2index_map indexmap =
      m_maskWS->getDetectorIDToWorkspaceIndexMap(true);

  this->processMaskOnDetectors(indexmap, true, m_maskDetID);
  // TODO: Not implemented, but should work as soon as m_unMask contains
  // something
  this->processMaskOnDetectors(indexmap, false, m_unMaskDetID);
}

void LoadMask::initDetectors() {

  if (!m_defaultToUse) { // Default is to use all detectors
    size_t numHist = m_maskWS->getNumberHistograms();
    for (size_t wkspIndex = 0; wkspIndex < numHist; wkspIndex++) {
      m_maskWS->setMaskedIndex(wkspIndex);
    }
  }
}

//----------------------------------------------------------------------------------------------
/**  Mask detectors or Unmask detectors
 *   @param indexmap: spectraId to spectraNum map used
 *                   in masking
 *   @param tomask:  true to mask, false to unmask
 *   @param singledetids: list of individual det ids to mask

 */
void LoadMask::processMaskOnDetectors(const detid2index_map &indexmap,
                                      bool tomask,
                                      std::vector<int32_t> singledetids) {
  // 1. Get index map
  // 2. Mask
  g_log.debug() << "Mask = " << tomask
                << "  Final Single IDs Size = " << singledetids.size() << '\n';

  for (auto detid : singledetids) {
    detid2index_map::const_iterator it;
    it = indexmap.find(detid);
    if (it != indexmap.end()) {
      size_t index = it->second;
      if (tomask)
        m_maskWS->dataY(index)[0] = 1;
      else
        m_maskWS->dataY(index)[0] = 0;
    } else {
      g_log.warning() << "Pixel w/ ID = " << detid << " Cannot Be Located\n";
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Extract a component's detectors and return it within detectors array
 *  It is a generalized version of bankToDetectors()
 *
 * @param componentnames -- vector of compnents names to process
 * @param detectors      -- vector of detectors id, which belongs to components
 *provided as input.
 */
void LoadMask::componentToDetectors(
    const std::vector<std::string> &componentnames,
    std::vector<detid_t> &detectors) {
  Geometry::Instrument_const_sptr minstrument = m_maskWS->getInstrument();

  for (auto &componentname : componentnames) {
    g_log.debug() << "Component name = " << componentname << '\n';

    // a) get component
    Geometry::IComponent_const_sptr component =
        minstrument->getComponentByName(componentname);
    if (component)
      g_log.debug() << "Component ID = " << component->getComponentID() << '\n';
    else {
      // A non-exiting component.  Ignore
      g_log.warning() << "Component " << componentname << " does not exist!\n";
      continue;
    }

    // b) component -> component assembly --> children (more than detectors)
    boost::shared_ptr<const Geometry::ICompAssembly> asmb =
        boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(component);
    std::vector<Geometry::IComponent_const_sptr> children;
    asmb->getChildren(children, true);

    g_log.debug() << "Number of Children = " << children.size() << '\n';

    size_t numdets(0);
    detid_t id_min = std::numeric_limits<Mantid::detid_t>::max();
    detid_t id_max = 0;

    for (const auto &child : children) {
      // c) convert component to detector
      Geometry::IDetector_const_sptr det =
          boost::dynamic_pointer_cast<const Geometry::IDetector>(child);

      if (det) {
        detid_t detid = det->getID();
        detectors.push_back(detid);
        numdets++;
        if (detid < id_min)
          id_min = detid;
        if (detid > id_max)
          id_max = detid;
      }
    }

    g_log.debug() << "Number of Detectors in Children = " << numdets
                  << "  Range = " << id_min << ", " << id_max << '\n';
  } // for component
}

//----------------------------------------------------------------------------------------------
/** Convert bank to detectors
* This routine have never been invoked.
* @param   singlebanks -- vector of string containing bank names
* @param  detectors   -- vector of detector-id-s belonging to these banks
 */
void LoadMask::bankToDetectors(const std::vector<std::string> &singlebanks,
                               std::vector<detid_t> &detectors) {
  std::stringstream infoss;
  infoss << "Bank IDs to be converted to detectors: \n";
  for (auto &singlebank : singlebanks) {
    infoss << "Bank: " << singlebank << '\n';
  }
  g_log.debug(infoss.str());

  Geometry::Instrument_const_sptr minstrument = m_maskWS->getInstrument();

  for (auto &singlebank : singlebanks) {
    std::vector<Geometry::IDetector_const_sptr> idetectors;

    minstrument->getDetectorsInBank(idetectors, singlebank);
    g_log.debug() << "Bank: " << singlebank << " has " << idetectors.size()
                  << " detectors\n";

    // a) get information
    size_t numdets = idetectors.size();
    detid_t detid_first = idetectors.front()->getID();
    detid_t detid_last = idetectors.back()->getID();

    // b) set detectors

    for (const auto &det : idetectors) {
      int32_t detid = det->getID();
      detectors.push_back(detid);
    }
    g_log.debug() << "Number of Detectors in Bank  " << singlebank
                  << "  is: " << numdets << "\nRange From: " << detid_first
                  << " To: " << detid_last << '\n';

  } // ENDFOR
}

//----------------------------------------------------------------------------------------------
/** Set the mask on the spectrum numbers or convert them to detector-s id if
 *  sample workspace is provided
 *@param  mask           -- to mask or unmask appropriate spectra
 *@param maskedSpecID    -- vector of the spectra numbers to process
 *@param singleDetIds    -- vector of det-id-s to extend if workspace-
 *                          source of spectra-detector map is provided
 */
void LoadMask::processMaskOnWorkspaceIndex(bool mask,
                                           std::vector<int32_t> &maskedSpecID,
                                           std::vector<int32_t> &singleDetIds) {
  // 1. Check
  if (maskedSpecID.empty())
    return;

  if (m_sourceMapWS) {
    // convert spectra masks into det-id mask using source workspace
    convertSpMasksToDetIDs(m_sourceMapWS, maskedSpecID, singleDetIds);
    maskedSpecID
        .clear(); // specrtra ID not needed any more as all converted to det-ids
    return;
  }
  // 2. Get Map
  const spec2index_map s2imap = m_maskWS->getSpectrumToWorkspaceIndexMap();

  spec2index_map::const_iterator s2iter;

  // 3. Set mask
  auto spec0 = maskedSpecID[0];
  auto prev_masks = spec0;
  for (size_t i = 0; i < maskedSpecID.size(); i++) {

    auto spec2mask = maskedSpecID[i];

    s2iter = s2imap.find(spec2mask);
    if (s2iter == s2imap.end()) {
      // spectrum not found.  bad branch
      g_log.error()
          << "Spectrum " << spec2mask
          << " does not have an entry in GroupWorkspace's spec2index map\n";
      throw std::runtime_error("Logic error");
    } else {
      size_t wsindex = s2iter->second;
      if (wsindex >= m_maskWS->getNumberHistograms()) {
        // workspace index is out of range.  bad branch
        g_log.error() << "Group workspace's spec2index map is set wrong: "
                      << " Found workspace index = " << wsindex
                      << " for spectrum No " << spec2mask
                      << " with workspace size = "
                      << m_maskWS->getNumberHistograms() << '\n';
      } else {
        // Finally set the maskiing;
        if (mask)
          m_maskWS->dataY(wsindex)[0] = 1.0;
        else
          m_maskWS->dataY(wsindex)[0] = 0.0;
      } // IF-ELSE: ws index out of range
    }   // IF-ELSE: spectrum No has an entry

    if (spec2mask > prev_masks + 1) {
      g_log.debug() << "Masked Spectrum " << spec0 << "  To " << prev_masks
                    << '\n';
      spec0 = spec2mask;
    }
  } // FOR EACH SpecNo
}

//----------------------------------------------------------------------------------------------
/** Initalize Poco XML Parser
* @param filename  -- name of the xml file to process.
 */
void LoadMask::initializeXMLParser(const std::string &filename) {
  // const std::string instName
  std::cout << "Load File " << filename << '\n';
  const std::string xmlText = Kernel::Strings::loadFile(filename);
  std::cout << "Successfully Load XML File \n";

  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  try {
    m_pDoc = pParser.parseString(xmlText);
  } catch (Poco::Exception &exc) {
    throw Kernel::Exception::FileError(
        exc.displayText() + ". Unable to parse File:", filename);
  } catch (...) {
    throw Kernel::Exception::FileError("Unable to parse File:", filename);
  }
  // Get pointer to root element
  m_pRootElem = m_pDoc->documentElement();
  if (!m_pRootElem->hasChildNodes()) {
    g_log.error("XML file: " + filename + "contains no root element.");
    throw Kernel::Exception::InstrumentDefinitionError(
        "No root element in XML instrument file", filename);
  }
}

//----------------------------------------------------------------------------------------------
/** Parse XML file and define the following internal variables:
    std::vector<detid_t> m_maskDetID;
    //spectrum id-s to unmask
    std::vector<detid_t> m_unMaskDetID;

    spectra mask provided
    std::vector<specnum_t> m_maskSpecID;
    spectra unmask provided NOT IMPLEMENTED
    std::vector<specnum_t> m_unMaskSpecID;

    std::vector<std::string> m_maskCompIDSingle;
    std::vector<std::string> m_uMaskCompIDSingle;
//
Supported xml Node names are:
component:  the name of an instrument component, containing detectors.
ids      : spectra numbers
detids   : detector numbers
Full implementation needs unit tests verifying all these. Only detector id-s are
currently implemented
// There are also no current support for keyword, switching on un-masking
 */
void LoadMask::parseXML() {
  // 0. Check
  if (!m_pDoc)
    throw std::runtime_error("Call LoadMask::initialize() before parseXML.");

  // 1. Parse and create a structure
  Poco::AutoPtr<NodeList> pNL_type = m_pRootElem->getElementsByTagName("type");
  g_log.information() << "Node Size = " << pNL_type->length() << '\n';

  Poco::XML::NodeIterator it(m_pDoc, Poco::XML::NodeFilter::SHOW_ELEMENT);
  Poco::XML::Node *pNode = it.nextNode();

  std::vector<specnum_t> singleSp, pairSp;
  std::vector<detid_t> maskSingleDet, maskPairDet;
  std::vector<detid_t> umaskSingleDet, umaskPairDet;

  bool tomask = true;
  bool ingroup = false;
  while (pNode) {
    const Poco::XML::XMLString value = pNode->innerText();

    if (pNode->nodeName().compare("group") == 0) {
      // Node "group"
      ingroup = true;
      tomask = true;

    } else if (pNode->nodeName().compare("component") == 0) {
      // Node "component"
      if (ingroup) {
        parseComponent(value, tomask, m_maskCompIdSingle, m_uMaskCompIdSingle);
      } else {
        g_log.error() << "XML File heirachial (component) error!\n";
      }
      // g_log.information() << "Component: " << value << '\n';

    } else if (pNode->nodeName().compare("ids") == 0) {
      // Node "ids"
      if (ingroup) {
        parseRangeText(value, singleSp, pairSp);
        // this->parseSpectrumNos(value, m_maskSpecID);
      } else {
        g_log.error() << "XML File (ids) heirachial error!"
                      << "  Inner Text = " << pNode->innerText() << '\n';
      }

    } else if (pNode->nodeName().compare("detids") == 0) {
      // Node "detids"
      if (ingroup) {
        if (tomask) {
          parseRangeText(value, maskSingleDet, maskPairDet);
        } else { // NOTE -- currently never happens.TODO: NOT IMPLEMENTED
          parseRangeText(value, umaskSingleDet, umaskPairDet);
        }
      } else {
        g_log.error() << "XML File (detids) heirachial error!\n";
      }

    } else if (pNode->nodeName().compare("detector-masking") == 0) {
      // Node "detector-masking".  Check default value
      m_defaultToUse = true;
    } // END-IF-ELSE: pNode->nodeName()

    pNode = it.nextNode();
  } // ENDWHILE

  convertToVector(singleSp, pairSp, m_maskSpecID);
  convertToVector(maskSingleDet, maskPairDet, m_maskDetID);
  // NOTE: -- TODO: NOT IMPLEMENTD -- if unmasking is implemented, should be
  // enabled
  // convertToVector(umaskSingleDet, umaskPairDet, m_unMaskDetID);
}

/* Convert spectra mask into det-id mask using workspace as source of
*spectra-detector maps
*
* @param sourceWS       -- the workspace containing source spectra-detecot map
*                          to use on masks
* @param maskedSpecID   -- vector of spectra id to mask
* @param singleDetIds   -- output vector of detectors id to mask
*/
void LoadMask::convertSpMasksToDetIDs(const API::MatrixWorkspace_sptr &sourceWS,
                                      const std::vector<int32_t> &maskedSpecID,
                                      std::vector<int32_t> &singleDetIds) {

  spec2index_map s2imap = sourceWS->getSpectrumToWorkspaceIndexMap();
  detid2index_map sourceDetMap =
      sourceWS->getDetectorIDToWorkspaceIndexMap(false);

  std::multimap<size_t, Mantid::detid_t> spectr2index_map;
  for (auto it = sourceDetMap.begin(); it != sourceDetMap.end(); it++) {
    spectr2index_map.insert(
        std::pair<size_t, Mantid::detid_t>(it->second, it->first));
  }
  spec2index_map new_map;
  for (size_t i = 0; i < maskedSpecID.size(); i++) {
    // find spectra number from spectra ID for the source workspace
    const auto itSpec = s2imap.find(maskedSpecID[i]);
    if (itSpec == s2imap.end()) {
      throw std::runtime_error(
          "Can not find spectra with ID: " +
          boost::lexical_cast<std::string>(maskedSpecID[i]) +
          " in the workspace" + sourceWS->getName());
    }
    size_t specN = itSpec->second;

    // find detector range related to this spectra id in the source workspace
    const auto source_range = spectr2index_map.equal_range(specN);
    if (source_range.first == spectr2index_map.end()) {
      throw std::runtime_error("Can not find spectra N: " +
                               boost::lexical_cast<std::string>(specN) +
                               " in the workspace" + sourceWS->getName());
    }
    // add detectors to the masked det-id list
    for (auto it = source_range.first; it != source_range.second; ++it) {
      singleDetIds.push_back(it->second);
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Initialize the Mask Workspace with instrument
 */
void LoadMask::intializeMaskWorkspace() {

  if (m_sourceMapWS) {
    m_maskWS = DataObjects::MaskWorkspace_sptr(
        new DataObjects::MaskWorkspace(m_sourceMapWS->getInstrument()));
  } else {
    MatrixWorkspace_sptr tempWs(new DataObjects::Workspace2D());
    const bool ignoreDirs(true);
    const std::string idfPath = API::FileFinder::Instance().getFullPath(
        m_instrumentPropValue, ignoreDirs);

    auto loadInst = createChildAlgorithm("LoadInstrument");
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", tempWs);

    if (idfPath.empty())
      loadInst->setPropertyValue("InstrumentName", m_instrumentPropValue);
    else
      loadInst->setPropertyValue("Filename", m_instrumentPropValue);

    loadInst->setProperty("RewriteSpectraMap",
                          Mantid::Kernel::OptionalBool(false));
    loadInst->executeAsChildAlg();

    if (!loadInst->isExecuted()) {
      g_log.error() << "Unable to load Instrument " << m_instrumentPropValue
                    << '\n';
      throw std::invalid_argument(
          "Incorrect instrument name or invalid IDF given.");
    }

    m_maskWS = DataObjects::MaskWorkspace_sptr(
        new DataObjects::MaskWorkspace(tempWs->getInstrument()));
  }
  m_maskWS->setTitle("Mask");
}

/**Validates if either input workspace or instriment name is defined
@return the inconsistency between Instrument/Workspace properties or empty list
if no errors is found.
*/
std::map<std::string, std::string> LoadMask::validateInputs() {

  std::map<std::string, std::string> result;

  API::MatrixWorkspace_sptr inputWS = getProperty("RefWorkspace");
  std::string InstrName = getProperty("Instrument");

  if (inputWS) {
    try {
      auto inst = inputWS->getInstrument();
      std::string Name = inst->getName();
      if (Name != InstrName) {
        result["RefWorkspace"] =
            "If both workspace and instrument name are defined, "
            "workspace has to have the instrument with the same name";
      }
    } catch (Kernel::Exception::NotFoundError &) {
      result["RefWorkspace"] =
          "If workspace is defined, it mast have an instrument";
    }
  }

  return result;
}

} // namespace Mantid
} // namespace DataHandling
