/*
 * WatsonSTTImpl.hpp
 *
 * Licensed Materials - Property of IBM
 * Copyright IBM Corp. 2019, 2020
 *
 *  Created on: Jan 14, 2020
 *      Author: joergboe
*/

#ifndef COM_IBM_STREAMS_STTGATEWAY_WATSONSTTIMPL_HPP_
#define COM_IBM_STREAMS_STTGATEWAY_WATSONSTTIMPL_HPP_

#include <string>
#include <atomic>
#include <memory>

// This operator heavily relies on the Websocket++ header only library.
// https://docs.websocketpp.org/index.html
// This C++11 library code does the asynchronous full duplex Websocket communication with
// the Watson STT service via a series of event handlers (a.k.a callback methods).
// Bulk of the logic in this operator class appears in those event handler methods below.
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

// A nice read in this URL about using property_tree for JSON parsing:
// http://zenol.fr/blog/boost-property-tree/en.html
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <boost/exception/to_string.hpp>

// SPL Operator related includes
#include <SPL/Runtime/Type/SPLType.h>
#include <SPL/Runtime/Function/SPLFunctions.h>
#include <SPL/Runtime/Common/Metric.h>
#include <SPL/Runtime/Utility/Mutex.h>

#include <WatsonSTTImplReceiver.hpp>
#include <SttGatewayResource.h>

namespace com { namespace ibm { namespace streams { namespace sttgateway {

// Websocket related type definitions.
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
// Pull out the type of messages sent by our config
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;

/*
 * Implementation class for operator Watson STT
 * Move almost of the c++ code of the operator into this class to take the advantage of c++ editor support
 *
 * Template argument : OP: Operator type (must inherit from SPL::Operator)
 *                     OT: Output Tuple type
 */
template<typename OP, typename OT>
class WatsonSTTImpl : public WatsonSTTImplReceiver<OP, OT> {
public:
	// shorthands
	typedef WatsonSTTConfig Conf;
	typedef WatsonSTTImplReceiver<OP, OT> Rec;
	//Constructors
	WatsonSTTImpl(OP & splOperator_, Conf config_);
	//WatsonSTTImpl(WatsonSTTImpl const &) = delete;

	//Destructor
	~WatsonSTTImpl();

protected:
	// Tuple processing for mutating data port 0
	template<typename IT0, typename DATA_TYPE, DATA_TYPE & (IT0::*GETTER)()>
	void process_0(IT0 & inputTuple);

	// Tuple processing for authentication port 1
	template<typename IT1, const SPL::rstring& (IT1::*GETTER)()const>
	void process_1(IT1 const & inputTuple);

private:
	// check connection state and connect if necessary and send the data
	// does not take the ownership of audioBytes
	// takes the ownership of nextOTuple
	void connectAndSendDataToSTT(std::unique_ptr<unsigned char> audioBytes, uint64_t audioSize, bool sendEndOfText, OT * nextOTuple);

private:
	// Websocket operations related member variables.
	// values set from receiver thread and read from sender side
	// All values are primitive atomic values, no locking

	//In - out related variables (set from sender thread
	int numberOfAudioBlobFragmentsReceivedInCurrentConversation;
	bool mediaEndReached;

	// If recentOTuple is not null, a conversation is ongoing
	std::atomic<OT *> recentOTuple;
	// When a new oTuple is to be set the previous otuple is put into the oTupleWastebasket
	// The wastebasked is emptied after transcription was finalizes and before a new conversation starts
	std::vector<OT *> oTupleWastebasket;

	std::string accessToken;

	// Port Mutex
	SPL::Mutex portMutex;

	SPL::int64 numberOfFullAudioConversationsReceived;

	// Custom metrics for this operator.
	SPL::Metric * const nFullAudioConversationsReceivedMetric;
	SPL::Metric * const nSTTResultModeMetric;

protected:
	// Helper functions
	void incrementNumberOfFullAudioConversationsReceived();
};

// Enum to define the results of the getSpeechSamples operation
enum class GetSpeechSamplesResult {
	sucess,
	sucessAndMediaEnd,
	errorAndMediaEnd
	//errorIntermediate
};

// The extraction function to extract the blob or the blob from filename into the blob list and size list
// Returns success; success and media end; of error and media end (which is a file read error)
// The function transfers the ownership of the audioBytes buffer to the caller if not null
// The primary template is never used so it is not defined
template<typename DATA_TYPE>
GetSpeechSamplesResult getSpeechSamples(
			DATA_TYPE & input,
			unsigned char * & audioBytes,
			uint64_t & audioSize,
			std::string & currentFileName);

/*template<typename OP, typename OT>
const SPL::float64 WatsonSTTImpl<OP, OT>::waitTimeBeforeSTTServiceConnectionRetryLong = 60.0;*/

template<typename OP, typename OT>
WatsonSTTImpl<OP, OT>::WatsonSTTImpl(OP & splOperator_,Conf config_)
:
		Rec(splOperator_, config_),

		numberOfAudioBlobFragmentsReceivedInCurrentConversation(0),
		mediaEndReached(true),
		recentOTuple{},
		oTupleWastebasket(),

		accessToken{},

		portMutex{},

		numberOfFullAudioConversationsReceived{0},

		// Custom metrics for this operator are already defined in the operator model XML file.
		// Hence, there is no need to explicitly create them here.
		// Simply get the custom metrics already defined for this operator.
		// The update of metrics nFullAudioConversationsReceived and nFullAudioConversationsTranscribed depends on parameter sttLiveMetricsUpdateNeeded
		nFullAudioConversationsReceivedMetric{ & Rec::splOperator.getContext().getMetrics().getCustomMetricByName("nFullAudioConversationsReceived")},
		nSTTResultModeMetric{ & Rec::splOperator.getContext().getMetrics().getCustomMetricByName("nSTTResultMode")}
{
	if (Conf::customizationId == "") {
		// No customization id configured. Hence, set the customization weight to
		// 9.9 which will be ignored by the C++ logic later in the on_open method.
		Conf::customizationWeight = 9.9;
	}

	if (Conf::sttResultMode < 1 || Conf::sttResultMode > 3) {
		throw std::runtime_error(STTGW_INVALID_PARAM_VALUE_1("WatsonSTT", Conf::sttResultMode));
	}

	if (Conf::maxUtteranceAlternatives <= 0) {
		throw std::runtime_error(STTGW_INVALID_PARAM_VALUE_2("WatsonSTT", Conf::maxUtteranceAlternatives));
	}

	if (Conf::wordAlternativesThreshold < 0.0 || Conf::wordAlternativesThreshold >= 1.0) {
		throw std::runtime_error(STTGW_INVALID_PARAM_VALUE_3("WatsonSTT", Conf::wordAlternativesThreshold, "wordAlternativesThreshold"));
	}

	if (Conf::keywordsSpottingThreshold < 0.0 || Conf::keywordsSpottingThreshold >= 1.0) {
		throw std::runtime_error(STTGW_INVALID_PARAM_VALUE_3("WatsonSTT", Conf::keywordsSpottingThreshold, "keywordsSpottingThreshold"));
	}

	// If the keywords to be spotted list is empty, then disable keywords_spotting.
	if (Conf::keywordsToBeSpotted.size() == 0) {
		Conf::keywordsSpottingThreshold = 0.0;
	}

	if (Conf::cpuYieldTimeInAudioSenderThread < 0.0) {
		throw std::runtime_error(STTGW_INVALID_PARAM_VALUE_4("WatsonSTT", Conf::cpuYieldTimeInAudioSenderThread, "cpuYieldTimeInAudioSenderThread", "0.0"));
	}

	if (Conf::waitTimeBeforeSTTServiceConnectionRetry < 1.0) {
		throw std::runtime_error(STTGW_INVALID_PARAM_VALUE_4("WatsonSTT", Conf::waitTimeBeforeSTTServiceConnectionRetry,  "waitTimeBeforeSTTServiceConnectionRetry", "1.0"));
	}

	if (Conf::connectionAttemptsThreshold < 1) {
		throw std::runtime_error(STTGW_INVALID_PARAM_VALUE_4("WatsonSTT", Conf::connectionAttemptsThreshold, "connectionAttemptsThreshold", "1"));
	}

	// We are not going to support the following utterance based
	// features when the STT result mode is 3 (full transcript).
	// Many of these features return the results in individual arrays for a
	// given utterance. When we assemble the full transcript using
	// multiple utterances, it will be too much data to deal with and
	// it will prove to be not very useful in processing multiple
	// arrays to make sense out of what happened in the context of
	// a full transcript. Hence, we are disabling these features for
	// the STT result mode 3 (full transcript).
	if (Conf::sttResultMode == 3) {
		// No n-best utterance alternative hypotheses.
		Conf::maxUtteranceAlternatives = 1;
		// No Confusion Networks.
		Conf::wordAlternativesThreshold = 0.0;
		// No individual word confidences.
		Conf::wordConfidenceNeeded = false;
		// No individual timestamps.
		Conf::wordTimestampNeeded = false;
		//No speaker identification for the individual words in an utterance.
		Conf::identifySpeakers = false;
		//No keyword spotting inside an utterance.
		Conf::keywordsSpottingThreshold = 0.0;
	}

	// Update the operator metric.
	nSTTResultModeMetric->setValueNoLock(Conf::sttResultMode);

	//print the configuration
	std::cout << "WatsonSTT configuration:"
	<< "\nOperatorName                            = " << Rec::splOperator.getContext().getName()
	<< "\ncpuYieldTimeInAudioSenderThread         = " << Conf::cpuYieldTimeInAudioSenderThread
	<< "\nwaitTimeBeforeSTTServiceConnectionRetry = " << Conf::waitTimeBeforeSTTServiceConnectionRetry
	<< "\nwaitTimeBeforeSTTServiceConnectionRetryLong=" << Conf::waitTimeBeforeSTTServiceConnectionRetryLong
	<< "\nwaitTimeWhenIdle                        = " << Conf::waitTimeWhenIdle
	<< "\nwaitTimeBeforePreviousTranscriptionFinalizes=" << Conf::waitTimeBeforePreviousTranscriptionFinalizes
	<< "\nconnectionAttemptsThreshold             = " << Conf::connectionAttemptsThreshold
	<< "\nsttLiveMetricsUpdateNeeded              = " << Conf::sttLiveMetricsUpdateNeeded
	<< "\nuri                                     = " << Conf::uri
	<< "\nbaseLanguageModel                       = " << Conf::baseLanguageModel
	<< "\ncontentType                             = " << Conf::contentType
	<< "\nsttResultMode                           = " << Conf::sttResultMode
	<< "\nsttRequestLogging                       = " << Conf::sttRequestLogging
	<< "\nbaseModelVersion                        = " << Conf::baseModelVersion
	<< "\ncustomizationId                         = " << Conf::customizationId
	<< "\ncustomizationWeight                     = " << Conf::customizationWeight
	<< "\nacousticCustomizationId                 = " << Conf::acousticCustomizationId
	<< "\nfilterProfanity                         = " << Conf::filterProfanity
	<< "\nsttJsonResponseDebugging                = " << Conf::sttJsonResponseDebugging
	<< "\nmaxUtteranceAlternatives                = " << Conf::maxUtteranceAlternatives
	<< "\nwordAlternativesThreshold               = " << Conf::wordAlternativesThreshold
	<< "\nwordConfidenceNeeded                    = " << Conf::wordConfidenceNeeded
	<< "\nwordTimestampNeeded                     = " << Conf::wordTimestampNeeded
	<< "\nidentifySpeakers                        = " << Conf::identifySpeakers
	<< "\nsmartFormattingNeeded                   = " << Conf::smartFormattingNeeded
	<< "\nkeywordsSpottingThreshold               = " << Conf::keywordsSpottingThreshold
	<< "\nkeywordsToBeSpotted                     = " << Conf::keywordsToBeSpotted
	<< "\nconnectionState.wsState.is_lock_free()  = " << Rec::wsState.is_lock_free()
	<< "\nrecentOTuple.is_lock_free()             = " << Rec::recentOTuple.is_lock_free()
	<< "\nnumberOfFullAudioConversationsTranscribed.is_lock_free()= " << Rec::numberOfFullAudioConversationsTranscribed.is_lock_free()
	<< "\n----------------------------------------------------------------" << std::endl;
}

template<typename OP, typename OT>
WatsonSTTImpl<OP, OT>::~WatsonSTTImpl() {
	for (auto x : oTupleWastebasket)
		delete x;
}

template<typename OP, typename OT>
template<typename IT1, const SPL::rstring& (IT1::*GETTER)()const>
void WatsonSTTImpl<OP, OT>::process_1(IT1 const & inputTuple) {

	// The access token is written here and is read in method ws_init (in context of the operator receiver thread)
	// But the lock with portMutex is sufficient, because ws_init reads access token
	// when wsConnectionEstablished is false and makeNewWebsocketConnection is true
	// and during this time the portMutex is acquired from process_0
	SPL::AutoMutex autoMutex(portMutex);

	// Save the access token for subsequent use within this operator.
	const SPL::rstring& at = (inputTuple.*GETTER)();
	accessToken = at;
	SPLAPPTRC(L_INFO, Conf::traceIntro << "-->Received new/refreshed access token.", "process");

	// This must be the audio data arriving here via port 0 i.e. first input port.
	// If we have a non-empty IAM access token, process the audio data.
	// Otherwise, skip it.
	if (accessToken.empty()) {
		SPLAPPLOG(L_ERROR, STTGW_EMPTY_IAM_TOKEN("WatsonSTT"), "process");
	}
}

//Definition of the template function getSpeechSamples if data type is SPL::blob
template<>
GetSpeechSamplesResult getSpeechSamples<SPL::blob>(
		SPL::blob & input,
		unsigned char * & audioBytes,
		uint64_t & audioSize,
		std::string & currentFileName)
{
	uint64_t sizeOfBlob = SPL::Functions::Collections::blobSize(input);

	audioSize = sizeOfBlob;
	if (sizeOfBlob == 0) {
		audioBytes = nullptr;
	} else {
		audioBytes = input.releaseData(sizeOfBlob);
	}
	// Data bluffer assigned or null
	return GetSpeechSamplesResult::sucess;
}

//Definition of the template function getSpeechSamples if data type is rstring -> read file
template<>
GetSpeechSamplesResult getSpeechSamples<SPL::rstring>(
		SPL::rstring & input,
		unsigned char * & audioBytes,
		uint64_t & audioSize,
		std::string & currentFileName)
{
	SPLAPPTRC(L_DEBUG, "-->Sending file " << input, "getSpeechSamples");
	currentFileName = input;

	// Check for the file existence before attempting to read the audio data.
	// In C++, this is a very fast way to check file existence.
	// https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
	struct stat fileStat;
	int32_t fileStatReturnCode = stat(input.c_str(), &fileStat);
	// int32_t fileSize = fileStat.st_size;

	if (fileStatReturnCode != 0) {

		// File doesn't exist.
		// Log this information and remove this audio data from the vector.
		SPLAPPTRC(L_ERROR, "-->Audio file not found. Skipping STT task for this file: " << input, "getSpeechSamples");
		//No output buffer assigned
		return GetSpeechSamplesResult::sucessAndMediaEnd;

	} else {
		// Audio file exists. Read binary file into buffer
		std::ifstream inputStream(input.c_str(), std::ios::binary);
		//std::vector<char> buffer((std::istreambuf_iterator<char>(inputStream)), (std::istreambuf_iterator<char>()));
		uint64_t fsize = fileStat.st_size;
		SPLAPPTRC(L_DEBUG, "-->Sending file size" << fsize, "getSpeechSamples");

		if (fsize == 0) {
			audioBytes = nullptr;
			audioSize = 0;
		} else {
			unsigned char * const data = new unsigned char[fsize];
			unsigned char * datax = data;
			std::istreambuf_iterator<char> it(inputStream);
			std::istreambuf_iterator<char> it_end;
			while (not it.equal(it_end)) {
				*datax = *it;
				++datax;
				++it;
			}
			//TODO: error handling during read
			// Data buffer is assigned
			audioBytes = data;
			audioSize = fsize;
		}

		return GetSpeechSamplesResult::sucessAndMediaEnd;
	} // End of if (stat(audioFileName.c_str(), &buffer) != 0)
}

template<typename OP, typename OT>
template<typename IT0, typename DATA_TYPE, DATA_TYPE & (IT0::*GETTER)()>
void WatsonSTTImpl<OP, OT>::process_0(IT0 & inputTuple) {

	// serialize this method and protect from issues when multiple threads send
	// to this port
	SPL::AutoMutex autoMutex(portMutex);

	// If the media end of the previous translation was reached, we wait until the translation is finalized.
	// A finalized transcription is signed through Rec::transcriptionFinalized
	// Transcription finalized must be set in all error cases too!
	// The oTuple is not longer needed when the transcription is finalized
	// An error cleans oTuple too
	// The is no need to acquire the state mutex here because we make no changes, we just wait for the transition
	// to null. There are no other write operations to recentOTuple due to port mutex
	if (mediaEndReached) {
		WsState myWsState = Rec::wsState.load();
		bool receiverHasStopped = (myWsState == WsState::closed) || (myWsState == WsState::failed) || (myWsState == WsState::crashed);
		while (not Rec::transcriptionFinalized && not receiverHasStopped) {
			SPLAPPTRC(L_TRACE, Conf::traceIntro <<
					"-->Sender 1 We have something to send but the previous transcription is not finalized, "
					" wsState=" << wsStateToString(myWsState) << " block for " <<
					Conf::waitTimeBeforePreviousTranscriptionFinalizes << " second",
					"process_0");
			SPL::Functions::Utility::block(Conf::waitTimeBeforePreviousTranscriptionFinalizes);
			receiverHasStopped = (myWsState == WsState::closed) || (myWsState == WsState::failed) || (myWsState == WsState::crashed);
			if (Rec::splOperator.getPE().getShutdownRequested())
				return;
		}
		// Here is the receicvr either dead or a transcription has finalized
		// A new transcription ha not yet been started, hence no race condition can occure
		Rec::transcriptionFinalized = false;
		mediaEndReached = false;
		// this is the first blob in a conversation
		numberOfAudioBlobFragmentsReceivedInCurrentConversation = 0;

		// no current conversation is ongoing -> clear recentOTuple
		Rec::recentOTuple.store(nullptr);

		// recycle all oTuples from wastebasket
		for (OT* x : oTupleWastebasket)
			delete x;
		oTupleWastebasket.clear();
	}

	// This must be the audio data arriving here via port 0 i.e. first input port.
	// If we have a non-empty IAM access token, process the audio data.
	// Otherwise, skip it.
	if (accessToken.empty()) {
		SPLAPPTRC(L_ERROR, Conf::traceIntro << "-->Ignoring the received audio data at this time due to an empty IAM access "
				"token. User must first provide the IAM access token before sending any audio data to this operator.",
				"process_0");
		return;
	}

	//get input
	DATA_TYPE & mySpeechAttribute = (inputTuple.*GETTER)();

	std::string myCurrentFileName;
	unsigned char * audioBytes_ = nullptr;
	uint64_t myAudioSize = 0ul;
	GetSpeechSamplesResult result = getSpeechSamples(mySpeechAttribute, audioBytes_, myAudioSize, myCurrentFileName);
	std::unique_ptr<unsigned char> myAudioBytes(audioBytes_);

	bool myAudioSamplesFound = false;
	bool myMediaEndFound = false;

	if (result == GetSpeechSamplesResult::errorAndMediaEnd) {

		// File read error: do not push samples into audio vector
		// Send error tuple directly
		incrementNumberOfFullAudioConversationsReceived();
		numberOfAudioBlobFragmentsReceivedInCurrentConversation = 0;

		std::string errorMsg = Conf::traceIntro + "-->Audio file not found. Skipping STT task for this file: " + myCurrentFileName;
		SPLAPPTRC(L_ERROR, errorMsg, "process_0");

		// Create an output tuple, auto assign from current input tuple
		OT * myOTuple = Rec::splOperator.createOutTupleAndAutoAssign(inputTuple);
		// Assign error message and send the tuple
		Rec::splOperator.setErrorAttribute(myOTuple, errorMsg);
		Rec::splOperator.submit(*myOTuple, 0);
		Rec::incrementNumberOfFullAudioConversationsFailed();

		delete myOTuple;
		return;

	} else if (result == GetSpeechSamplesResult::sucessAndMediaEnd) {

		incrementNumberOfFullAudioConversationsReceived();

		if (myAudioSize == 0) {
			numberOfAudioBlobFragmentsReceivedInCurrentConversation = 0;
			// File read success: but file is empty: Send error tuple directly
			std::string errorMsg = Conf::traceIntro + "-->Audio file is empty. Skipping STT task for this file: " + myCurrentFileName;
			SPLAPPTRC(L_WARN, errorMsg, "process_0");

			// Create an output tuple, auto assign from current input tuple
			OT * myOTuple = Rec::splOperator.createOutTupleAndAutoAssign(inputTuple);
			// Assign error message and send the tuple
			Rec::splOperator.setErrorAttribute(myOTuple, errorMsg);
			Rec::splOperator.submit(*myOTuple, 0);
			Rec::incrementNumberOfFullAudioConversationsFailed();

			delete myOTuple;
			return;

		} else {
			// File read success
			numberOfAudioBlobFragmentsReceivedInCurrentConversation = 1;
			myAudioSamplesFound = true;
			myMediaEndFound = true;
		}
	} else { // GetSpeechSamplesResult::sucess

		if (numberOfAudioBlobFragmentsReceivedInCurrentConversation == 0) {

			// A new transcription must be started
			incrementNumberOfFullAudioConversationsReceived();

			if (myAudioSize == 0) {

				std::string errorMsg = "-->Received audio data is an empty blob in an empty conversation";
				SPLAPPTRC(L_WARN, Conf::traceIntro << errorMsg, "process_0");
				// Create an output tuple, auto assign from current input tuple
				OT * myOTuple = Rec::splOperator.createOutTupleAndAutoAssign(inputTuple);
				// Assign error message and send the tuple
				Rec::splOperator.setErrorAttribute(myOTuple, errorMsg);
				Rec::splOperator.submit(*myOTuple, 0);
				Rec::incrementNumberOfFullAudioConversationsFailed();

				delete myOTuple;
				return;

			} else {

				++numberOfAudioBlobFragmentsReceivedInCurrentConversation;
				myAudioSamplesFound = true;
				myMediaEndFound = false;
			}

		} else {

			// A subsequent blob is received
			if (myAudioSize == 0) {
				// End of current conversation
				numberOfAudioBlobFragmentsReceivedInCurrentConversation = 0;
				myAudioSamplesFound = false;
				myMediaEndFound = true;
			} else {
				++numberOfAudioBlobFragmentsReceivedInCurrentConversation;
				myAudioSamplesFound = true;
				myMediaEndFound = false;
			}
		}
	}

	// store the media end condition
	mediaEndReached = myMediaEndFound;
	OT * nextOTuple = Rec::splOperator.createOutTupleAndAutoAssign(inputTuple);
	connectAndSendDataToSTT(std::unique_ptr<unsigned char>(myAudioBytes.release()), myAudioSize, myMediaEndFound, nextOTuple);

	return;

	// Let us do all the auto output tuple attribute assignments and store it in a
	// list to be used in the on_message event handler method below at the time of
	// either full or partial transcription gets completed for a given audio data.
	// We will create a dynamic oTuple object via the new C++ construct so that
	// the object pointer can be stored in the oTupleList below.
	// For every new audio (either a new audio filename or the very first
	// audio blob fragment sent here, we will create a new oTuple object.
	//const bool newAudioSegment = numberOfAudioBlobFragmentsReceivedInCurrentConversation == 1;
	//if (newAudioSegment) {
	//	OT * oTuple = splOperator.createOutTupleAndAutoAssign(inputTuple);
		// Push this partially filled oTuple object's pointer to the
		// vector so that we can get it back when the transcription result arrives.
	//	oTupleList.push_back(oTuple);
} // End: WatsonSTTImpl<OP, OT>::process_0

// check connection state and connect if necessary and send the data
template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::connectAndSendDataToSTT(std::unique_ptr<unsigned char> audioBytes, uint64_t audioSize, bool sendEndOfText, OT * nextOTuple) {

	SPLAPPTRC(L_DEBUG, Conf::traceIntro << "connectAndSendDataToSTT(audioBytes=" <<
			audioBytes.get() << ", audioSize=" << audioSize << ", sendEndOfText=" << sendEndOfText << ")",
			"connectAndSendDataToSTT");

	// We make a new connection or use a existing connection if data are to send
	websocketpp::lib::error_code ec1{};
	if (audioSize > 0) {
		WsState myWsState = Rec::wsState.load();
		while (myWsState != WsState::listening) {

			if (myWsState == WsState::start || myWsState == WsState::connecting || myWsState == WsState::open) {
				// Connection was already triggered
				SPLAPPTRC(L_TRACE, Conf::traceIntro <<
						"A connection is about to enter listening state but not listening wsState=" << wsStateToString(myWsState) <<
						" wait 0.5",
						"connectAndSendDataToSTT");
				SPL::Functions::Utility::block(0.500);

			} else if (myWsState == WsState::error){
				SPLAPPTRC(L_TRACE, Conf::traceIntro <<
						"A connection has received and error  wsState=" << wsStateToString(myWsState) <<
						" wait 0.5",
						"connectAndSendDataToSTT");
				SPL::Functions::Utility::block(0.500);
			} else {
				// Make a new connection attempt
				// The receiver thread must have reached a final state
				// Here the state must not start, not connecting, not open, not listening, not error
				++Rec::numberOfWebsocketConnectionAttempts;
				Rec::nWebsocketConnectionAttemptsMetric->setValueNoLock(Rec::numberOfWebsocketConnectionAttempts);
				if (Rec::numberOfWebsocketConnectionAttempts == 1) {
					SPLAPPTRC(L_INFO, "Make a connection attempt number 1 from wsState=" << wsStateToString(myWsState), "connectAndSendDataToSTT");
				} else {
					// Delay repeated connection requests
					SPL::float64 waitTime = Conf::waitTimeBeforeSTTServiceConnectionRetry;
					if (Rec::numberOfWebsocketConnectionAttempts >= Conf::connectionAttemptsThreshold)
						waitTime = Conf::waitTimeBeforeSTTServiceConnectionRetryLong;
					SPLAPPTRC(L_WARN, Conf::traceIntro <<
							"Delay repeated connection requests number " << Rec::numberOfWebsocketConnectionAttempts <<
							" wsState=" << static_cast<int>(myWsState) << " wait " << waitTime, "connectAndSendDataToSTT");
					SPL::Functions::Utility::block(waitTime);
					SPLAPPTRC(L_INFO, "Make a connection attempt number " <<  Rec::numberOfWebsocketConnectionAttempts <<
							" from wsState=" << wsStateToString(myWsState), "connectAndSendDataToSTT");
				}

				// The receiver thread is in an inactive state: Now store the access token to receiver thread variable
				Rec::accessToken = accessToken;
				// make the connection attempt
				Rec::wsState.store(WsState::start);
			}
			if (Rec::splOperator.getPE().getShutdownRequested()) {
				if (nextOTuple)
					delete nextOTuple;
				return;
			}
			myWsState = Rec::wsState.load();
		} // END: while (not connectionState.Rec::wsConnectionEstablished)

		// myWsState == WsState::listening
		if (myWsState == WsState::listening) {
			// put new otuple to recentOTuple and to wastebasket
			// if nextOTuple is null, the old recentOTuple is kept
			if (nextOTuple) {
				oTupleWastebasket.push_back(nextOTuple);
				recentOTuple.store(nextOTuple);
				nextOTuple = nullptr;
			}

			if (Conf::cpuYieldTimeInAudioSenderThread > 0.0) {
				// Audio data available for processing. Yield the CPU briefly and get to work soon.
				// Even a tiny value of 1 millisecond (0.001 second) will yield the
				// CPU and will not show 0% idle in the Linux top command.
				SPLAPPTRC(L_TRACE, Conf::traceIntro << "-->Going to send data , "
						"block for cpuYieldTimeInAudioSenderThread=" << Conf::cpuYieldTimeInAudioSenderThread,
						"connectAndSendDataToSTT");
				SPL::Functions::Utility::block(Conf::cpuYieldTimeInAudioSenderThread);
			}

			// Speech data is sent into this operator via a blob buffer rather than via a file.
			// Send the blob data (either in full or in a partial fragment) to the STT service.
			// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-websockets#WSaudio
			// c->get_alog().write(websocketpp::log::alevel::app, "Sent binary Message: " + boost::to_string(buffer.size()));
			websocketpp::lib::error_code ec;
			Rec::wsClient->send(Rec::wsHandle, audioBytes.get(), audioSize, websocketpp::frame::opcode::binary, ec);
			Rec::statusOfAudioDataTransmissionToSTT = AUDIO_BLOB_FRAGMENTS_BEING_SENT_TO_STT;
			if (ec) {
				SPLAPPTRC(L_ERROR, "Error when send sendEndOfText ec=" << ec, "connectAndSendDataToSTT");
			}
		}
	} // END: if (audioSize > 0)

	// We use a existing connection if end of thext has to be send
	// end of text is ignored if no connection is available
	if (sendEndOfText) {
		WsState myWsState = Rec::wsState.load();
		if (myWsState == WsState::listening) {
			// put new otuple to recentOTuple and to wastebasket
			// if nextOTuple is null, the old recentOTuple is kept
			if (nextOTuple) {
				oTupleWastebasket.push_back(nextOTuple);
				recentOTuple.store(nextOTuple);
				nextOTuple = nullptr;
			}

			SPLAPPTRC(L_DEBUG, "Send \"action\" : \"stop\"", "connectAndSendDataToSTT");
			// We reached the end of the blob data as sent/streamed from the SPL application.
			// Signal end of the audio data.
			// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-websockets#WSstop
			websocketpp::lib::error_code ec{};
			Rec::wsClient->send(Rec::wsHandle, "{\"action\" : \"stop\"}" , websocketpp::frame::opcode::text, ec);
			// In a blob based audio data, the entire blob has been sent to the STT service at this time.
			// So set this flag to indicate that.
			Rec::statusOfAudioDataTransmissionToSTT = FULL_AUDIO_DATA_SENT_TO_STT;
			if (ec) {
				SPLAPPTRC(L_ERROR, "Error when send sendEndOfText ec=" << ec, "connectAndSendDataToSTT");
			}
		} else {
			SPLAPPTRC(L_ERROR, "Connection state myWsState != WsState::listening but sendEndOfText requested. Ignoring", "connectAndSendDataToSTT");
		}
	}

	if (nextOTuple)
		delete nextOTuple;
	return;
} // End: WatsonSTTImpl<OP, OT>::connectAndSendDataToSTT


template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::incrementNumberOfFullAudioConversationsReceived() {
	++numberOfFullAudioConversationsReceived;
	// Update the operator metric only if the user asked for a live update.
	if (Conf::sttLiveMetricsUpdateNeeded == true) {
		nFullAudioConversationsReceivedMetric->setValueNoLock(numberOfFullAudioConversationsReceived);
	}
}

}}}}

#endif /* COM_IBM_STREAMS_STTGATEWAY_WATSONSTTIMPL_HPP_ */
