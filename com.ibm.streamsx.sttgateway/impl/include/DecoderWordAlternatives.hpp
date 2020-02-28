// **********************************************************************
// * Copyright (C)2020, International Business Machines Corporation and *
// * others. All Rights Reserved.                                       *
// **********************************************************************

#ifndef COM_IBM_STREAMS_STTGATEWAY_DECODER_WORD_ALTERNATIVES_H_
#define COM_IBM_STREAMS_STTGATEWAY_DECODER_WORD_ALTERNATIVES_H_

#include "DecoderCommons.hpp"
#include <vector>

namespace com { namespace ibm { namespace streams { namespace sttgateway {

class DecoderWordAlternatives : public virtual DecoderCommons {
private:
	rapidjson::SizeType alternativesSize;
	SPL::list<SPL::float64> startTimes;
	SPL::list<SPL::float64> endTimes;
	SPL::list<SPL::list<SPL::rstring> > wordAlternatives;
	SPL::list<SPL::list<SPL::float64> > wordConfidences;

public:
	bool                                        hasResult() const noexcept                      { return alternativesSize != 0; }
	const SPL::list<SPL::float64> &             getWordAlternativesStartTimes() const noexcept  { return startTimes; }
	const SPL::list<SPL::float64> &             getWordAlternativesEndTimes() const noexcept    { return endTimes; }
	const SPL::list<SPL::list<SPL::rstring> > & getWordAlternatives() const noexcept            { return wordAlternatives; }
	const SPL::list<SPL::list<SPL::float64> > & getWordAlternativesConfidences() const noexcept { return wordConfidences; }

protected:
	DecoderWordAlternatives(const WatsonSTTConfig & config) :
		DecoderCommons(config),
		alternativesSize(0), startTimes(), endTimes(), wordAlternatives(), wordConfidences() {
	}

	void reset() {
		alternativesSize = 0;
		startTimes.clear();
		endTimes.clear();
		wordAlternatives.clear();
		wordConfidences.clear();
	}

	void doWork(const rapidjson::Value& result, const std::string & parentName, rapidjson::SizeType resultIndex);
};

void DecoderWordAlternatives::doWork(const rapidjson::Value& result, const std::string & parentName, rapidjson::SizeType resultIndex) {
	const rapidjson::Value * wordAlternatives_ = getOptionalMember<ArrayLabel>(result, "word_alternatives", parentName.c_str());
	if (not wordAlternatives_) {
		alternativesSize = 0;
		return;
	}
	alternativesSize = wordAlternatives_->Size();
	for (rapidjson::SizeType i = 0; i < alternativesSize; i++) {
		std::stringstream ppname;
		ppname << parentName << "#word_alternatives[" << i << "]";

		const rapidjson::Value & wordAlternative_ = (*wordAlternatives_)[i];
		if (not wordAlternative_.IsObject()) {
			throw DecoderException("wordAlternative is not an Object. " + ppname.str() + " in json=" + *json);
		}

		const rapidjson::Value & startTime_ = getRequiredMember<NumberLabel>(wordAlternative_, "start_time", ppname.str().c_str());
		startTimes.pushBack(startTime_.GetDouble());

		const rapidjson::Value & endTime_ = getRequiredMember<NumberLabel>(wordAlternative_, "end_time", ppname.str().c_str());
		endTimes.push_back(endTime_.GetDouble());

		const rapidjson::Value & alternatives_ = getRequiredMember<ArrayLabel>(wordAlternative_, "alternatives", ppname.str().c_str());
		SPL::list<SPL::rstring> words_;
		SPL::list<SPL::float64> confidences_;
		rapidjson::SizeType wordsSize_ = alternatives_.Size();
		for (rapidjson::SizeType j = 0; j < wordsSize_; j++) {
			std::stringstream pppname;
			ppname << parentName << "[" << j << "]";

			const rapidjson::Value & alternative_ = alternatives_[j];
			if (not alternative_.IsObject()) {
				throw DecoderException("alternative is not an Object. " + pppname.str() + " in json=" + *json);
			}
			const rapidjson::Value & word_ = getRequiredMember<StringLabel>(alternative_, "word", pppname.str().c_str());
			words_.push_back(SPL::rstring(word_.GetString()));
			const rapidjson::Value & confidence_ = getRequiredMember<NumberLabel>(alternative_, "confidence", pppname.str().c_str());
			confidences_.push_back(confidence_.GetDouble());
		}
		wordAlternatives.pushBack(words_);
		wordConfidences.pushBack(confidences_);
	}
}
}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_WORD_ALTERNATIVES_H_ */
