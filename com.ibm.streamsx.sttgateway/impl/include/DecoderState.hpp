// **********************************************************************
// * Copyright (C)2020, International Business Machines Corporation and *
// * others. All Rights Reserved.                                       *
// **********************************************************************

#ifndef COM_IBM_STREAMS_STTGATEWAY_DECODER_STATE_H_
#define COM_IBM_STREAMS_STTGATEWAY_DECODER_STATE_H_

#include "DecoderCommons.hpp"

namespace com { namespace ibm { namespace streams { namespace sttgateway {

class DecoderState : public virtual DecoderCommons {
private:
	const rapidjson::Value* stateValue;
	bool isListening_;

public:
	bool hasResult() const noexcept   { return stateValue != nullptr; }
	bool isListening() const noexcept { return isListening_; }

protected:
	DecoderState(const WatsonSTTConfig & config) :
		DecoderCommons(config),
		stateValue(nullptr), isListening_(false) {
	}

	void doWork() {
		stateValue = getOptionalMember<StringLabel>(jsonDoc, "state", "universe");
		if (stateValue) {
			std::string statevar = stateValue->GetString();
			isListening_ = statevar == "listening";
		} else {
			isListening_ = false;
		}
	}
};

}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_STATE_H_ */
