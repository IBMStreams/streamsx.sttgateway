// **********************************************************************
// * Copyright (C)2020, International Business Machines Corporation and *
// * others. All Rights Reserved.                                       *
// **********************************************************************

#ifndef COM_IBM_STREAMS_STTGATEWAY_DECODER_ERROR_H_
#define COM_IBM_STREAMS_STTGATEWAY_DECODER_ERROR_H_

#include "DecoderCommons.hpp"

namespace com { namespace ibm { namespace streams { namespace sttgateway {

class DecoderError : public virtual DecoderCommons {
private:
	const rapidjson::Value* value_;

public:
	DecoderError(const std::string & inp, bool completeResults_) : DecoderCommons(inp, completeResults_), value_(nullptr) { }

	bool hasResult() { return value_ != nullptr; }

	std::string getResult() { return std::string(value_->GetString()); }

protected:
	void doWork() {
		value_ = getOptionalMember<StringLabel>(jsonDoc, "error", "universe");
	}
};

}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_ERROR_H_ */
