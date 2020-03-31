// **********************************************************************
// * Copyright (C)2020, International Business Machines Corporation and *
// * others. All Rights Reserved.                                       *
// **********************************************************************

#ifndef COM_IBM_STREAMS_STTGATEWAY_DECODER_RESULTS_H_
#define COM_IBM_STREAMS_STTGATEWAY_DECODER_RESULTS_H_

#include "DecoderCommons.hpp"
#include "DecoderFinal.hpp"
#include "DecoderAlternatives.hpp"
#include "DecoderKeywordResults.hpp"
#include "DecoderWordAlternatives.hpp"
#include <vector>

namespace com { namespace ibm { namespace streams { namespace sttgateway {

class DecoderResults : public DecoderFinal, public DecoderAlternatives, public DecoderKeywordsResult, public DecoderWordAlternatives {
private:
	rapidjson::SizeType resultsSize;

public:
	bool hasResult() { return resultsSize != 0; }

	rapidjson::SizeType getSize() { return resultsSize; }

protected:
	DecoderResults(const WatsonSTTConfig & config):
			DecoderCommons(config),
			DecoderFinal(config),
			DecoderAlternatives(config),
			DecoderKeywordsResult(config),
			DecoderWordAlternatives(config),
			resultsSize(0) {
	}

	void reset() {
		DecoderWordAlternatives::reset();
		DecoderKeywordsResult::reset();
		DecoderAlternatives::reset();
		DecoderFinal::reset();
		resultsSize = 0;
	}

	// Decode: final, Alternatives, Keyword results and word alternatives
	// final -> vector of resultsSize of all result finals
	// alternatives -> decoding for final results and in mode WatsonSTTConfig::partial
	// DecoderKeywordsResult -> decoding for final results only and concatenate all results
	// DecoderWordAlternatives -> decoding for final results only and concatenate all results
	void doWork() {
		const rapidjson::Value * results = getOptionalMember<ArrayLabel>(jsonDoc, "results", "universe");
		if (results) {
			resultsSize = results->Size();
			if (configuration.sttOutputResultMode != WatsonSTTConfig::complete) {
				if (resultsSize > 1) {
					SPLAPPTRC(L_ERROR, "We expect partial results but dimension of results is " << resultsSize, WATSON_DECODER);
				}
			}
			for (rapidjson::SizeType i = 0; i < resultsSize; i++) {
				std::stringstream ppname;
				ppname << "results[" << i << "]";
				const rapidjson::Value & result = (*results)[i];
				if (not result.IsObject()) {
					std::stringstream ss; ss << "Result is not an Object. " << ppname << " in json=" << *json;
					throw DecoderException(ss.str());
				}
				DecoderFinal::doWork(result, ppname.str().c_str());
				bool final = DecoderFinal::getResult(i);
				if (final || (configuration.sttOutputResultMode == WatsonSTTConfig::partial)) {
					DecoderAlternatives::doWork(result, ppname.str().c_str(), i, final);
				}
				if (final) {
					DecoderWordAlternatives::doWork(result, ppname.str().c_str(), i);
					DecoderKeywordsResult::doWork(result, ppname.str().c_str(), i);
				}
			}
		} else {
			resultsSize = 0;
		}
	}
};

}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_RESULTS_H_ */
