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
	bool getResult(size_t index) const noexcept { return finals_[index]; }

protected:
	DecoderFinal(const WatsonSTTConfig & config) :
		DecoderCommons(config), finals_() {
	}

	void reset() {
		finals_.clear();
	}

	void doWork(const rapidjson::Value & result, const std::string & parentName) {
		const rapidjson::Value & final = getRequiredMember<BooleanLabel>(result, "final", parentName.c_str());
		finals_.push_back(final.GetBool());
	}
};

}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_FINAL_H_ */
