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
	rapidjson::SizeType alternativesSize;
	SPL::rstring utteranceText;
	SPL::list<SPL::rstring> utteranceAlternatives;
	SPL::float64 confidence; // confidence for alternative index 0
	SPL::list<SPL::rstring> utteranceWords;
	SPL::list<SPL::float64> utteranceWordsConfidences;
	SPL::list<SPL::float64> utteranceWordsStartTimes;
	SPL::list<SPL::float64> utteranceWordsEndTimes;
	SPL::float64 utteranceStartTime;
	SPL::float64 utteranceEndTime;

public:
	bool                            hasResult() const noexcept                    { return alternativesSize != 0; }
	const SPL::rstring &            getUtteranceText() const noexcept             { return utteranceText; }
	const SPL::list<SPL::rstring> & getUtteranceAlternatives() const noexcept     { return utteranceAlternatives; }
	const SPL::float64 &            getConfidence() const noexcept                { return confidence; }
	const SPL::list<SPL::rstring> & getUtteranceWords() const noexcept            { return utteranceWords; }
	const SPL::list<SPL::float64> & getUtteranceWordsConfidences() const noexcept { return utteranceWordsConfidences; }
	const SPL::list<SPL::float64> & getUtteranceWordsStartTimes() const noexcept  { return utteranceWordsStartTimes; }
	const SPL::list<SPL::float64> & getUtteranceWordsEndTimes() const noexcept    { return utteranceWordsEndTimes; }
	const SPL::float64 &            getUtteranceStartTime() const noexcept        { return utteranceStartTime; }
	const SPL::float64 &            getUtteranceEndTime() const noexcept          { return utteranceEndTime; }

protected:
	DecoderAlternatives(const WatsonSTTConfig & config) :
		DecoderCommons(config),
		alternativesSize(0), utteranceText(), utteranceAlternatives(), confidence(0.0),
		utteranceWords(), utteranceWordsStartTimes(),
		utteranceWordsEndTimes(), utteranceStartTime(0.0), utteranceEndTime(0.0) {
	}

	void reset() {
		alternativesSize = 0;
		utteranceText.clear();
		utteranceAlternatives.clear();
		confidence = 0.0;
		utteranceWords.clear();
		utteranceWordsConfidences.clear();
		utteranceWordsStartTimes.clear();
		utteranceWordsEndTimes.clear();
		utteranceStartTime = 0.0;
		utteranceEndTime = 0.0;
	}

	void doWork(const rapidjson::Value& result, const std::string & parentName, rapidjson::SizeType resultIndex, bool final);

private:
	void doWorkTimestamps(const rapidjson::Value& result, const std::string & parentName, rapidjson::SizeType resultIndex);
	void doWorkWordConfidence(const rapidjson::Value& result, const std::string & parentName, rapidjson::SizeType resultIndex);
};

// Decode confidence: not delivered in sttResultMode == WatsonSTTConfig::complete
//		and only for the first alternative in the first result
// Decode utteranceText: concatenate all first alternatives in all results
// Decode alternatives: decode only in mode sttResultMode != WatsonSTTConfig::complete
//						and for the first result! no concatenation
// Decode timestamps: concatenate all results of the first alternative
// Decode word confidences: concatenate all final results of the first alternative
void DecoderAlternatives::doWork(const rapidjson::Value& result, const std::string & parentName, rapidjson::SizeType resultIndex, bool final) {
	const rapidjson::Value & alternatives = getRequiredMember<ArrayLabel>(result, "alternatives", parentName.c_str());
	alternativesSize = alternatives.Size();
	std::string parentNameAlt = parentName + "#alternatives";
	for (rapidjson::SizeType i = 0; i < alternativesSize; i++) {
		std::stringstream ppname;
		ppname << parentNameAlt << "[" << i << "]";

		const rapidjson::Value & alternative = alternatives[i];
		if (not alternative.IsObject()) {
			throw DecoderException("alternative is not an Object. " + ppname.str() + " in json=" + *json);
		}

		// Decode confidence: Confidence is evaluated only if partial utterances are requested
		if (configuration.sttOutputResultMode != WatsonSTTConfig::complete) {
			if (resultIndex == 0) {
				if (i == 0) {
					const rapidjson::Value * confidenceVal = getOptionalMember<NumberLabel>(alternative, "confidence", ppname.str().c_str());
					if (confidenceVal) {
						confidence = confidenceVal->GetDouble();
					} else {
						confidence = -1.0;
					}
				}
			} else {
				// if we are requesting not completeResults it should not happen that there are more than one result
				SPLAPPTRC(L_ERROR, "partial utterances requested but resultIndex > 0. in json" << *json, DecoderCommons::WATSON_DECODER);
			}
		}
		// transcript
		const rapidjson::Value & transcriptVal = getRequiredMember<StringLabel>(alternative, "transcript", ppname.str().c_str());
		if (i == 0) { //utterance text
			utteranceText.append(transcriptVal.GetString());
		} else {
			// alternatives are collected only if not completeResults
			// if we are requesting not completeResults it should not happen that there are more than one result
			if ((configuration.sttOutputResultMode != WatsonSTTConfig::complete) && (resultIndex == 0)) {
				utteranceAlternatives.pushBack(SPL::rstring(transcriptVal.GetString()));
			}
		}
		// timestamps
		if (i == 0) {
			doWorkTimestamps(alternative, ppname.str().c_str(), resultIndex);
			if (final) {
				doWorkWordConfidence(alternative, ppname.str().c_str(), resultIndex);
			}
		}
	}

}

void DecoderAlternatives::doWorkTimestamps(const rapidjson::Value& alternative, const std::string & parentName, rapidjson::SizeType resultIndex) {
	const rapidjson::Value * timestamps = getOptionalMember<ArrayLabel>(alternative, "timestamps", parentName.c_str());
	if (timestamps) {
		rapidjson::SizeType size = timestamps->Size();
		std::string parentNameTim = parentName + "#timestamps";
		for (rapidjson::SizeType i = 0; i < size; i++) {
			std::stringstream ppname;
			ppname << parentNameTim << "[" << i << "]";

			const rapidjson::Value & timestamp = (*timestamps)[i];

			if (timestamp.Size() != 3) {
				throw DecoderException("timestamp size is not 3 " + ppname.str() + " json:" + *json);
			} else {
				//std::cout << "ts index: " << i << timestamp[0].GetString() << "," << timestamp[1].GetDouble() << "," << timestamp[2].GetDouble() << std::endl;

				// take words from timestamps because these are available also for non final utterances
				// confidences are not available for non final utterances
				utteranceWords.pushBack(SPL::rstring(timestamp[0].GetString()));
				utteranceWordsStartTimes.pushBack(timestamp[1].GetDouble());
				utteranceWordsEndTimes.pushBack(timestamp[2].GetDouble());

				// set start time from the very first word and set floating end time
				if ((resultIndex == 0) && (i==0))
					utteranceStartTime = timestamp[1].GetDouble();
				utteranceEndTime = timestamp[2].GetDouble();
			}
		}
	}
}

void DecoderAlternatives::doWorkWordConfidence(const rapidjson::Value& alternative, const std::string & parentName, rapidjson::SizeType resultIndex) {
	const rapidjson::Value * wordConfidences = getOptionalMember<ArrayLabel>(alternative, "word_confidence", parentName.c_str());
	if (wordConfidences) {
		rapidjson::SizeType size = wordConfidences->Size();
		std::string parentNameConfd = parentName + "#word_confidence";
		for (rapidjson::SizeType i = 0; i < size; i++) {
			std::stringstream ppname;
			ppname << parentNameConfd << "[" << i << "]";

			const rapidjson::Value & wordConfidence = (*wordConfidences)[i];

			if (wordConfidence.Size() != 2) {
				throw DecoderException("wordConfidence size is not 2 " + ppname.str() + " in json:" + *json);
			} else {
				//utteranceWords.pushBack(SPL::rstring(wordConfidence[0].GetString()));
				utteranceWordsConfidences.pushBack(wordConfidence[1].GetDouble());
			}
		}
	}
}
}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_ALTERNATIVES_H_ */
