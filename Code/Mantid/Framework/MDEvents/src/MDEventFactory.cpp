/* Auto-generated by '/home/8oz/Code/Mantid/Code/Mantid/Framework/MDEvents/src/generate_mdevent_declarations.py' 
 *     on 2011-03-23 17:53:14.043487
 *
 * DO NOT EDIT!
 */ 
 
#include <boost/shared_ptr.hpp>
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"
 
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/MDBin.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/IMDBox.h"

// We need to include the .cpp files so that the declarations are picked up correctly. Weird, I know. 
// See http://www.parashift.com/c++-faq-lite/templates.html#faq-35.13 
#include "MDBox.cpp"
#include "MDEventWorkspace.cpp"
#include "MDGridBox.cpp"
#include "MDBin.cpp"


namespace Mantid
{
namespace MDEvents
{
// Instantiations for MDEvent
template DLLExport class MDEvent<1>;
template DLLExport class MDEvent<2>;
template DLLExport class MDEvent<3>;
template DLLExport class MDEvent<4>;
template DLLExport class MDEvent<5>;
template DLLExport class MDEvent<6>;
template DLLExport class MDEvent<7>;
template DLLExport class MDEvent<8>;
template DLLExport class MDEvent<9>;


// Instantiations for MDBox
template DLLExport class MDBox<MDEvent<1>, 1>;
template DLLExport class MDBox<MDEvent<2>, 2>;
template DLLExport class MDBox<MDEvent<3>, 3>;
template DLLExport class MDBox<MDEvent<4>, 4>;
template DLLExport class MDBox<MDEvent<5>, 5>;
template DLLExport class MDBox<MDEvent<6>, 6>;
template DLLExport class MDBox<MDEvent<7>, 7>;
template DLLExport class MDBox<MDEvent<8>, 8>;
template DLLExport class MDBox<MDEvent<9>, 9>;


// Instantiations for IMDBox
template DLLExport class IMDBox<MDEvent<1>, 1>;
template DLLExport class IMDBox<MDEvent<2>, 2>;
template DLLExport class IMDBox<MDEvent<3>, 3>;
template DLLExport class IMDBox<MDEvent<4>, 4>;
template DLLExport class IMDBox<MDEvent<5>, 5>;
template DLLExport class IMDBox<MDEvent<6>, 6>;
template DLLExport class IMDBox<MDEvent<7>, 7>;
template DLLExport class IMDBox<MDEvent<8>, 8>;
template DLLExport class IMDBox<MDEvent<9>, 9>;


// Instantiations for MDGridBox
template DLLExport class MDGridBox<MDEvent<1>, 1>;
template DLLExport class MDGridBox<MDEvent<2>, 2>;
template DLLExport class MDGridBox<MDEvent<3>, 3>;
template DLLExport class MDGridBox<MDEvent<4>, 4>;
template DLLExport class MDGridBox<MDEvent<5>, 5>;
template DLLExport class MDGridBox<MDEvent<6>, 6>;
template DLLExport class MDGridBox<MDEvent<7>, 7>;
template DLLExport class MDGridBox<MDEvent<8>, 8>;
template DLLExport class MDGridBox<MDEvent<9>, 9>;


// Instantiations for MDEventWorkspace
template DLLExport class MDEventWorkspace<MDEvent<1>, 1>;
template DLLExport class MDEventWorkspace<MDEvent<2>, 2>;
template DLLExport class MDEventWorkspace<MDEvent<3>, 3>;
template DLLExport class MDEventWorkspace<MDEvent<4>, 4>;
template DLLExport class MDEventWorkspace<MDEvent<5>, 5>;
template DLLExport class MDEventWorkspace<MDEvent<6>, 6>;
template DLLExport class MDEventWorkspace<MDEvent<7>, 7>;
template DLLExport class MDEventWorkspace<MDEvent<8>, 8>;
template DLLExport class MDEventWorkspace<MDEvent<9>, 9>;


// Instantiations for MDBin
template DLLExport class MDBin<MDEvent<1>, 1>;
template DLLExport class MDBin<MDEvent<2>, 2>;
template DLLExport class MDBin<MDEvent<3>, 3>;
template DLLExport class MDBin<MDEvent<4>, 4>;
template DLLExport class MDBin<MDEvent<5>, 5>;
template DLLExport class MDBin<MDEvent<6>, 6>;
template DLLExport class MDBin<MDEvent<7>, 7>;
template DLLExport class MDBin<MDEvent<8>, 8>;
template DLLExport class MDBin<MDEvent<9>, 9>;




/** Create a MDEventWorkspace of the given type
@param nd :: number of dimensions
@param eventType :: string describing the event type (currently ignored) 
*/
API::IMDEventWorkspace_sptr MDEventFactory::CreateMDEventWorkspace(size_t nd, std::string eventType)
{
  (void) eventType; // Avoid compiler warning
  switch(nd)
  {
  case (1):
    return boost::shared_ptr<MDEventWorkspace<MDEvent<1>,1> >(new MDEventWorkspace<MDEvent<1>,1>);
  case (2):
    return boost::shared_ptr<MDEventWorkspace<MDEvent<2>,2> >(new MDEventWorkspace<MDEvent<2>,2>);
  case (3):
    return boost::shared_ptr<MDEventWorkspace<MDEvent<3>,3> >(new MDEventWorkspace<MDEvent<3>,3>);
  case (4):
    return boost::shared_ptr<MDEventWorkspace<MDEvent<4>,4> >(new MDEventWorkspace<MDEvent<4>,4>);
  case (5):
    return boost::shared_ptr<MDEventWorkspace<MDEvent<5>,5> >(new MDEventWorkspace<MDEvent<5>,5>);
  case (6):
    return boost::shared_ptr<MDEventWorkspace<MDEvent<6>,6> >(new MDEventWorkspace<MDEvent<6>,6>);
  case (7):
    return boost::shared_ptr<MDEventWorkspace<MDEvent<7>,7> >(new MDEventWorkspace<MDEvent<7>,7>);
  case (8):
    return boost::shared_ptr<MDEventWorkspace<MDEvent<8>,8> >(new MDEventWorkspace<MDEvent<8>,8>);
  case (9):
    return boost::shared_ptr<MDEventWorkspace<MDEvent<9>,9> >(new MDEventWorkspace<MDEvent<9>,9>);
  default:
    throw std::invalid_argument("Invalid number of dimensions passed to CreateMDEventWorkspace.");
  }
}

} // namespace Mantid
} // namespace MDEvents 

/* THIS FILE WAS AUTO-GENERATED BY /home/8oz/Code/Mantid/Code/Mantid/Framework/MDEvents/src/generate_mdevent_declarations.py - DO NOT EDIT! */ 
