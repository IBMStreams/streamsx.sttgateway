// **********************************************************************
// * Copyright (C)2020, International Business Machines Corporation and *
// * others. All Rights Reserved.                                       *
// **********************************************************************

#ifndef COM_IBM_STREAMS_STTGATEWAY_DECODER_SPEAKER_LABELS_H_
#define COM_IBM_STREAMS_STTGATEWAY_DECODER_SPEAKER_LABELS_H_

#include "DecoderCommons.hpp"
#include <vector>

namespace com { namespace ibm { namespace streams { namespace sttgateway {

class DecoderSpeakerLabels : public virtual DecoderCommons {
private:
	const rapidjson::Value* value_;
	size_t size_;
	SPL::list<SPL::float64> from_;
	SPL::list<SPL::int32> speaker_;
	SPL::list<SPL::float64> confidence_;

public:
	DecoderSpeakerLabels(std::string const & inp) : DecoderCommons(inp), value_(nullptr), size_(0),
			from_(), speaker_(), confidence_() { }

	bool hasResult() { return value_ != nullptr; }

	size_t getSize() { return size_; }

	SPL::list<SPL::float64> getFrom() { return from_; }

	SPL::list<SPL::int32>   getSpeaker() { return speaker_; }

	SPL::list<SPL::float64> getConfidence() { return confidence_; }

protected:
	void doWork() {
		value_ = getOptionalMember<ArrayLabel>(jsonDoc, "speaker_labels", "universe");
		size_ = value_->GetArray().Size();
		for (size_t i = 0; i < size_; i++) {
			rapidjson::Value const & speaker = value_[i];
			if ( ! speaker.IsObject()) {
				std::stringstream ss; ss << "Speaker is not an Object. Index:" << i << " in " << " json=" << json;
				throw DecoderException(ss.str());
			}
			from_.pushBack(SPL::float64(getRequiredMember<NumberLabel>(speaker, "from", "speaker")));
			speaker_.pushBack(SPL::int32(getRequiredMember<IntegerLabel>(speaker, "speaker", "speaker")));
			confidence_.pushBack(SPL::float64(getRequiredMember<NumberLabel>(speaker, "confidence", "speaker")));
		}
	}
};

}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_SPEAKER_LABELS_H_ */
