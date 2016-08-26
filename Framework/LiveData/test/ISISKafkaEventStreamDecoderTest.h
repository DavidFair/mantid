#ifndef MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODERTEST_H_
#define MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidLiveData/ISIS/ISISKafkaEventStreamDecoder.h"
#include "MantidLiveData/Kafka/KafkaBroker.h"

#include <thread>

class ISISKafkaEventStreamDecoderTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ISISKafkaEventStreamDecoderTest *createSuite() {
    return new ISISKafkaEventStreamDecoderTest();
  }
  static void destroySuite(ISISKafkaEventStreamDecoderTest *suite) {
    delete suite;
  }

  void test_Live() {
    using namespace Mantid::LiveData;

    KafkaBroker broker("sakura");
    ISISKafkaEventStreamDecoder streamer(broker, "SANS2Devent_data",
                                         "SANS2Drun_data", "SANS2Dspdet_data");
    streamer.startCapture();
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    streamer.stopCapture();
  }
};

#endif /* MANTID_LIVEDATA_ISISKAFKAEVENTSTREAMDECODERTEST_H_ */
