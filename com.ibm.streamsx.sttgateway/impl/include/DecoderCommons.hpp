// **********************************************************************
// * Copyright (C)2020, International Business Machines Corporation and *
// * others. All Rights Reserved.                                       *
// **********************************************************************

#ifndef COM_IBM_STREAMS_STTGATEWAY_DECODER_COMMONS_H_
#define COM_IBM_STREAMS_STTGATEWAY_DECODER_COMMONS_H_

#include <rapidjson/document.h>

#include <SPL/Runtime/Common/RuntimeDebug.h>
#include <SPL/Runtime/Type/SPLType.h>

#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>

#include "WatsonSTTConfig.hpp"

namespace com { namespace ibm { namespace streams { namespace sttgateway {

class DecoderException: public std::invalid_argument {
public:
	explicit DecoderException(const std::string& arg): invalid_argument(arg){};
};

// all arrays must not be const char, because tempate generation need external binding
char BooleanLabel[] = "Boolean";
char ObjectLabel[]  = "Object";
char IntegerLabel[] = "Integer";
char NumberLabel[]  = "Number";
char StringLabel[]  = "String";
char ArrayLabel[]   = "Array";

template<char* JSONTYPE> bool isType(const rapidjson::Value& member);
template<>               bool isType<BooleanLabel>(const rapidjson::Value& member) { return member.IsBool(); }
template<>               bool isType<ObjectLabel> (const rapidjson::Value& member) { return member.IsObject(); }
template<>               bool isType<IntegerLabel>(const rapidjson::Value& member) { return member.IsInt(); }
template<>               bool isType<NumberLabel> (const rapidjson::Value& member) { return member.IsNumber(); }
template<>               bool isType<StringLabel> (const rapidjson::Value& member) { return member.IsString(); }
template<>               bool isType<ArrayLabel>  (const rapidjson::Value& member) { return member.IsArray(); }

class DecoderCommons {
protected:
	// Input string pointer: points during work to input string
	const std::string * json;
	// Document after parsing
	rapidjson::Document jsonDoc;
	// Configuration values
	const WatsonSTTConfig & configuration;
	// static log aspect
	static const char* const WATSON_DECODER;

	// ctor
	DecoderCommons(const WatsonSTTConfig & config) :
		json(nullptr), jsonDoc(), configuration(config) {
	}
	// start decoding initialize json and jsonDoc
	void doWorkStart(std::string const & inp) {
		json = & inp;
		jsonDoc.Parse(json->c_str());
	}
	// stop working
	void doWorkEnd() noexcept {
		json = nullptr;
	}
	// get a required member
	template<char* JSONTYPE>
	const rapidjson::Value& getRequiredMember(const rapidjson::Value& value, const char* memberName, const char* parentName) const {
		if ( ! value.HasMember(memberName)) {
			std::stringstream ss("member "); ss << memberName << " is required in " << parentName << " json=" << *json;
			throw DecoderException(ss.str());
		}
		SPLAPPTRC(L_TRACE, memberName << " found", WATSON_DECODER);
		const rapidjson::Value& member = value[memberName];
		if ( ! isType<JSONTYPE>(member)) {
			std::stringstream ss; ss << memberName << " is not a " << JSONTYPE << " in " << parentName << " json=" << *json;
			throw DecoderException(ss.str());
		}
		SPLAPPTRC(L_TRACE, memberName << " is " << JSONTYPE, WATSON_DECODER);
		return member;
	}

	// get an option member; return nullptr if not there
	template<char* JSONTYPE>
	const rapidjson::Value* getOptionalMember(const rapidjson::Value& value, const char* memberName, const char* parentName) const {
		if ( ! value.HasMember(memberName))
			return nullptr;
		SPLAPPTRC(L_TRACE, memberName << " found", WATSON_DECODER);
		const rapidjson::Value& member = value[memberName];
		if ( ! isType<JSONTYPE>(member)) {
			std::stringstream ss; ss << memberName << " is not a " << JSONTYPE << " in " << parentName << " json=" << *json;
			throw DecoderException(ss.str());
		}
		SPLAPPTRC(L_TRACE, memberName << " is " << JSONTYPE, WATSON_DECODER);
		return &member;
	}
};

const char* const DecoderCommons::WATSON_DECODER = "WatsonDecoder";

}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_DECODER_COMMONS_H_ */
