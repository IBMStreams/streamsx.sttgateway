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
	const rapidjson::Value* errorValue;

public:
	bool hasResult() const noexcept { return errorValue != nullptr; }
	std::string getResult() const   { return std::string(errorValue->GetString()); }

protected:
	DecoderError(const WatsonSTTConfig & config) :
		DecoderCommons(config), errorValue(nullptr) {
	}

	void doWork() {
		errorValue = getOptionalMember<StringLabel>(jsonDoc, "error", "universe");
	}
};

}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_ERROR_H_ */
