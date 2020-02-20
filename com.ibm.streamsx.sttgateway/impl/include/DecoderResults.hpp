// **********************************************************************
// * Copyright (C)2020, International Business Machines Corporation and *
// * others. All Rights Reserved.                                       *
// **********************************************************************

#ifndef COM_IBM_STREAMS_STTGATEWAY_DECODER_RESULTS_H_
#define COM_IBM_STREAMS_STTGATEWAY_DECODER_RESULTS_H_

#include "DecoderCommons.hpp"
#include <vector>

namespace com { namespace ibm { namespace streams { namespace sttgateway {

class DecoderResults : public DecoderFinal, public DecoderAlternatives {
private:
	rapidjson::SizeType size_;

public:
	rapidjson::SizeType getSize() { return size_; }

protected:
	DecoderResults(std::string const & inp, bool completeResults_) : DecoderFinal(inp, completeResults_), DecoderAlternatives(inp, completeResults_),
			size_(0) { }

	void doWork() {
		const rapidjson::Value * value_ = getOptionalMember<ArrayLabel>(jsonDoc, "result", "universe");
		if (value_) {
			size_ = value_->Size();
			if (not completeResults) {
				if (size_ > 1) {
					SPLAPPTRC(L_ERROR, "We expect partial results but dimension of results is " << size_, WATSON_DECODER);
				}
			}
			for (rapidjson::SizeType i = 0; i < size_; i++) {
				std::stringstream ppname;
				ppname << "result[" << i << "]";
				DecoderFinal::doWork(value_[i], ppname.str().c_str(), i);
				DecoderAlternatives::doWork(value_[i], ppname.str().c_str(), i);
			}
		} else {
			size_ = 0;
		}
		value_ = getOptionalMember<ArrayLabel>(jsonDoc, "speaker_labels", "universe");
		size_ = value_->GetArray().Size();
		for (size_t i = 0; i < size_; i++) {
			rapidjson::Value const & speaker = value_[i];
			if (not speaker.IsObject()) {
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
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_RESULTS_H_ */
