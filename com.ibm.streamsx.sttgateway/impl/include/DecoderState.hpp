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
	const rapidjson::Value* value_;
	bool isListening_;

public:
	DecoderState(std::string const & inp, bool completeResults_) : DecoderCommons(inp, completeResults_), value_(nullptr), isListening_(false) { }

	bool hasResult() { return value_ != nullptr; }

	bool isLitsening() { return isListening_; }

protected:
	void doWork() {
		value_ = getOptionalMember<StringLabel>(jsonDoc, "state", "universe");
		std::string statevar = value_->GetString();
		isListening_ = statevar == "listening";
	}
};

}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_STATE_H_ */
