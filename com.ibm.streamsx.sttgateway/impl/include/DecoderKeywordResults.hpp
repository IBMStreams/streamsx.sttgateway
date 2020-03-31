// **********************************************************************
// * Copyright (C)2020, International Business Machines Corporation and *
// * others. All Rights Reserved.                                       *
// **********************************************************************

#ifndef COM_IBM_STREAMS_STTGATEWAY_DECODER_KEYWORD_RESULT_H_
#define COM_IBM_STREAMS_STTGATEWAY_DECODER_KEYWORD_RESULT_H_

#include "DecoderCommons.hpp"

#include <vector>
#include <unordered_map>
#include <utility>
#include <string>

namespace com { namespace ibm { namespace streams { namespace sttgateway {

class DecoderKeywordsResult : public virtual DecoderCommons {
public:
	// do not use SPL list and map because the KeywordEmergence has no serialization implementation
	struct KeywordEmergenceStruct { double start_time; double end_time; double confidence; };
	typedef std::unordered_map<std::string, std::vector<KeywordEmergenceStruct> > ResultMapType;
private:
	ResultMapType kwmaplist;

public:
	bool                  hasResult() const noexcept                  { return kwmaplist.size() > 0; }
	const ResultMapType & getKeywordsSpottingResults() const noexcept { return kwmaplist; }

protected:
	DecoderKeywordsResult(const WatsonSTTConfig & config) :
		DecoderCommons(config), kwmaplist() {
	}

	void reset() {
		kwmaplist.clear();
	}

	void doWork(const rapidjson::Value& result, const std::string & parentName, rapidjson::SizeType resultIndex);
};


void DecoderKeywordsResult::doWork(const rapidjson::Value& result, const std::string & parentName, rapidjson::SizeType resultIndex) {
	const rapidjson::Value * keywords_result_ = getOptionalMember<ObjectLabel>(result, "keywords_result", parentName.c_str());
	if (not keywords_result_) {
		return;
	}

	std::string parentNameKwres = parentName + "#keywords_result";
	for (auto kw : configuration.keywordsToBeSpotted) {
		const rapidjson::Value * keyword_ = getOptionalMember<ArrayLabel>(*keywords_result_, kw.c_str(), parentNameKwres.c_str());
		if (keyword_) {
			std::stringstream ppname;
			ppname << parentNameKwres << "[" << kw << "]";
			if (not keyword_->IsArray()) {
				throw DecoderException("Keyword " + kw.string() + " is not an array in " + ppname.str());
			}

			std::vector<KeywordEmergenceStruct> innerList;
			rapidjson::SizeType sz = keyword_->Size();
			for (rapidjson::SizeType i = 0; i < sz; i++) {
				std::stringstream pppname;
				pppname << parentName << "[" << i << "]";
				const rapidjson::Value & match = (*keyword_)[i];
				const rapidjson::Value & normalized_text = getRequiredMember<StringLabel>(match, "normalized_text", pppname.str().c_str());
				const rapidjson::Value & start_time      = getRequiredMember<NumberLabel>(match, "start_time", pppname.str().c_str());
				const rapidjson::Value & end_time        = getRequiredMember<NumberLabel>(match, "end_time", pppname.str().c_str());
				const rapidjson::Value & confidence      = getRequiredMember<NumberLabel>(match, "confidence", pppname.str().c_str());
				SPL::map<SPL::rstring, SPL::float64> innerMap;
				const double st = start_time.GetDouble();
				const double et = end_time.GetDouble();
				const double cf = confidence.GetDouble();
				innerList.push_back(KeywordEmergenceStruct{st, et, cf});
			}
			kwmaplist.insert(ResultMapType::value_type(kw, innerList));
		}
	}
}

}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_KEYWORD_RESULT_H_ */
