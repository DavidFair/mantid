// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_TDCTTIMESTAMPS_H_
#define FLATBUFFERS_GENERATED_TDCTTIMESTAMPS_H_

#include "flatbuffers/flatbuffers.h"

struct timestamp;

struct timestamp FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  static FLATBUFFERS_CONSTEXPR const char *GetFullyQualifiedName() {
    return "timestamp";
  }
  enum {
    VT_NAME = 4,
    VT_TIMESTAMPS = 6,
    VT_SEQUENCE_COUNTER = 8
  };
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  flatbuffers::String *mutable_name() {
    return GetPointer<flatbuffers::String *>(VT_NAME);
  }
  const flatbuffers::Vector<uint64_t> *timestamps() const {
    return GetPointer<const flatbuffers::Vector<uint64_t> *>(VT_TIMESTAMPS);
  }
  flatbuffers::Vector<uint64_t> *mutable_timestamps() {
    return GetPointer<flatbuffers::Vector<uint64_t> *>(VT_TIMESTAMPS);
  }
  uint64_t sequence_counter() const {
    return GetField<uint64_t>(VT_SEQUENCE_COUNTER, 0);
  }
  bool mutate_sequence_counter(uint64_t _sequence_counter) {
    return SetField<uint64_t>(VT_SEQUENCE_COUNTER, _sequence_counter, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffsetRequired(verifier, VT_NAME) &&
           verifier.Verify(name()) &&
           VerifyOffsetRequired(verifier, VT_TIMESTAMPS) &&
           verifier.Verify(timestamps()) &&
           VerifyField<uint64_t>(verifier, VT_SEQUENCE_COUNTER) &&
           verifier.EndTable();
  }
};

struct timestampBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(timestamp::VT_NAME, name);
  }
  void add_timestamps(flatbuffers::Offset<flatbuffers::Vector<uint64_t>> timestamps) {
    fbb_.AddOffset(timestamp::VT_TIMESTAMPS, timestamps);
  }
  void add_sequence_counter(uint64_t sequence_counter) {
    fbb_.AddElement<uint64_t>(timestamp::VT_SEQUENCE_COUNTER, sequence_counter, 0);
  }
  explicit timestampBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  timestampBuilder &operator=(const timestampBuilder &);
  flatbuffers::Offset<timestamp> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<timestamp>(end);
    fbb_.Required(o, timestamp::VT_NAME);
    fbb_.Required(o, timestamp::VT_TIMESTAMPS);
    return o;
  }
};

inline flatbuffers::Offset<timestamp> Createtimestamp(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> name = 0,
    flatbuffers::Offset<flatbuffers::Vector<uint64_t>> timestamps = 0,
    uint64_t sequence_counter = 0) {
  timestampBuilder builder_(_fbb);
  builder_.add_sequence_counter(sequence_counter);
  builder_.add_timestamps(timestamps);
  builder_.add_name(name);
  return builder_.Finish();
}

inline flatbuffers::Offset<timestamp> CreatetimestampDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *name = nullptr,
    const std::vector<uint64_t> *timestamps = nullptr,
    uint64_t sequence_counter = 0) {
  return Createtimestamp(
      _fbb,
      name ? _fbb.CreateString(name) : 0,
      timestamps ? _fbb.CreateVector<uint64_t>(*timestamps) : 0,
      sequence_counter);
}

inline const timestamp *Gettimestamp(const void *buf) {
  return flatbuffers::GetRoot<timestamp>(buf);
}

inline const timestamp *GetSizePrefixedtimestamp(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<timestamp>(buf);
}

inline timestamp *GetMutabletimestamp(void *buf) {
  return flatbuffers::GetMutableRoot<timestamp>(buf);
}

inline const char *timestampIdentifier() {
  return "tdct";
}

inline bool timestampBufferHasIdentifier(const void *buf) {
  return flatbuffers::BufferHasIdentifier(
      buf, timestampIdentifier());
}

inline bool VerifytimestampBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<timestamp>(timestampIdentifier());
}

inline bool VerifySizePrefixedtimestampBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<timestamp>(timestampIdentifier());
}

inline void FinishtimestampBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<timestamp> root) {
  fbb.Finish(root, timestampIdentifier());
}

inline void FinishSizePrefixedtimestampBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<timestamp> root) {
  fbb.FinishSizePrefixed(root, timestampIdentifier());
}

#endif  // FLATBUFFERS_GENERATED_TDCTTIMESTAMPS_H_
