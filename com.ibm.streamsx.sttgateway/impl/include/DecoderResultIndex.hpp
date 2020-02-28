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
	bool hasResult() const noexcept { return resultIndexValue != nullptr; }

	// calling this function is always secure
	// return -1 if no index was received in doWork
	SPL::int32 getResult() const {
		if (resultIndexValue)
			return resultIndexValue->GetInt();
		else
			return -1;
	}

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
