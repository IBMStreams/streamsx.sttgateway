// **********************************************************************
// * Copyright (C)2020, International Business Machines Corporation and *
// * others. All Rights Reserved.                                       *
// **********************************************************************

#ifndef COM_IBM_STREAMS_STTGATEWAY_DECODER_RESULT_INDEX_H_
#define COM_IBM_STREAMS_STTGATEWAY_DECODER_RESULT_INDEX_H_

#include "DecoderCommons.hpp"

namespace com { namespace ibm { namespace streams { namespace sttgateway {

class DecoderResultIndex : public virtual DecoderCommons {
private:
	const rapidjson::Value* resultIndexValue;

public:
	bool hasResult() { return resultIndexValue != nullptr; }

	SPL::int32 getResult() { return resultIndexValue->GetInt(); }

protected:
	DecoderResultIndex(const WatsonSTTConfig & config) :
		DecoderCommons(config), resultIndexValue(nullptr) {
	}

	void doWork() {
		resultIndexValue = getOptionalMember<NumberLabel>(jsonDoc, "result_index", "universe");
	}
};

}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_RESULT_INDEX_H_ */
