// **********************************************************************
// * Copyright (C)2020, International Business Machines Corporation and *
// * others. All Rights Reserved.                                       *
// **********************************************************************

#ifndef COM_IBM_STREAMS_STTGATEWAY_DECODER_H_
#define COM_IBM_STREAMS_STTGATEWAY_DECODER_H_

#include "DecoderError.hpp"
#include "DecoderState.hpp"
#include "DecoderResults.hpp"
#include "DecoderResultIndex.hpp"
#include "DecoderSpeakerLabels.hpp"

namespace com { namespace ibm { namespace streams { namespace sttgateway {

class Decoder : public DecoderState, public DecoderError, public DecoderResults, public DecoderResultIndex,  public DecoderSpeakerLabels {
public:
	Decoder(const WatsonSTTConfig & config):
			DecoderCommons(config), //Virtual member must be initialized here
			DecoderState(config),
			DecoderError(config),
			DecoderResults(config),
			DecoderResultIndex(config),
			DecoderSpeakerLabels(config) {
	}

	// Cleans, set the pointer to inp and decode
	void doWork(std::string const & inp) {

		DecoderResults::reset();
		DecoderSpeakerLabels::reset();

		DecoderCommons::doWorkStart(inp);
		DecoderError::doWork();
		DecoderState::doWork();
		DecoderResults::doWork();
		DecoderSpeakerLabels::doWork();

		SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec stateListening: " << isLitsening(), WATSON_DECODER);
		if (DecoderError::hasResult())
			SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec error: " << DecoderError::getResult(), WATSON_DECODER);
		if (DecoderResultIndex::hasResult())
			SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec resultIndex: " << DecoderResultIndex::getResult(), WATSON_DECODER);
		if (DecoderResults::hasResult()) {
			for (rapidjson::SizeType i = 0; i < DecoderResults::getSize(); i++) {
				SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec final: " << DecoderResults::DecoderFinal::getResult(i), WATSON_DECODER);
			}

			SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec confidence: " << DecoderResults::DecoderAlternatives::getConfidence(), WATSON_DECODER);
			SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec utt: " << DecoderResults::DecoderAlternatives::getUtteranceText(), WATSON_DECODER);
			SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec alt: " << DecoderResults::DecoderAlternatives::getUtteranceAlternatives(), WATSON_DECODER);
			SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec words: " << DecoderResults::DecoderAlternatives::getUtteranceWords(), WATSON_DECODER);
			SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec wordConf: " << DecoderResults::DecoderAlternatives::getUtteranceWordsConfidences(), WATSON_DECODER);
			SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec wordStart: " << DecoderResults::DecoderAlternatives::getUtteranceWordsStartTimes(), WATSON_DECODER);
			SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec wordEnd: " << DecoderResults::DecoderAlternatives::getUtteranceWordsEndTimes(), WATSON_DECODER);
		}
		if (DecoderSpeakerLabels::hasResult()) {
			SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec spk from: " << DecoderSpeakerLabels::getFrom(), WATSON_DECODER);
			SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec spk: " << DecoderSpeakerLabels::getSpeaker(), WATSON_DECODER);
			SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec spk conf: " << DecoderSpeakerLabels::getConfidence(), WATSON_DECODER);
		}
		if (DecoderWordAlternatives::hasResult()) {
			SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec confusion st: " << DecoderWordAlternatives::getWordAlternativesStartTimes(), WATSON_DECODER);
			SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec confusion end: " << DecoderWordAlternatives::getWordAlternativesEndTimes(), WATSON_DECODER);
			SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec confusion conf: " << DecoderWordAlternatives::getWordAlternativesConfidences(), WATSON_DECODER);
			SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec confusion word: " << DecoderWordAlternatives::getWordAlternatives(), WATSON_DECODER);
		}
		if (DecoderKeywordsResult::hasResult()) {
			SPLAPPTRC(L_TRACE, configuration.traceIntro << "-->dec keywords: " << DecoderKeywordsResult::getKeywordsSpottingResults(), WATSON_DECODER);
		}

		DecoderCommons::doWorkEnd();
	}
};

}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_H_ */
