/*
 * WatsonSTTImpl.hpp
 *
 * Licensed Materials - Property of IBM
 * Copyright IBM Corp. 2019, 2021
 *
 *  Created on: Jan 14, 2020
 *  Modified on: Sep 12, 2021
 *  Author(s): Senthil, joergboe
*/

#ifndef COM_IBM_STREAMS_STTGATEWAY_WATSONSTTIMPL_HPP_
#define COM_IBM_STREAMS_STTGATEWAY_WATSONSTTIMPL_HPP_

#include <string>
#include <atomic>
#include <cmath>
#include <fstream>

// This operator heavily relies on the Websocket++ header only library.
// https://docs.websocketpp.org/index.html
// This C++11 library code does the asynchronous full duplex Websocket communication with
// the Watson STT service via a series of event handlers (a.k.a callback methods).
// The websocketpp gives c++11 support also for older compilers
// With c++11 capable compiler the stdlib is used
#include <websocketpp/common/memory.hpp>

// Bulk of the logic in this operator class appears in those event handler methods below.
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

// A nice read in this URL about using property_tree for JSON parsing:
// http://zenol.fr/blog/boost-property-tree/en.html
// we do not use boost json parser any longer
//#include <boost/property_tree/ptree.hpp>
//#include <boost/property_tree/json_parser.hpp>

#include <boost/exception/to_string.hpp>

// SPL Operator related includes
#include <SPL/Runtime/Type/SPLType.h>
#include <SPL/Runtime/Function/SPLFunctions.h>
#include <SPL/Runtime/Common/Metric.h>
#include <SPL/Runtime/Utility/Mutex.h>
#include <SPL/Runtime/Operator/Port/Punctuation.h>

#include <SttGatewayResource.h>

#include "WatsonSTTImplReceiver.hpp"

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
	// Notify port readiness
	//void allPortsReady();

	// Processing for websocket receiver and ping threads
	//void process(uint32_t idx);

	// Tuple processing for non mutating data port 0
	template<typename IT0, typename DATA_TYPE, DATA_TYPE const & (IT0::*GETTER)() const>
	void process_0(IT0 const & inputTuple);

	// Tuple processing for non mutating authentication port 1
	template<typename IT1, SPL::rstring const & (IT1::*GETTER)()const>
	void process_1(IT1 const & inputTuple);

	// Punctuation processing for data port 0 Window Markers
	void processPunct_0(SPL::Punctuation const & punct);

private:
	// check connection state and connect if necessary
	// acquires the accessTokenMutex
	// this function may delay for some time and the aqccessToken may change during this time
	// this assures that the connection can succeed after an access token becomes invalid and the new token is received
	// via port 1
	void connect();

	// send the the audio data to stt if any
	// does not take the ownership of audioBytes
	void sendDataToSTT(unsigned char const * audioBytes, uint64_t audioSize);

	// send the action stop if connection is in listening state
	void sendActionStop();

	// The ping thread send a ping periodically with period
	// senderPingPeriod if the wsState is listening
	void ping_init();

private:
	// Access Token mutex this should be a different to portMutex
	// Concurrent access to the access token from port 0 and port 1 is managed by the accessTokenMutex
	// The receiver thread gets an own copy of the access token just before the connection attempt is initiated
	// in function connect.
	SPL::Mutex accessTokenMutex;
	std::string accessToken;

	// Port0 Mutex to serialize the operations of port 0
	SPL::Mutex portMutex;

	// Websocket operations related member variables.
	// All values are primitive atomic values, no locking with receiver thread

	//In - out related variables set from sender thread only controlled by portMutex
	int numberOfAudioBlobFragmentsReceivedInCurrentConversation;
	SPL::int64 numberOfAudioSendInCurrentConversation;
	// is set when a media end is detected
	// In parallel regions window marker are broadcasted to all chains
	// so if an window marker is directly followed by a window marker, it will be ignored
	bool mediaEndReached;

	// The list of all o tuples of an conversation
	// The wastebasked is emptied after transcription was finalized and before a new conversation starts
	// The transcription is finalized when, the receiver thread has finished the sending of the last tuple
	// in a conversation and has receives the next 'listening' event, then the Rec::transcriptionFinalized flag is set
	// from receiver thread
	// This ensures that a o tuple is never used concurrently from sender and receiver thread
	std::vector<OT *> oTupleWastebasket;

	// Metrics completely controlled by sender thread
	SPL::int64 nFullAudioConversationsReceived;
	SPL::int64 nWebsocketConnectionAttempts;
	SPL::int64 nWebsocketConnectionAttemptsFailed;
	SPL::int64 nAudioBytesSend;

	// Custom metrics for this operator.
	SPL::Metric * const sttOutputResultModeMetric;
	SPL::Metric * const nFullAudioConversationsReceivedMetric;
	SPL::Metric * const nWebsocketConnectionAttemptsMetric;
	SPL::Metric * const nWebsocketConnectionAttemptsFailedMetric;
	SPL::Metric * const nAudioBytesSendMetric;
	//int pingSequenceNumber;
};

// The extraction function to extract the blob or reads the blob from file
// This function has 2 specializations DATA_TYPE=SPL::rstring and DATA_TYPE=SPL::blob
// Returns true success, when the file or the blob can be acquired without issues
// Return false when a file read error occurred
// The audioBytes and audioSize are always the pointer/size to the extracted data
// If a resource is allocated (the file read case) it is assigned to resource parameter
// Otherwise the resource pointer is null
// The primary template is never used so it is not defined
template<typename DATA_TYPE>
bool getSpeechSamples(
			DATA_TYPE const & input,
			unsigned char const * & audioBytes,
			uint64_t & audioSize,
			std::vector<unsigned char> * & resource,
			std::string & currentFileName);

template<typename OP, typename OT>
WatsonSTTImpl<OP, OT>::WatsonSTTImpl(OP & splOperator_,Conf config_)
:
		Rec(splOperator_, config_),

		accessTokenMutex(),
		accessToken(),

		portMutex(),

		numberOfAudioBlobFragmentsReceivedInCurrentConversation(0),
		numberOfAudioSendInCurrentConversation(0),
		mediaEndReached(true),
		oTupleWastebasket(),

		nFullAudioConversationsReceived(0),
		nWebsocketConnectionAttempts(0),
		nWebsocketConnectionAttemptsFailed(0),
		nAudioBytesSend(0),

		// Custom metrics for this operator are already defined in the operator model XML file.
		// Hence, there is no need to explicitly create them here.
		// Simply get the custom metrics already defined for this operator.
		// The update of metrics nFullAudioConversationsReceived and nAudioBytesSendMetric depends on parameter sttLiveMetricsUpdateNeeded
		sttOutputResultModeMetric{ & Rec::splOperator.getContext().getMetrics().getCustomMetricByName("sttOutputResultMode")},
		nFullAudioConversationsReceivedMetric{ & Rec::splOperator.getContext().getMetrics().getCustomMetricByName("nFullAudioConversationsReceived")},
		nWebsocketConnectionAttemptsMetric{ & Rec::splOperator.getContext().getMetrics().getCustomMetricByName("nWebsocketConnectionAttempts")},
		nWebsocketConnectionAttemptsFailedMetric{& Rec::splOperator.getContext().getMetrics().getCustomMetricByName("nWebsocketConnectionAttemptsFailed")},
		nAudioBytesSendMetric{ & Rec::splOperator.getContext().getMetrics().getCustomMetricByName("nAudioBytesSend")}
		//pingSequenceNumber(0)
{
	if (Conf::sttOutputResultMode == Conf::partial)
		if (not Conf::nonFinalUtterancesNeeded)
			Conf::sttOutputResultMode = Conf::final;

	if (Conf::customizationId == "") {
		// No customization id configured. Hence, set the customization weight to
		// 9.9 which will be ignored by the C++ logic later in the on_open method.
		Conf::customizationWeight = 9.9;
	}

	if (Conf::sttOutputResultMode < 1 || Conf::sttOutputResultMode > 3) {
		throw std::runtime_error(STTGW_INVALID_PARAM_VALUE_1("WatsonSTT", Conf::sttOutputResultMode));
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

	if (Conf::speechDetectorSensitivity < 0.0 || Conf::speechDetectorSensitivity > 1.0) {
		throw std::runtime_error(STTGW_INVALID_PARAM_VALUE_3("WatsonSTT", Conf::speechDetectorSensitivity, "speechDetectorSensitivity"));
	}

	if (Conf::backgroundAudioSuppression < 0.0 || Conf::backgroundAudioSuppression > 1.0) {
		throw std::runtime_error(STTGW_INVALID_PARAM_VALUE_3("WatsonSTT", Conf::backgroundAudioSuppression, "backgroundAudioSuppression"));
	}

	if (Conf::characterInsertionBias < -0.5 || Conf::characterInsertionBias > 1.0) {
		throw std::runtime_error(STTGW_INVALID_PARAM_VALUE_3("WatsonSTT", Conf::characterInsertionBias, "characterInsertionBias"));
	}

	// If the keywords to be spotted list is empty, then disable keywords_spotting.
	if (Conf::keywordsToBeSpotted.size() == 0) {
		Conf::keywordsSpottingThreshold = 0.0;
	}

	if (Conf::cpuYieldTimeInAudioSenderThread < 0.0) {
		throw std::runtime_error(STTGW_INVALID_PARAM_VALUE_4("WatsonSTT", Conf::cpuYieldTimeInAudioSenderThread, "cpuYieldTimeInAudioSenderThread", "0.0"));
	}

	if (Conf::maxConnectionRetryDelay < 1.0) {
		throw std::runtime_error(STTGW_INVALID_PARAM_VALUE_4("WatsonSTT", Conf::maxConnectionRetryDelay,  "maxConnectionRetryDelay", "1.0"));
	}

	// The parameters maxUtteranceAlternatives, wordAlternativesThreshold, keywordsSpottingThreshold, keywordsToBeSpotted
	// are not available in sttResultMode complete
	// The COF getUtteranceNumber, isFinalizedUtterance, getConfidence, getUtteranceAlternatives
	// are not available in sttResultMode complete

	// Update the operator metric.
	sttOutputResultModeMetric->setValueNoLock(Conf::sttOutputResultMode);

	//print the configuration
	std::cout << "WatsonSTT configuration:"
	<< "\nOperatorName                            = " << Rec::splOperator.getContext().getName()
	<< "\ncpuYieldTimeInAudioSenderThread         = " << Conf::cpuYieldTimeInAudioSenderThread
	<< "\nmaxConnectionRetryDelay                 = " << Conf::maxConnectionRetryDelay
	<< "\nreceiverWaitTimeWhenIdle                = " << Conf::receiverWaitTimeWhenIdle
	<< "\nsenderWaitTimeForTranscriptionFinalization=" << Conf::senderWaitTimeForTranscriptionFinalization
	<< "\nsenderWaitTimeForFinalReceiverState     = " << Conf::senderWaitTimeForFinalReceiverState
	<< "\nsttLiveMetricsUpdateNeeded              = " << Conf::sttLiveMetricsUpdateNeeded
	<< "\nuri                                     = " << Conf::uri
	<< "\nbaseLanguageModel                       = " << Conf::baseLanguageModel
	<< "\ncontentType                             = " << Conf::contentType
	<< "\nsttOutputResultMode                     = " << Conf::sttOutputResultMode
	<< "\nnonFinalUtterancesNeeded                = " << Conf::nonFinalUtterancesNeeded
	<< "\nsttRequestLogging                       = " << Conf::sttRequestLogging
	<< "\nbaseModelVersion                        = " << Conf::baseModelVersion
	<< "\ncustomizationId                         = " << Conf::customizationId
	<< "\ncustomizationWeight                     = " << Conf::customizationWeight
	<< "\nacousticCustomizationId                 = " << Conf::acousticCustomizationId
	<< "\nfilterProfanity                         = " << Conf::filterProfanity
	<< "\nmaxUtteranceAlternatives                = " << Conf::maxUtteranceAlternatives
	<< "\nwordAlternativesThreshold               = " << Conf::wordAlternativesThreshold
	<< "\nwordConfidenceNeeded                    = " << Conf::wordConfidenceNeeded
	<< "\nwordTimestampNeeded                     = " << Conf::wordTimestampNeeded
	<< "\nidentifySpeakers                        = " << Conf::identifySpeakers
	<< "\nspeakerUpdatesNeeded                    = " << Conf::speakerUpdatesNeeded
	<< "\nsmartFormattingNeeded                   = " << Conf::smartFormattingNeeded
	<< "\nredactionNeeded                         = " << Conf::redactionNeeded
	<< "\nkeywordsSpottingThreshold               = " << Conf::keywordsSpottingThreshold
	<< "\nkeywordsToBeSpotted                     = " << Conf::keywordsToBeSpotted
	<< "\nisTranscriptionCompletedRequested       = " << Conf::isTranscriptionCompletedRequested
	<< "\nspeechDetectorSensitivity               = " << Conf::speechDetectorSensitivity
	<< "\nbackgroundAudioSuppression              = " << Conf::backgroundAudioSuppression
	<< "\ncharacterInsertionBias                  = " << Conf::characterInsertionBias
	<< "\nconnectionState.wsState.is_lock_free()  = " << Rec::wsState.is_lock_free()
	<< "\nrecentOTuple.is_lock_free()             = " << Rec::recentOTuple.is_lock_free()
	<< "\n----------------------------------------------------------------" << std::endl;
}

template<typename OP, typename OT>
WatsonSTTImpl<OP, OT>::~WatsonSTTImpl() {
	for (auto x : oTupleWastebasket)
		delete x;
}

/* ping is not able to keep the connection
template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::allPortsReady() {
	// create the operator receiver thread and the ping thread
	uint32_t userThreadIndex = Rec::splOperator.createThreads(2);
	if (userThreadIndex != 0) {
		throw std::invalid_argument(Conf::traceIntro +" WatsonSTTImpl invalid userThreadIndex");
	}
}

template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::process(uint32_t idx) {
	SPLAPPTRC(L_INFO, Conf::traceIntro << "-->Run thread idx=" << idx, "ws_receiver");
	if (idx == 0)
		// run the ping trhead
		ping_init();
	else
		// run the operator receiver thread
		Rec::ws_init();
} */

template<typename OP, typename OT>
template<typename IT1, const SPL::rstring& (IT1::*GETTER)()const>
void WatsonSTTImpl<OP, OT>::process_1(IT1 const & inputTuple) {

	// The concurrent access from process_1 and process_0 is controlled by accessTokenMutex
	// The receiver thread gets an own copy of access token while the receiver thread is not active (connect)
	SPL::AutoMutex autoMutex(accessTokenMutex);

	// Save the access token for subsequent use within this operator.
	const SPL::rstring& at = (inputTuple.*GETTER)();
	accessToken = at;
	SPLAPPTRC(L_INFO, Conf::traceIntro << "-->Received new/refreshed access token.", "process_1");

	// This must be the audio data arriving here via port 0 i.e. first input port.
	// If we have a non-empty IAM access token, process the audio data.
	// Otherwise, skip it.
	if (accessToken.empty()) {
		SPLAPPLOG(L_ERROR, STTGW_EMPTY_IAM_TOKEN("WatsonSTT"), "process_1");
	}
}

//Definition of the template function getSpeechSamples if data type is SPL::blob
template<>
bool getSpeechSamples<SPL::blob>(
		SPL::blob const & input,
		unsigned char const * & audioBytes,
		uint64_t & audioSize,
		std::vector<unsigned char> * & resource,
		std::string & currentFileName)
{
	uint64_t sizeOfBlob = SPL::Functions::Collections::blobSize(input);

	resource = nullptr;
	audioSize = sizeOfBlob;
	if (sizeOfBlob == 0) {
		audioBytes = nullptr;
	} else {
		//audioBytes = input.releaseData(sizeOfBlob);
		audioBytes = input.getData();
	}
	// Data bluffer assigned or null
	return true;
}

//Definition of the template function getSpeechSamples if data type is rstring -> read file
template<>
bool getSpeechSamples<SPL::rstring>(
		SPL::rstring const & input,
		unsigned char const * & audioBytes,
		uint64_t & audioSize,
		std::vector<unsigned char> * & resource,
		std::string & currentFileName)
{
	SPLAPPTRC(L_DEBUG, "-->Sending file " << input, "ws_sender");
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
		SPLAPPTRC(L_ERROR, "-->Audio file not found. Skipping STT task for this file: " << input, "ws_sender");
		//No output buffer assigned
		resource = nullptr;
		audioSize = 0; audioBytes = nullptr;
		return false;

	} else {
		// Audio file exists. Read binary file into buffer
		std::ifstream inputStream(input.c_str(), std::ios::binary);
		std::vector<unsigned char>* buffer = new std::vector<unsigned char>(
				std::istreambuf_iterator<char>(inputStream), std::istreambuf_iterator<char>());
		resource = buffer;
		size_t fsize = buffer->size();
		SPLAPPTRC(L_DEBUG, "-->Sending file size" << fsize, "ws_sender");

		if (fsize == 0) {
			audioBytes = nullptr;
			audioSize = 0;
		} else {
			//Known problem: No error handling during read. Low risk because the existence of the file was checked before
			// Data buffer is assigned
			audioBytes = buffer->data();
			audioSize = fsize;
		}

		return true;
	} // End of if (stat(audioFileName.c_str(), &buffer) != 0)
}

template<typename OP, typename OT>
template<typename IT0, typename DATA_TYPE, DATA_TYPE const & (IT0::*GETTER)() const>
void WatsonSTTImpl<OP, OT>::process_0(IT0 const & inputTuple) {

	// serialize this method and processPunct and protect from issues when multiple threads send to this port
	SPL::AutoMutex autoMutex(portMutex);

	bool mediaEndReachedEntryState = mediaEndReached;
	if (mediaEndReached) {
		// We have a new connection attempt pending
		Rec::nextConversationQueued.store(true);

		// this tuple starts a new conversation
		++nFullAudioConversationsReceived;
		SPLAPPTRC(L_INFO, Conf::traceIntro << "-->PR0 Start a new conversation number " <<
				nFullAudioConversationsReceived, "ws_sender");
		if (Conf::sttLiveMetricsUpdateNeeded)
			nFullAudioConversationsReceivedMetric->setValueNoLock(nFullAudioConversationsReceived);

		// If the media end of the previous translation was reached, we wait until the translation has finalized.
		// A finalized transcription is signed through Rec::transcriptionFinalized
		// The is no need to acquire the state mutex here because we make no changes, we just wait for the transition
		// of Rec::transcriptionFinalized to false or an inactive receiver state.
		WsState myWsState = Rec::wsState.load();
		while (not Rec::transcriptionFinalized.load() && not receiverHasStopped(myWsState)) {
			SPLAPPTRC(L_TRACE, Conf::traceIntro <<
					"-->PR1 We have something to send but the previous transcription is not finalized, "
					" wsState=" << wsStateToString(myWsState) << " block for " <<
					Conf::senderWaitTimeForTranscriptionFinalization << " second",
					"ws_sender");
			SPL::Functions::Utility::block(Conf::senderWaitTimeForTranscriptionFinalization);
			myWsState = Rec::wsState.load();
			if (Rec::splOperator.getPE().getShutdownRequested())
				return;
		}
		// Here is the receiver either dead or a transcription has finalized
		// A new transcription has not yet been started, hence no race condition can occur
		Rec::transcriptionFinalized.store(false);
		mediaEndReached = false;
		// this is the first blob in a conversation
		numberOfAudioBlobFragmentsReceivedInCurrentConversation = 0;
		numberOfAudioSendInCurrentConversation = 0;

		// no current conversation is ongoing -> clear recentOTuple to be on the save side
		// The recentOTuple is not longer needed when the transcription is finalized
		// recentOTuple is cleared from the receiver task
		Rec::recentOTuple.store(nullptr);

		// recycle all oTuples from wastebasket
		for (OT* x : oTupleWastebasket)
			delete x;
		oTupleWastebasket.clear();
	} // END if (mediaEndReached)

	// Get the file and the file read result here
	//get input
	DATA_TYPE const & mySpeechAttribute = (inputTuple.*GETTER)();

	unsigned char const * myAudioBytes = nullptr;
	uint64_t myAudioSize = 0ul;
	std::vector<unsigned char> * buffer_ = nullptr;
	std::string currentFile;
	bool fileReadResult = getSpeechSamples(mySpeechAttribute, myAudioBytes, myAudioSize, buffer_, currentFile);
	// ensure release of resource with unique_ptr
	std::unique_ptr<std::vector<unsigned char> > myBuffer(buffer_);

	// This must be the audio data arriving here via port 0 i.e. first input port.
	// If we have a non-empty IAM access token, process the audio data.
	// Otherwise, wait until an access token is available
	while(true) {
		std::string myAccessToken;
		{
			SPL::AutoMutex autoMutex(accessTokenMutex);
			myAccessToken = accessToken;
		}
		if (myAccessToken.empty()) {
			SPLAPPTRC(L_ERROR, Conf::traceIntro << "-->PR9 Wait for access token due to an empty IAM access "
					"token. User must first provide the IAM access token before sending any audio data to this operator.",
					"ws_sender");
			SPL::Functions::Utility::block(Conf::senderWaitTimeEmptyAccessToken);
			if (Rec::splOperator.getPE().getShutdownRequested())
				return;
		} else {
			break;
		}
	}

	// log the file read error occurred
	if (not fileReadResult) {
		std::string errorMsg;
		if (mediaEndReachedEntryState) {
			errorMsg = Conf::traceIntro +
					"-->Read error in the first segment of an conversation. Skipping STT task. File: " + currentFile;
			SPLAPPTRC(L_ERROR, errorMsg, "ws_sender");
		} else {
			errorMsg = Conf::traceIntro +
					"-->Read error in a subsequent segment of an conversation. Close STT task. File: " + currentFile;
			SPLAPPTRC(L_ERROR, errorMsg, "ws_sender");
		}
		connect();
		if (Rec::splOperator.getPE().getShutdownRequested())
			return;
		// Do not send a tuple here because of probably multi threading issues
		// Create an output tuple with the error text and auto assign from current input tuple
		OT * myOTuple = Rec::splOperator.createOutTupleAndAutoAssign(inputTuple);
		// Assign error message and send the tuple
		Rec::splOperator.appendErrorAttribute(myOTuple, errorMsg);
		oTupleWastebasket.push_back(myOTuple);
		Rec::recentOTuple.store(myOTuple);
		// here we must be in listening state
		// ignore race condition if state enters a different state
		mediaEndReached = true;
		Rec::nextConversationQueued.store(false);
		sendActionStop();

	} else { // Result success
		connect();
		if (Rec::splOperator.getPE().getShutdownRequested())
			return;
		// here we must be in listening state
		// ignore race condition if state enters a different state
		OT * nextOTuple = Rec::splOperator.createOutTupleAndAutoAssign(inputTuple);
		oTupleWastebasket.push_back(nextOTuple);
		Rec::recentOTuple.store(nextOTuple);

		++numberOfAudioBlobFragmentsReceivedInCurrentConversation;
		numberOfAudioSendInCurrentConversation = numberOfAudioSendInCurrentConversation + myAudioSize;
		nAudioBytesSend = nAudioBytesSend + myAudioSize;
		if (Conf::sttLiveMetricsUpdateNeeded)
			nAudioBytesSendMetric->setValueNoLock(nAudioBytesSend);

		sendDataToSTT(myAudioBytes, myAudioSize);
		// send end in case of empty data blob
		if (myAudioBytes == 0) {
			mediaEndReached = true;
			Rec::nextConversationQueued.store(false);
			sendActionStop();
		}
	}
} // End: WatsonSTTImpl<OP, OT>::process_0

// Punctuation processing for data port 0 Window Markers
template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::processPunct_0(SPL::Punctuation const & punct) {
	// serialize this method and process and protect from issues when multiple threads send to this port
	SPL::AutoMutex autoMutex(portMutex);

	if (punct == SPL::Punctuation::WindowMarker) {
		if (not mediaEndReached) {
			// Ignore message if not in listening state
			mediaEndReached = true;
			Rec::nextConversationQueued.store(false);
			sendActionStop();
		} else {
			SPLAPPTRC(L_TRACE, Conf::traceIntro << "PP1 Ignore window marker without data", "ws_sender");
		}
	} else if (punct == SPL::Punctuation::FinalMarker) {
		// final marker wait until the current conversation ends if any
		WsState myWsState = Rec::wsState.load();
		while(not Rec::transcriptionFinalized.load() && not receiverHasStopped(myWsState) && not Rec::splOperator.getPE().getShutdownRequested()) {
			SPLAPPTRC(L_TRACE, Conf::traceIntro <<
					"-->PP2 Final punct received wait for transcription end, "
					" wsState=" << wsStateToString(myWsState) << " block for " <<
					Conf::senderWaitTimeForTranscriptionFinalization << " second",
					"ws_sender");
			SPL::Functions::Utility::block(Conf::senderWaitTimeForTranscriptionFinalization);
			myWsState = Rec::wsState.load();
		}
		Rec::splOperator.submit(punct, 0);
	}
}

template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::connect() {
	SPLAPPTRC(L_DEBUG, Conf::traceIntro << "-->CS0 connect()", "ws_sender");

	// We make a new connection or use a existing connection if data are to send
	WsState myWsState = Rec::wsState.load();
	bool firstLog = true;
	while (myWsState != WsState::listening) {

		if (receiverHasTransientState(myWsState)) {
			// Connection was already triggered or is shutting down
			if (firstLog) {
				firstLog = false;
				SPLAPPTRC(L_DEBUG, Conf::traceIntro <<
						"-->CS3 Something to sent but receiver is in transient state: " << wsStateToString(myWsState)
						<< " wait " << Conf::senderWaitTimeForFinalReceiverState, "ws_sender");
			}
			SPL::Functions::Utility::block(Conf::senderWaitTimeForFinalReceiverState);
		} else {
			// Make a new connection attempt
			// The receiver thread must have reached a final state
			// Here the state must not be: start, connecting, open, listening, error
			SPL::float64 nConnectAtempts = Rec::getNWebsocketConnectionAttemptsCurrent();
			if (nConnectAtempts > 0) {
				// Delay repeated connection requests
				SPL::float64 waitTime = pow(2.0, nConnectAtempts);
				if (waitTime > Conf::maxConnectionRetryDelay)
					waitTime = Conf::maxConnectionRetryDelay;
				SPLAPPTRC(L_WARN, Conf::traceIntro << "-->CS2 Delay repeated connection requests wait " << waitTime, "ws_sender");
				SPL::Functions::Utility::block(waitTime);
			}
			++nWebsocketConnectionAttempts;
			nWebsocketConnectionAttemptsMetric->setValueNoLock(nWebsocketConnectionAttempts);
			if (nConnectAtempts > 0) {
				++nWebsocketConnectionAttemptsFailed;
				nWebsocketConnectionAttemptsFailedMetric->setValueNoLock(nWebsocketConnectionAttemptsFailed);
			}
			Rec::incrementNWebsocketConnectionAttemptsCurrent();
			SPLAPPTRC(L_INFO, Conf::traceIntro << "-->CS5 Make a connection attempt number " <<
					Rec::getNWebsocketConnectionAttemptsCurrent() << " from wsState=" << wsStateToString(myWsState),
					"ws_sender");

			// The receiver thread is in an inactive state: Now store the access token to receiver thread variable
			std::string myAccessToken;
			{
				SPL::AutoMutex autoMutex(accessTokenMutex);
				myAccessToken = accessToken;
			}
			Rec::accessToken = myAccessToken;

			// make the connection attempt
			Rec::setWsState(WsState::start);
		}
		if (Rec::splOperator.getPE().getShutdownRequested()) {
			return;
		}
		myWsState = Rec::wsState.load();
	} // END: while (not connectionState.Rec::wsConnectionEstablished)
}

// send the data requires the listening state
template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::sendDataToSTT(unsigned char const * audioBytes, uint64_t audioSize) {
	SPLAPPTRC(L_DEBUG, Conf::traceIntro << "-->CS0 sendDataToSTT(audioBytes=" <<
			static_cast<const void*>(audioBytes) << ", audioSize=" << audioSize, "ws_sender");

	// send bytes but only if size > 0
	if (audioSize > 0) {
		// put new otuple to recentOTuple and to wastebasket
		// if nextOTuple is null, the old recentOTuple is kept

		if (Conf::cpuYieldTimeInAudioSenderThread > 0.0) {
			// Audio data available for processing. Yield the CPU briefly and get to work soon.
			// Even a tiny value of 1 millisecond (0.001 second) will yield the
			// CPU and will not show 0% idle in the Linux top command.
			SPLAPPTRC(L_TRACE, Conf::traceIntro << "-->CS1 Going to send data , "
					"block for cpuYieldTimeInAudioSenderThread=" << Conf::cpuYieldTimeInAudioSenderThread,
					"ws_sender");
			SPL::Functions::Utility::block(Conf::cpuYieldTimeInAudioSenderThread);
		}

		// Speech data is sent into this operator via a blob buffer rather than via a file.
		// Send the blob data (either in full or in a partial fragment) to the STT service.
		// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-websockets#WSaudio
		// c->get_alog().write(websocketpp::log::alevel::app, "Sent binary Message: " + boost::to_string(buffer.size()));
		websocketpp::lib::error_code ec;
		Rec::wsClient->send(Rec::wsHandle, audioBytes, audioSize, websocketpp::frame::opcode::binary, ec);
		//Rec::statusOfAudioDataTransmissionToSTT = AUDIO_BLOB_FRAGMENTS_BEING_SENT_TO_STT;
		if (ec) {
			SPLAPPTRC(L_ERROR, Conf::traceIntro << "-->CS9 Error when send connectAndSendDataToSTT ec=" << ec <<
					" message=" << ec.message(), "ws_sender");
		}
	} // END: if (audioSize > 0)

	return;
} // End: WatsonSTTImpl<OP, OT>::sendDataToSTT

template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::sendActionStop() {

	WsState myWsState = Rec::wsState.load();
	if (myWsState == WsState::listening) {

		SPLAPPTRC(L_INFO, Conf::traceIntro << "-->CS6 Send \"action\" : \"stop\"", "ws_sender");
		// We reached the end of the blob data as sent/streamed from the SPL application.
		// Signal end of the audio data.
		// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-websockets#WSstop
		websocketpp::lib::error_code ec{};
		Rec::wsClient->send(Rec::wsHandle, "{\"action\" : \"stop\"}" , websocketpp::frame::opcode::text, ec);
		// In a blob based audio data, the entire blob has been sent to the STT service at this time.
		// So set this flag to indicate that.
		//Rec::statusOfAudioDataTransmissionToSTT = FULL_AUDIO_DATA_SENT_TO_STT;
		if (ec) {
			SPLAPPTRC(L_ERROR, Conf::traceIntro << "-->CS10 Error in sendActionStop ec=" << ec <<
					" message=" << ec.message(), "ws_sender");
		}
	} else {
		SPLAPPTRC(L_ERROR, Conf::traceIntro <<
				"-->CS11 Connection state myWsState != WsState::listening but sendActionStop requested. Ignoring",
				"ws_sender");
	}

	return;
}

/* ping is not able to keep the connection
template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::ping_init() {
	while (not Rec::splOperator.getPE().getShutdownRequested()) {
		SPL::Functions::Utility::block(Conf::senderPingPeriod);
		if (not Rec::splOperator.getPE().getShutdownRequested()) {

			if (Rec::wsState.load() == WsState::listening) {
				std::string pingmessage{"pong_" + std::to_string(pingSequenceNumber++)};
				websocketpp::lib::error_code ec{};
				Rec::wsClient->ping(Rec::wsHandle, pingmessage, ec);
				if (ec)
					SPLAPPTRC(L_ERROR, Conf::traceIntro << "-->CS99 Error when send ping ec=" << ec << " message=" << ec.message(), "ws_sender");
			}
		}
	}
} */
}}}}

#endif /* COM_IBM_STREAMS_STTGATEWAY_WATSONSTTIMPL_HPP_ */
