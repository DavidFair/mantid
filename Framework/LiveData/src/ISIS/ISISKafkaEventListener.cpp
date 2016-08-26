#include "MantidLiveData/ISIS/ISISKafkaEventListener.h"
#include "MantidLiveData/ISIS/ISISKafkaEventStreamDecoder.h"
#include "MantidLiveData/Kafka/KafkaBroker.h"

#include "MantidAPI/LiveListenerFactory.h"

namespace {
Mantid::Kernel::Logger g_log("ISISKafkaEventListener");
}

namespace Mantid {
namespace LiveData {

DECLARE_LISTENER(ISISKafkaEventListener)

/// @copydoc ILiveListener::connect
bool ISISKafkaEventListener::connect(
    const Poco::Net::SocketAddress &address,
    const API::ILiveListener::ConnectionArgs &args) {
  KafkaBroker broker(address.toString());
  try {
    const std::string eventTopic(args.instrumentName + "event_data"),
        runInfoTopic(args.instrumentName + "run_data"),
        spDetInfoTopic(args.instrumentName + "spdet_data");
    m_decoder = Kernel::make_unique<ISISKafkaEventStreamDecoder>(
        broker, eventTopic, runInfoTopic, spDetInfoTopic);
  } catch (std::exception &exc) {
    g_log.error() << "ISISKafkaEventListener::connect - Connection Error: "
                  << exc.what() << "\n";
    return false;
  }
  return true;
}

/// @copydoc ILiveListener::start
void ISISKafkaEventListener::start(Kernel::DateAndTime startTime) {
  UNUSED_ARG(startTime);
  m_decoder->startCapture();
}

/// @copydoc ILiveListener::extractData
boost::shared_ptr<API::Workspace> ISISKafkaEventListener::extractData() {
  if (isConnected()) {
    return m_decoder->extractData();
  }
  throw std::runtime_error("ISISKafkaEventListener::extractData() - Cannot "
                           "extract data, listener not connected.");
}

/// @copydoc ILiveListener::isConnected
bool ISISKafkaEventListener::isConnected() {
  return (m_decoder ? m_decoder->isRunning() : false);
}

/// @copydoc ILiveListener::runStatus
API::ILiveListener::RunStatus ISISKafkaEventListener::runStatus() {
  return Running;
}

/// @copydoc ILiveListener::runNumber
int ISISKafkaEventListener::runNumber() const {
  return (m_decoder ? m_decoder->runNumber() : -1);
}
}
}
