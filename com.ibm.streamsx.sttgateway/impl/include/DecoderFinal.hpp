// **********************************************************************
// * Copyright (C)2020, International Business Machines Corporation and *
// * others. All Rights Reserved.                                       *
// **********************************************************************

#ifndef COM_IBM_STREAMS_STTGATEWAY_DECODER_FINAL_H_
#define COM_IBM_STREAMS_STTGATEWAY_DECODER_FINAL_H_

#include "DecoderCommons.hpp"
#include <vector>

namespace com { namespace ibm { namespace streams { namespace sttgateway {

class DecoderFinal : public virtual DecoderCommons {
private:
	std::vector<bool> finals_;

public:
	bool getResult(size_t index) { return finals_[index]; }

protected:
	DecoderFinal(std::string const & inp, bool completeResults_) : DecoderCommons(inp, completeResults_), finals_() { }

	void doWork(const rapidjson::Value& result, const char* parentName, rapidjson::SizeType index) {
		/*std::stringstream ppname;
		ppname << parentName << "[" << index << "]";*/
		/*if (not result.IsObject()) {
			std::stringstream ss; ss << "result is not an Object. " << ppname << " in " << " json=" << json;
			throw DecoderException(ss.str());
		}*/
		finals_.push_back(getRequiredMember<BooleanLabel>(result, "final", parentName));
	}
};

}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_FINAL_H_ */
