// **********************************************************************
// * Copyright (C)2020, International Business Machines Corporation and *
// * others. All Rights Reserved.                                       *
// **********************************************************************

#ifndef COM_IBM_STREAMS_STTGATEWAY_DECODER_SPEAKER_LABELS_H_
#define COM_IBM_STREAMS_STTGATEWAY_DECODER_SPEAKER_LABELS_H_

#include "DecoderCommons.hpp"

namespace com { namespace ibm { namespace streams { namespace sttgateway {

class DecoderSpeakerLabels : public virtual DecoderCommons {
private:
	rapidjson::SizeType size_;
	SPL::list<SPL::float64> from_;
	SPL::list<SPL::int32> speaker_;
	SPL::list<SPL::float64> confidence_;

public:
	bool                            hasResult() const noexcept  { return size_ > 0; }
	rapidjson::SizeType             getSize() const noexcept    { return size_; }
	const SPL::list<SPL::float64> & getFrom() const noexcept    { return from_; }
	const SPL::list<SPL::int32> &   getSpeaker() const noexcept { return speaker_; }
	const SPL::list<SPL::float64> & getConfidence() const noexcept { return confidence_; }

protected:
	DecoderSpeakerLabels(const WatsonSTTConfig & config) :
			DecoderCommons(config),
			size_(0), from_(), speaker_(), confidence_() {
	}

	void reset() {
		size_ = 0;
		from_.clear();
		speaker_.clear();
		confidence_.clear();
	}

	void doWork() {
		const rapidjson::Value * speakerLabels = getOptionalMember<ArrayLabel>(jsonDoc, "speaker_labels", "universe");
		if (speakerLabels) {
			size_ = speakerLabels->Size();
			std::string parentName("universe#speaker_labels");
			for (rapidjson::SizeType i = 0; i < size_; i++) {
				std::stringstream ppname;
				ppname << parentName << "[" << i << "]";
				rapidjson::Value const & speaker = (*speakerLabels)[i];
				if ( ! speaker.IsObject()) {
					throw DecoderException("Speaker is not an Object. Parent: " + ppname.str() + " in json=" + *json);
				}
				const rapidjson::Value & from = getRequiredMember<NumberLabel>(speaker, "from", ppname.str().c_str());
				const rapidjson::Value & spk = getRequiredMember<IntegerLabel>(speaker, "speaker", ppname.str().c_str());
				const rapidjson::Value & conf = getRequiredMember<NumberLabel>(speaker, "confidence", ppname.str().c_str());
				from_.pushBack(SPL::float64(from.GetDouble()));
				speaker_.pushBack(SPL::int32(spk.GetInt()));
				confidence_.pushBack(SPL::float64(conf.GetDouble()));
			}
		}
	}
};

}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_SPEAKER_LABELS_H_ */
