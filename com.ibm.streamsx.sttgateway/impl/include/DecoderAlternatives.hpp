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

public:
	bool hasResult() { return alternativesSize != 0; }
	SPL::rstring            getUtteranceText() { return utteranceText; }
	SPL::list<SPL::rstring> getUtteranceAlternatives() { return utteranceAlternatives; }
	SPL::float64            getConfidence() { return confidence; }
	SPL::list<SPL::rstring> getUtteranceWords() { return utteranceWords; }
	SPL::list<SPL::float64> getUtteranceWordsConfidences() { return utteranceWordsConfidences; }
	SPL::list<SPL::float64> getUtteranceWordsStartTimes() { return utteranceWordsStartTimes; }
	SPL::list<SPL::float64> getUtteranceWordsEndTimes() { return utteranceWordsEndTimes; }

protected:
	DecoderAlternatives(const WatsonSTTConfig & config) :
		DecoderCommons(config),
		alternativesSize(0), utteranceText(), utteranceAlternatives(), confidence(0.0),
		utteranceWords(), utteranceWordsStartTimes(),
		utteranceWordsEndTimes() {
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
	}

	void doWork(const rapidjson::Value& result, const std::string & parentName, rapidjson::SizeType resultIndex, bool final);

private:
	void doWorkTimestamps(const rapidjson::Value& result, const std::string & parentName, rapidjson::SizeType resultIndex);
	void doWorkWordConfidence(const rapidjson::Value& result, const std::string & parentName, rapidjson::SizeType resultIndex);
};


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
		if (configuration.sttResultMode != WatsonSTTConfig::complete) {
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
			if ((configuration.sttResultMode != WatsonSTTConfig::complete) && (resultIndex == 0)) {
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
				std::cout << "ts index: " << i << timestamp[0].GetString() << "," << timestamp[1].GetDouble() << "," << timestamp[2].GetDouble() << std::endl;
				//utteranceWords.pushBack(timestamp[0].GetString());
				utteranceWordsStartTimes.pushBack(timestamp[1].GetDouble());
				utteranceWordsEndTimes.pushBack(timestamp[2].GetDouble());
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
				utteranceWords.pushBack(SPL::rstring(wordConfidence[0].GetString()));
				utteranceWordsConfidences.pushBack(wordConfidence[1].GetDouble());
			}
		}
	}
}
}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_ALTERNATIVES_H_ */
