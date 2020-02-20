// **********************************************************************
// * Copyright (C)2020, International Business Machines Corporation and *
// * others. All Rights Reserved.                                       *
// **********************************************************************

#ifndef COM_IBM_STREAMS_STTGATEWAY_DECODER_ALTERNATIVES_H_
#define COM_IBM_STREAMS_STTGATEWAY_DECODER_ALTERNATIVES_H_

#include "DecoderCommons.hpp"
#include <vector>

namespace com { namespace ibm { namespace streams { namespace sttgateway {

class DecoderAlternatives : public virtual DecoderCommons {
private:
	SPL::rstring utteranceText;
	SPL::list<SPL::rstring> utteranceAlternatives;
	SPL::float64 confidence;
	SPL::list<SPL::rstring> utteranceWords;
	SPL::list<SPL::float64> utteranceWordsStartTimes;
	SPL::list<SPL::float64> utteranceWordsEndTimes;

public:
	SPL::rstring            getUtteranceText() { return utteranceText; }
	SPL::list<SPL::rstring> getUtteranceAlternatives() { return utteranceAlternatives; }
	SPL::float64            getConfidence() { return confidence; }
	SPL::list<SPL::rstring> getUtteranceWords() { return utteranceWords; }
	SPL::list<SPL::float64> getUtteranceWordsStartTimes() { return utteranceWordsStartTimes; }
	SPL::list<SPL::float64> getUtteranceWordsEndTimes() { return utteranceWordsEndTimes; }

protected:
	DecoderAlternatives(std::string const & inp, bool completeResults_) : DecoderCommons(inp, completeResults_),
		utteranceText(), utteranceAlternatives(), confidence(0.0), utteranceWords(), utteranceWordsStartTimes(),
		utteranceWordsEndTimes() { }

	void doWork(const rapidjson::Value& result, const char* parentName, rapidjson::SizeType index) {
		const rapidjson::Value& alternatives = getRequiredMember<ArrayLabel>(result, "alternatives", parentName);
		rapidjson::SizeType alternativesSize = alternatives.Size();
		for (rapidjson::SizeType i = 0; i < alternativesSize; i++) {
			std::stringstream ppname;
			ppname << parentName << "[" << i << "]";
			const rapidjson::Value * transcriptVal = getRequiredMember<StringLabel>(alternatives[i], "transcript", ppname.str().c_str());
			const rapidjson::Value * confidenceVal = getOptionalMember<NumberLabel>(alternatives[i], "confidence", ppname.str().c_str());
			const rapidjson::Value * timestampsVal = getOptionalMember<ArrayLabel>(alternatives[i], "timestamps", ppname.str().c_str());
			const rapidjson::Value * word_confidenceVal = getRequiredMember<ArrayLabel>(alternatives[i], "transcript", ppname.str().c_str());

			if (index == 0) {
				if ( i == 0) {
					utteranceText = transcriptVal->GetString();
					if (confidenceVal) {
						confidence = confidenceVal->GetDouble();
					} else {
						throw DecoderException("No confidence in alternative 0 " + ppname.str() + " json:" + json);
					}
				} else {
					utteranceAlternatives.pushBack(transcriptVal->GetString());
				}

			} else {
				if (not completeResults_)
					throw
				if ( i == 0) {
					utteranceText = utteranceText + transcriptVal->GetString();
				}
			}
			if (i == 0) {

			}
		}
	}
};

}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_ALTERNATIVES_H_ */
