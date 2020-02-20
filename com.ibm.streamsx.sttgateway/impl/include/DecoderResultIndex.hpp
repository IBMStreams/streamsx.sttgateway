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
	const rapidjson::Value* value_;

public:
	DecoderResultIndex(std::string const & inp) : DecoderCommons(inp), value_(nullptr) { }

	bool hasResult() { return value_ != nullptr; }

	SPL::int32 getResult() { return value_->GetInt(); }

protected:
	void doWork() {
		value_ = getOptionalMember<NumberLabel>(jsonDoc, "result_index", "universe");
	}
};

}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_RESULT_INDEX_H_ */
