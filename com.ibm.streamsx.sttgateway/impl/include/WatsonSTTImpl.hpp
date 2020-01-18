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

namespace com { namespace ibm { namespace streams { namespace sttgateway {

// Websocket related type definitions.
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
// Pull out the type of messages sent by our config
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;

enum StatusOfAudioDataTransmission {NO_AUDIO_DATA_SENT_TO_STT, AUDIO_BLOB_FRAGMENTS_BEING_SENT_TO_STT, FULL_AUDIO_DATA_SENT_TO_STT};

/*
 * Implementation class for operator Watson STT
 * Move almost of the c++ code of the operator into this class to take the advantage of c++ editor support
 *
 * Template argument : OP: Operator type (must inherit from SPL::Operator)
 *                     OT: Output Tuple type
 */
template<typename OP, typename OT>
class WatsonSTTImpl {
public:
	//Constructors
	WatsonSTTImpl(
			OP & splOperator_,
			bool websocketLoggingNeeded_,
			SPL::float64 cpuYieldTimeInAudioSenderThread_,
			SPL::float64 waitTimeBeforeSTTServiceConnectionRetry_,
			SPL::int32 connectionAttemptsThreshold_,
			bool sttLiveMetricsUpdateNeeded_,
			const std::string& uri_,
			const std::string& baseLanguageModel_,
			const std::string& contentType_,
			SPL::int32 sttResultMode_,
			bool sttRequestLogging_,
			const std::string& baseModelVersion_,
			const std::string& customizationId_,
			SPL::float64 customizationWeight_,
			const std::string& acousticCustomizationId_,
			bool filterProfanity_,
			bool sttJsonResponseDebugging_,
			SPL::int32 maxUtteranceAlternatives_,
			SPL::float64 wordAlternativesThreshold_,
			bool wordConfidenceNeeded_,
			bool wordTimestampNeeded_,
			bool identifySpeakers_,
			bool smartFormattingNeeded_,
			SPL::float64 keywordsSpottingThreshold_,
			const SPL::list<SPL::rstring>& keywordsToBeSpotted_
);
	//WatsonSTTImpl(WatsonSTTImpl const &) = delete;

	//Destructor
	~WatsonSTTImpl();
protected:
	// Notify port readiness
	void allPortsReady();

	// Notify pending shutdown
	void prepareToShutdown();

	// Tuple processing for mutating data port 0
	template<typename IT0, typename DATA_TYPE, DATA_TYPE & (IT0::*GETTER)()>
	void process_0(IT0 & inputTuple);

	// Tuple processing for authentication port 1
	template<typename IT1, const SPL::rstring& (IT1::*GETTER)()const>
	void process_1(IT1 const & inputTuple);

	// Processing for websocket client threads
	void process(uint32_t idx);


private:
	// All the public methods below used to be static methods with
	// a static keyword at the beginning of every prototype
	// declaration. On Aug/28/2018, I removed the need for them to be static.
	//
	// Websocket connection open event handler
	void on_open(client* c, websocketpp::connection_hdl hdl);

	// Websocket message reception event handler
	void on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg);

	// Websocket connection close event handler
	void on_close(client* c, websocketpp::connection_hdl hdl);

	// Websocket TLS binding event handler
	context_ptr on_tls_init(client* c, websocketpp::connection_hdl);

	// Webscoket connection failure event handler
	void on_fail(client* c, websocketpp::connection_hdl hdl);

	// Websocket initialization thread method
	void ws_init();

	// Websocket audio blob sender thread method
	void ws_audio_blob_sender();

private:
	OP & splOperator;

protected:
	// Operator related member variables
	// We build our own intro for trace outputs
	const std::string operatorPhysicalName;
	const SPL::int32 udpChannelNumber;
	const std::string traceIntro;
private:
	// Websocket operations related member variables.
	bool wsConnectionEstablished;
	bool makeNewWebsocketConnection;
	bool websocketConnectionErrorOccurred;
	SPL::int64 numberOfWebsocketConnectionAttempts;
	client *wsClient;
	websocketpp::connection_hdl wsHandle;

	//In - out related varaibles
	std::vector<unsigned char *> audioBytes;
	std::vector<uint64_t> audioSize;
	int numberOfAudioBlobFragmentsReceivedInCurrentConversation;
	std::vector<OT *> oTupleList;

	//Internal state
	SPL::Mutex sttMutex1;

	SPL::int64 numberOfFullAudioConversationsReceived;
	SPL::int64 numberOfFullAudioConversationsTranscribed;
	StatusOfAudioDataTransmission statusOfAudioDataTransmissionToSTT;
	std::string transcriptionResult;
	bool transcriptionErrorOccurred;
	bool sttResultTupleWaitingToBeSent;

	SPL::list<SPL::int32> utteranceWordsSpeakers;
	SPL::list<SPL::float64> utteranceWordsSpeakersConfidences;
	SPL::list<SPL::float64> utteranceWordsStartTimes;

	std::string accessToken;

	//Configuration values
	const bool websocketLoggingNeeded;
	const SPL::float64 cpuYieldTimeInAudioSenderThread;
	const SPL::float64 waitTimeBeforeSTTServiceConnectionRetry;
	//This time becomes effective, when the connectionAttemptsThreshold limit is exceeded
	static const SPL::float64 waitTimeBeforeSTTServiceConnectionRetryLong;
	const SPL::int32 connectionAttemptsThreshold;
	const bool sttLiveMetricsUpdateNeeded;
	const std::string uri;
	const std::string baseLanguageModel;
	const std::string contentType;
	const SPL::int32 sttResultMode;
	const bool sttRequestLogging;
	const std::string baseModelVersion;
	const std::string customizationId;
	SPL::float64 customizationWeight;
	const std::string acousticCustomizationId;
	const bool filterProfanity;
	const bool sttJsonResponseDebugging;
	SPL::int32 maxUtteranceAlternatives;
	SPL::float64 wordAlternativesThreshold;
	bool wordConfidenceNeeded;
	bool wordTimestampNeeded;
	bool identifySpeakers;
	const bool smartFormattingNeeded;
	SPL::float64 keywordsSpottingThreshold;
	SPL::list<SPL::rstring> keywordsToBeSpotted;

	// Custom metrics for this operator.
	SPL::Metric * const nWebsocketConnectionAttemptsMetric;
	SPL::Metric * const nFullAudioConversationsReceivedMetric;
	SPL::Metric * const nFullAudioConversationsTranscribedMetric;
	SPL::Metric * const nSTTResultModeMetric;

	// These are the output attribute assignment functions
protected:
	/*SPL::int32 getUtteranceNumber(int32_t const & utteranceNumber) { return(utteranceNumber); }
	SPL::rstring getUtteranceText(std::string const & utteranceText) { return(utteranceText); }
	SPL::boolean isFinalizedUtterance(bool const & finalizedUtterance) { return(finalizedUtterance); }
	SPL::float32 getConfidence(float const & confidence) { return(confidence); }
	SPL::rstring getFullTranscriptionText(std::string const & fullText) { return(fullText); }
	SPL::boolean isTranscriptionCompleted(bool const & transcriptionCompleted) { return(transcriptionCompleted); }
	SPL::list<SPL::rstring> getUtteranceAlternatives(SPL::list<SPL::rstring> const & utteranceAlternatives) { return(utteranceAlternatives); }
	SPL::list<SPL::list<SPL::rstring>> getWordAlternatives(SPL::list<SPL::list<SPL::rstring>> const & wordAlternatives) { return(wordAlternatives); }
	SPL::list<SPL::list<SPL::float64>> getWordAlternativesConfidences(SPL::list<SPL::list<SPL::float64>> const & wordAlternativesConfidences) { return(wordAlternativesConfidences); }
	SPL::list<SPL::float64> getWordAlternativesStartTimes(SPL::list<SPL::float64> const & wordAlternativesStartTimes) { return(wordAlternativesStartTimes); }
	SPL::list<SPL::float64> getWordAlternativesEndTimes(SPL::list<SPL::float64> const & wordAlternativesEndTimes) { return(wordAlternativesEndTimes); }
	SPL::list<SPL::rstring> getUtteranceWords(SPL::list<SPL::rstring> const & utteranceWords) { return(utteranceWords); }
	SPL::list<SPL::float64> getUtteranceWordsConfidences(SPL::list<SPL::float64> const & utteranceWordsConfidences) { return(utteranceWordsConfidences); }*/
	inline SPL::list<SPL::float64> getUtteranceWordsStartTimes() { return(utteranceWordsStartTimes); }
	/*SPL::list<SPL::float64> getUtteranceWordsEndTimes(SPL::list<SPL::float64> const & utteranceWordsEndTimes) { return(utteranceWordsEndTimes); }
	SPL::float64 getUtteranceStartTime(SPL::float64 const & utteranceStartTime) { return(utteranceStartTime); }
	SPL::float64 getUtteranceEndTime(SPL::float64 const & utteranceEndTime) { return(utteranceEndTime); }*/
	inline SPL::list<SPL::int32> getUtteranceWordsSpeakers() { return(utteranceWordsSpeakers); }
	inline SPL::list<SPL::float64> getUtteranceWordsSpeakersConfidences() { return(utteranceWordsSpeakersConfidences); }
	/*SPL::map<SPL::rstring, SPL::list<SPL::map<SPL::rstring, SPL::float64>>>
		getKeywordsSpottingResults(SPL::map<SPL::rstring,
		SPL::list<SPL::map<SPL::rstring, SPL::float64>>> const & keywordsSpottingResults) { return(keywordsSpottingResults); }*/
};

// The extraction function to extract the blob or the blob from filename into the blob list and size list
// Returns 0 if success; 1 if tuple has to be ignored (repeated empty blob); 2 file read error
template<typename DATA_TYPE>
int getSpeechSamples(
			DATA_TYPE & input,
			std::string const & traceIntro,
			std::vector<unsigned char *>& audioBytes,
			std::vector<uint64_t>& audioSize,
			int& numberOfAudioBlobFragmentsReceivedInCurrentConversation,
			SPL::int64& numberOfFullAudioConversationsReceived,
			std::string audioErrorFileName);

template<typename OP, typename OT>
const SPL::float64 WatsonSTTImpl<OP, OT>::waitTimeBeforeSTTServiceConnectionRetryLong = 60.0;

template<typename OP, typename OT>
WatsonSTTImpl<OP, OT>::WatsonSTTImpl(
		OP & splOperator_,
		bool websocketLoggingNeeded_,
		SPL::float64 cpuYieldTimeInAudioSenderThread_,
		SPL::float64 waitTimeBeforeSTTServiceConnectionRetry_,
		SPL::int32 connectionAttemptsThreshold_,
		bool sttLiveMetricsUpdateNeeded_,
		const std::string& uri_,
		const std::string& baseLanguageModel_,
		const std::string& contentType_,
		SPL::int32 sttResultMode_,
		bool sttRequestLogging_,
		const std::string& baseModelVersion_,
		const std::string& customizationId_,
		SPL::float64 customizationWeight_,
		const std::string& acousticCustomizationId_,
		bool filterProfanity_,
		bool sttJsonResponseDebugging_,
		SPL::int32 maxUtteranceAlternatives_,
		SPL::float64 wordAlternativesThreshold_,
		bool wordConfidenceNeeded_,
		bool wordTimestampNeeded_,
		bool identifySpeakers_,
		bool smartFormattingNeeded_,
		SPL::float64 keywordsSpottingThreshold_,
		const SPL::list<SPL::rstring>& keywordsToBeSpotted_)
:
		splOperator(splOperator_),
		operatorPhysicalName{splOperator.getContext().getName()},
		udpChannelNumber{splOperator.getContext().getChannel()},
		traceIntro{"Operator " + operatorPhysicalName + "-->Channel " + boost::to_string(udpChannelNumber)},

		wsConnectionEstablished{false},
		makeNewWebsocketConnection{false},
		websocketConnectionErrorOccurred{false},
		numberOfWebsocketConnectionAttempts{0},
		wsClient{},
		wsHandle{},

		audioBytes{},
		audioSize{},
		numberOfAudioBlobFragmentsReceivedInCurrentConversation{0},
		oTupleList{},

		sttMutex1{},

		numberOfFullAudioConversationsReceived{0},
		numberOfFullAudioConversationsTranscribed{0},
		// 0 = NO_AUDIO_DATA_SENT_TO_STT, 1 = PARITAL_BLOB_FRAGMENTS_BEING_SENT_TO_STT,
		// 2 = FULL_AUDIO_DATA_SENT_TO_STT
		statusOfAudioDataTransmissionToSTT{NO_AUDIO_DATA_SENT_TO_STT},
		transcriptionResult{},
		transcriptionErrorOccurred{false},
		sttResultTupleWaitingToBeSent{false},
		utteranceWordsSpeakers{},
		utteranceWordsSpeakersConfidences{},
		utteranceWordsStartTimes{},

		accessToken{},

		websocketLoggingNeeded{websocketLoggingNeeded_},
		cpuYieldTimeInAudioSenderThread{cpuYieldTimeInAudioSenderThread_},
		waitTimeBeforeSTTServiceConnectionRetry{waitTimeBeforeSTTServiceConnectionRetry_},
		connectionAttemptsThreshold{connectionAttemptsThreshold_},
		sttLiveMetricsUpdateNeeded{sttLiveMetricsUpdateNeeded_},
		uri{uri_},
		baseLanguageModel{baseLanguageModel_},
		contentType{contentType_},
		sttResultMode{sttResultMode_},
		sttRequestLogging{sttRequestLogging_},
		baseModelVersion{baseModelVersion_},
		customizationId{customizationId_},
		customizationWeight{customizationWeight_},
		acousticCustomizationId{acousticCustomizationId_},
		filterProfanity{filterProfanity_},
		sttJsonResponseDebugging{sttJsonResponseDebugging_},
		maxUtteranceAlternatives{maxUtteranceAlternatives_},
		wordAlternativesThreshold{wordAlternativesThreshold_},
		wordConfidenceNeeded{wordConfidenceNeeded_},
		wordTimestampNeeded{wordTimestampNeeded_},
		identifySpeakers{identifySpeakers_},
		smartFormattingNeeded{smartFormattingNeeded_},
		keywordsSpottingThreshold{keywordsSpottingThreshold_},
		keywordsToBeSpotted{keywordsToBeSpotted_},
		// Custom metrics for this operator are already defined in the operator model XML file.
		// Hence, there is no need to explicitly create them here.
		// Simply get the custom metrics already defined for this operator.
		// The update of metrics nFullAudioConversationsReceived and nFullAudioConversationsTranscribed depends on parameter sttLiveMetricsUpdateNeeded
		nWebsocketConnectionAttemptsMetric{ & splOperator.getContext().getMetrics().getCustomMetricByName("nWebsocketConnectionAttempts")},
		nFullAudioConversationsReceivedMetric{ & splOperator.getContext().getMetrics().getCustomMetricByName("nFullAudioConversationsReceived")},
		nFullAudioConversationsTranscribedMetric{ & splOperator.getContext().getMetrics().getCustomMetricByName("nFullAudioConversationsTranscribed")},
		nSTTResultModeMetric{ & splOperator.getContext().getMetrics().getCustomMetricByName("nSTTResultMode")}
{
	if (customizationId == "") {
		// No customization id configured. Hence, set the customization weight to
		// 9.9 which will be ignored by the C++ logic later in the on_open method.
		customizationWeight = 9.9;
	}

	if (sttResultMode < 1 || sttResultMode > 3) {
		throw std::runtime_error(
			"WatsonSTT_cpp.cgt: Invalid value of " +
			boost::to_string(sttResultMode) + " is given for the sttResultMode parameter." +
			" Valid value must be either 1 or 2 or 3.");
	}

	if (maxUtteranceAlternatives <= 0) {
		throw std::runtime_error(
			"WatsonSTT_cpp.cgt: Invalid value of " +
			boost::to_string(maxUtteranceAlternatives) + " is given for the maxUtteranceAlternatives parameter." +
			" Valid value must be greater than 0.");
	}

	if (wordAlternativesThreshold < 0.0 || wordAlternativesThreshold >= 1.0) {
		throw std::runtime_error(
			"WatsonSTT_cpp.cgt: Invalid value of " +
			boost::to_string(wordAlternativesThreshold) + " is given for the wordAlternativesThreshold parameter." +
			" Valid value must be greater than or equal to 0.0 and less than 1.0.");
	}

	if (keywordsSpottingThreshold < 0.0 || keywordsSpottingThreshold >= 1.0) {
		throw std::runtime_error(
			"WatsonSTT_cpp.cgt: Invalid value of " +
			boost::to_string(keywordsSpottingThreshold) + " is given for the keywordsSpottingThreshold parameter." +
			" Valid value must be greater than or equal to 0.0 and less than 1.0.");
	}

	// If the keywords to be spotted list is empty, then disable keywords_spotting.
	if (keywordsToBeSpotted.size() == 0) {
		keywordsSpottingThreshold = 0.0;
	}

	if (cpuYieldTimeInAudioSenderThread < 0.0) {
		throw std::runtime_error(
			"WatsonSTT_cpp.cgt: Invalid value of " +
			boost::to_string(cpuYieldTimeInAudioSenderThread) + " is given for the cpuYieldTimeInAudioSenderThread parameter." +
			" Valid value must be greater than or equal to 0.0.");
	}

	if (waitTimeBeforeSTTServiceConnectionRetry < 1.0) {
		throw std::runtime_error(
			"WatsonSTT_cpp.cgt: Invalid value of " +
			boost::to_string(waitTimeBeforeSTTServiceConnectionRetry) + " is given for the waitTimeBeforeSTTServiceConnectionRetry parameter." +
			" Valid value must be greater than or equal to 1.0.");
	}

	if (connectionAttemptsThreshold < 1) {
		throw std::runtime_error(
			"WatsonSTT_cpp.cgt: Invalid value of " +
			boost::to_string(connectionAttemptsThreshold) + " is given for the connectionAttemptsThreshold parameter." +
			" Valid value must be greater than or equal to 1.");
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
	if (sttResultMode == 3) {
		// No n-best utterance alternative hypotheses.
		maxUtteranceAlternatives = 1;
		// No Confusion Networks.
		wordAlternativesThreshold = 0.0;
		// No individual word confidences.
		wordConfidenceNeeded = false;
		// No individual timestamps.
		wordTimestampNeeded = false;
		//No speaker identification for the individual words in an utterance.
		identifySpeakers = false;
		//No keyword spotting inside an utterance.
		keywordsSpottingThreshold = 0.0;
	}

	// Update the operator metric.
	nSTTResultModeMetric->setValueNoLock(sttResultMode);

	//print the configuration
	std::cout << "WatsonSTT configuration:"
	<< "\nOperatorName                            = " << splOperator.getContext().getName()
	<< "\ncpuYieldTimeInAudioSenderThread         = " << cpuYieldTimeInAudioSenderThread
	<< "\nwaitTimeBeforeSTTServiceConnectionRetry = " << waitTimeBeforeSTTServiceConnectionRetry
	<< "\nwaitTimeBeforeSTTServiceConnectionRetryLong=" << waitTimeBeforeSTTServiceConnectionRetryLong
	<< "\nconnectionAttemptsThreshold             = " << connectionAttemptsThreshold
	<< "\nsttLiveMetricsUpdateNeeded              = " << sttLiveMetricsUpdateNeeded
	<< "\nuri                                     = " << uri
	<< "\nbaseLanguageModel                       = " << baseLanguageModel
	<< "\ncontentType                             = " << contentType
	<< "\nsttResultMode                           = " << sttResultMode
	<< "\nsttRequestLogging                       = " << sttRequestLogging
	<< "\nbaseModelVersion                        = " << baseModelVersion
	<< "\ncustomizationId                         = " << customizationId
	<< "\ncustomizationWeight                     = " << customizationWeight
	<< "\nacousticCustomizationId                 = " << acousticCustomizationId
	<< "\nfilterProfanity                         = " << filterProfanity
	<< "\nsttJsonResponseDebugging                = " << sttJsonResponseDebugging
	<< "\nmaxUtteranceAlternatives                = " << maxUtteranceAlternatives
	<< "\nwordAlternativesThreshold               = " << wordAlternativesThreshold
	<< "\nwordConfidenceNeeded                    = " << wordConfidenceNeeded
	<< "\nwordTimestampNeeded                     = " << wordTimestampNeeded
	<< "\nidentifySpeakers                        = " << identifySpeakers
	<< "\nsmartFormattingNeeded                   = " << smartFormattingNeeded
	<< "\nkeywordsSpottingThreshold               = " << keywordsSpottingThreshold
	<< "\nkeywordsToBeSpotted                     = " << keywordsToBeSpotted
	<< "\naudioInputAsBlob                        = " << "<%=$audioInputAsBlob%>"
	<< "\n----------------------------------------------------------------" << std::endl;
}

template<typename OP, typename OT>
WatsonSTTImpl<OP, OT>::~WatsonSTTImpl() {
	if (wsClient) {
		delete wsClient;
	}
}

template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::allPortsReady() {
	uint32_t userThreadIndex = splOperator.createThreads(2);
	if (userThreadIndex != 0) {
		throw std::invalid_argument(traceIntro +" WatsonSTTImpl invalid userThreadIndex");
	}
}

template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::prepareToShutdown() {
	// Close the Websocket connection to the Watson STT service.
	// wsClient->get_alog().write(websocketpp::log::alevel::app, "Client is closing the Websocket connection to the Watson STT service.");
	try {
		if (wsClient) {
			SPLAPPTRC(L_INFO, traceIntro <<
				"-->Client is trying to close the Websocket connection to the Watson STT service.",
				"prepareToShutdown");
			if ( ! wsHandle.expired()) {
				SPLAPPTRC(L_INFO, traceIntro <<
					"-->Client is closing the Websocket connection to the Watson STT service.",
					"prepareToShutdown");
				wsClient->close(wsHandle,websocketpp::close::status::normal,"");
			}
		} else {
			SPLAPPTRC(L_INFO, traceIntro <<
				"-->Client is null.",
				"prepareToShutdown");
		}
	} catch (const std::exception& e) {
		SPLAPPTRC(L_ERROR, traceIntro <<
			"-->Exception during closing. " << e.what(),
			"prepareToShutdown");
	}
}

template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::process(uint32_t idx) {
	SPLAPPTRC(L_INFO, traceIntro << "--Run thread idx=" << idx, "process");
	if (idx == 0)
		ws_init();
	else
		ws_audio_blob_sender();
}

template<typename OP, typename OT>
template<typename IT1, const SPL::rstring& (IT1::*GETTER)()const>
void WatsonSTTImpl<OP, OT>::process_1(IT1 const & inputTuple) {
	// There are multiple methods (process, audio_blob_sender and on_message) that
	// regularly access (read, write and delete) the vector member variables.
	// All those methods work in parallel inside their own threads.
	// To make that vector access thread safe, we will use this mutex.
	SPL::AutoMutex autoMutex(sttMutex1);

	// Save the access token for subsequent use within this operator.
	const SPL::rstring& at = (inputTuple.*GETTER)();
	accessToken = at;
	SPLAPPTRC(L_INFO, traceIntro << "-->Received new/refreshed access token.", "process");
	// This must be the audio data arriving here via port 0 i.e. first input port.
	// If we have a non-empty IAM access token, process the audio data.
	// Otherwise, skip it.
	if (accessToken.empty()) {
		SPLAPPTRC(L_ERROR, traceIntro <<
			"-->Ignoring the received audio data at this time due "
			"to an empty IAM access token. User must first provide "
			" the IAM access token before sending any audio data to this operator.",
			"process");
	}
}

//Definition of the template function getSpeechSamples if data type is SPL::blob
template<>
int getSpeechSamples<SPL::blob>(
		SPL::blob & input,
		std::string const & traceIntro,
		std::vector<unsigned char *>& audioBytes,
		std::vector<uint64_t>& audioSize,
		int& numberOfAudioBlobFragmentsReceivedInCurrentConversation,
		SPL::int64& numberOfFullAudioConversationsReceived,
		std::string audioErrorFileName)
{
	uint64_t sizeOfBlob = SPL::Functions::Collections::blobSize(input);

	// We can allow an end of audio indicator (i.e. an empty blob) only
	// if we are already in the process of receiving one or more
	// blob fragments for an ongoing speech conversation.
	// If an empty blob arrives from the caller of this operator
	// (i.e. SPL code) abruptly when there is no ongoing speech conversation,
	// ignore that one without adding it to the vector.
	if (sizeOfBlob == 0 &&
		numberOfAudioBlobFragmentsReceivedInCurrentConversation == 0) {
		// This is not allowed. Ignore this empty blob.
		SPLAPPTRC(L_ERROR, traceIntro <<
			"-->Received audio data is an empty blob in an empty conversation. This tuple is ignored!",
			"process");
		return 1;
	}

	if (sizeOfBlob > 0) {
		audioBytes.push_back(input.releaseData(sizeOfBlob));
		numberOfAudioBlobFragmentsReceivedInCurrentConversation++;
	} else {
		// Append a NULL pointer to indicate the end of blob data for a given audio.
		audioBytes.push_back(nullptr);
		// All blob fragments for the current audio conversation have been received.
		numberOfAudioBlobFragmentsReceivedInCurrentConversation = 0;
		numberOfFullAudioConversationsReceived++;
	}
	audioSize.push_back(sizeOfBlob);
	return 0;
}

//Definition of the template function getSpeechSamples if data type is rstring -> read file
template<>
int getSpeechSamples<SPL::rstring>(
		SPL::rstring & input,
		std::string const & traceIntro,
		std::vector<unsigned char *>& audioBytes,
		std::vector<uint64_t>& audioSize,
		int& numberOfAudioBlobFragmentsReceivedInCurrentConversation,
		SPL::int64& numberOfFullAudioConversationsReceived,
		std::string audioErrorFileName)
{
	SPLAPPTRC(L_DEBUG, traceIntro << "-->Sending file " << input, "ws_audio_blob_sender");

	numberOfFullAudioConversationsReceived++;

	// Check for the file existence before attempting to read the audio data.
	// In C++, this is a very fast way to check file existence.
	// https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
	struct stat fileStat;
	int32_t fileStatReturnCode = stat(input.c_str(), &fileStat);
	// int32_t fileSize = fileStat.st_size;

	if (fileStatReturnCode != 0) {
		// File doesn't exist.
		// Log this information and remove this audio data from the vector.
		std::string errorMsg = traceIntro +
			"-->Audio file not found. Skipping STT task for this file: " + input;
		SPLAPPTRC(L_ERROR, errorMsg, "ws_audio_blob_sender");


		return 2;
	} else {
		// Audio file exists. Read binary file into buffer
		std::ifstream inputStream(input.c_str(), std::ios::binary);
		//std::vector<char> buffer((std::istreambuf_iterator<char>(inputStream)), (std::istreambuf_iterator<char>()));
		uint64_t fsize = fileStat.st_size;
		SPLAPPTRC(L_DEBUG, traceIntro << "-->Sending file size" << fsize, "ws_audio_blob_sender");

		unsigned char * const data = new unsigned char[fsize];
		unsigned char * datax = data;
		std::istreambuf_iterator<char> it(inputStream);
		std::istreambuf_iterator<char> it_end;
		while (not it.equal(it_end)) {
			*datax = *it;
			++datax;
			++it;
		}

		audioBytes.push_back(data);
		audioSize.push_back(fsize);
		// Push a null blob to trigger the 'end of conversation' procedure
		audioBytes.push_back(nullptr);
		audioSize.push_back(0u);

		numberOfAudioBlobFragmentsReceivedInCurrentConversation = 1;
		return 0;

	} // End of if (stat(audioFileName.c_str(), &buffer) != 0)
}

template<typename OP, typename OT>
template<typename IT0, typename DATA_TYPE, DATA_TYPE & (IT0::*GETTER)()>
void WatsonSTTImpl<OP, OT>::process_0(IT0 & inputTuple) {
	// There are multiple methods (process and on_message) that
	// regularly access (read, write and delete) the vector member variables.
	// All those methods work in parallel inside their own threads.
	// To make that vector access thread safe, we will use this mutex.
	SPL::AutoMutex autoMutex(sttMutex1);

	//get input
	DATA_TYPE & data = (inputTuple.*GETTER)();

	std::string audioFileName;
	int result = getSpeechSamples(data, traceIntro, audioBytes, audioSize, numberOfAudioBlobFragmentsReceivedInCurrentConversation, numberOfFullAudioConversationsReceived, audioFileName);

	if (result == 1) {
		SPLAPPTRC(L_ERROR, traceIntro <<
			"-->Received audio data is an empty blob in an empty conversation. This tuple is ignored!",
			"process");
		return;

	} else if ( result == 2) {
		std::string errorMsg = traceIntro +
			"-->Audio file not found. Skipping STT task for this file: " + audioFileName;
		SPLAPPTRC(L_ERROR, errorMsg, "ws_audio_blob_sender");

		// Create an output tuple, auto assign from current input tuple
		OT * oTuple = splOperator.createOutTupleAndAutoAssign(inputTuple);
		// Assign error message and send the tuple
		splOperator.setErrorAttribute(oTuple, errorMsg);
		splOperator.submit(*oTuple, 0);

		// Update the operator metric only if the user asked for a live update.
		if (sttLiveMetricsUpdateNeeded == true) {
			nFullAudioConversationsReceivedMetric->setValueNoLock(numberOfFullAudioConversationsReceived);
		}

		return;
	}

	//Successful operation
	// Update the operator metric only if the user asked for a live update.
	if (sttLiveMetricsUpdateNeeded == true) {
		nFullAudioConversationsReceivedMetric->setValueNoLock(numberOfFullAudioConversationsReceived);
	}

	// Let us do all the auto output tuple attribute assignments and store it in a
	// list to be used in the on_message event handler method below at the time of
	// either full or partial transcription gets completed for a given audio data.
	// We will create a dynamic oTuple object via the new C++ construct so that
	// the object pointer can be stored in the oTupleList below.
	// For every new audio (either a new audio filename or the very first
	// audio blob fragment sent here, we will create a new oTuple object.
	const bool newAudioSegment = numberOfAudioBlobFragmentsReceivedInCurrentConversation == 1;
	if (newAudioSegment) {
		OT * oTuple = splOperator.createOutTupleAndAutoAssign(inputTuple);
		// Push this partially filled oTuple object's pointer to the
		// vector so that we can get it back when the transcription result arrives.
		oTupleList.push_back(oTuple);
	}
} // End: WatsonSTTImpl<OP, OT>::process_0

// This method provides a thread for the Websocket audio blob sender
// as well as the Websocket connection termination monitor.
template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::ws_audio_blob_sender() {

	while (not splOperator.getPE().getShutdownRequested()) {
		// Keep waiting in this while loop until
		// there is some work that needs to be performed.
		// Wait for a configured amount of time that is not 0.0 when there is
		// audio data actively available for processing.
		// When there is no audio data available, yield the CPU for
		// slightly a longer time.
		bool bytesAreToSend_;
		{
			//block other threads from accessing the vectors
			SPL::AutoMutex autoMutex(sttMutex1);

			bytesAreToSend_ = audioBytes.size() > 0;
			//unblock
		}
		if(not bytesAreToSend_) {
			// There is no audio data available at this time.
			// Yield the CPU for slightly a longer time.
			// 1 second instead of 1 msec.
			SPLAPPTRC(L_TRACE, traceIntro <<
					"-->Nothing to send in thread ws_audio_blob_sender, block for 1 second",
					"ws_audio_blob_sender");
			SPL::Functions::Utility::block(1.0);
		} else if (cpuYieldTimeInAudioSenderThread > 0.0) {
			// Audio data available for processing. Yield the CPU briefly and get to work soon.
			// Even a tiny value of 1 millisecond (0.001 second) will yield the
			// CPU and will not show 0% idle in the Linux top command.
			SPLAPPTRC(L_TRACE, traceIntro << "-->Going to send data in thread ws_audio_blob_sender, "
					"block for cpuYieldTimeInAudioSenderThread=" << cpuYieldTimeInAudioSenderThread,
					"ws_audio_blob_sender");
			SPL::Functions::Utility::block(cpuYieldTimeInAudioSenderThread);
		}
		//SPL::Functions::Utility::block interrupts if shutdown is requested
		//do not sent any further data if shutdown is requested and exit from loop
		if (splOperator.getPE().getShutdownRequested())
			continue;

		// Check if the Websocket connection needs to be established.
		if (bytesAreToSend_ && not wsConnectionEstablished) {
			// When there is audio data waiting to be processed and
			// if the Websocket connection is not active at that time,
			// it could be due to one of these reasons.
			//
			// 1) Very first connection to STT service has not yet been
			//	  made since the time this operator came alive.
			// 2) Due to inactivity or session timeout, STT service may have terminated the connection.
			// 3) Invalid audio data was sent to STT earlier and STT rejected that
			//    invalid data and terminated the Websocket connection.
			// 4) Other network or remote STT service system error may have
			//    caused a connection termination.
			//
			// In that case, we will try to reestablish the connection.
			// There is audio data waiting in the vector. Reestablish the connection.

			//update numberOfWebsocketConnectionAttempts
			++numberOfWebsocketConnectionAttempts;
			//nWebsocketConnectionAttemptsMetric is set only from this thread, thus we use setValueNoLock
			nWebsocketConnectionAttemptsMetric->setValueNoLock(numberOfWebsocketConnectionAttempts);
			std::string msg = traceIntro +
				"-->Audio data is waiting to be processed. "
				"Establishing the Websocket connection to the Watson STT service now. "
				"This is attempt: ";
			if (numberOfWebsocketConnectionAttempts >= connectionAttemptsThreshold)
				SPLAPPTRC(L_ERROR, msg << numberOfWebsocketConnectionAttempts, "ws_connection_attempt");
			else if (numberOfWebsocketConnectionAttempts > 1)
				SPLAPPTRC(L_WARN,  msg << numberOfWebsocketConnectionAttempts, "ws_connection_attempt");
			else
				SPLAPPTRC(L_INFO,  msg << numberOfWebsocketConnectionAttempts, "ws_connection_attempt");
			websocketConnectionErrorOccurred = false;
			makeNewWebsocketConnection = true;

			// After a successful connection, makeNewWebsocketConnection will be
			// set to false in the ws_init method below.
			// Successful connection negotiation will invoke the on_open
			// event handler below which will exchange the start session message
			// with the STT service. On a successful session establishment,
			// STT will send a response message which will be received in the
			// on_message event handler below which will set the
			// wsConnectionEstablished to true.
			// Wait until the Websocket connection is fully made as explained above.
			while(not wsConnectionEstablished) {
				if (not websocketConnectionErrorOccurred) {
					SPLAPPTRC(L_DEBUG, traceIntro <<
						"-->Reached 9 Still no connection established and no connection error occurred; wait 0.500",
						"reestablish_ws_connection");
					SPL::Functions::Utility::block(0.500);
				} else {
					// connection error occured
					if (numberOfWebsocketConnectionAttempts >= connectionAttemptsThreshold) {
						// Slow down the frequency of connection attempts after unsucessfull connectionAttemptsThreshold
						// We keep on trying to connect to stt service
						SPL::Functions::Utility::block(waitTimeBeforeSTTServiceConnectionRetryLong);
					} else {
						// Back off for a few seconds and then try to connect again.
						SPL::Functions::Utility::block(waitTimeBeforeSTTServiceConnectionRetry);
					} // End of if (numberOfWebsocketConnectionAttempts > connectionAttemptsThreshold)

					// Break from this inner loop and then continue the
					// outer loop to retry the connection attempt.
					break;
				} // End of if (websocketConnectionErrorOccurred == true)

			} // End of the while loop.

			// Continue from the top of the outer loop.
			continue;
		} // End of the if (bytesAreToSend_ && wsConnectionEstablished == false)

		if (bytesAreToSend_ && wsConnectionEstablished == true) {
			//reset numberOfWebsocketConnectionAttempts
			numberOfWebsocketConnectionAttempts = 0;
			//nWebsocketConnectionAttemptsMetric is set only from this thread, thus we use setValueNoLock
			nWebsocketConnectionAttemptsMetric->setValueNoLock(numberOfWebsocketConnectionAttempts);
		}

		// It is a special delay to give enough time for the
		// Websocket thread's on_close event handler method below to
		// complete its cleanup during a connection termination
		// happening as a result of an abnormal transcription error.
		// Read more commentary about it in the next if segment.
		if (transcriptionErrorOccurred == true) {
			SPLAPPTRC(L_WARN, traceIntro <<
				" -->WatsonSTT_cpp.cgt: transcriptionErrorOccurred 1 (no connection); wait 0.600",
				"ws_connection_attempt_failure");
			// Wait for 600 milliseconds.
			SPL::Functions::Utility::block(0.600);
			transcriptionErrorOccurred = false;
			// Continue at the top of the while loop to reestablish the WS connection.
			continue;
		}

		// Check if there is audio data waiting to be sent to the STT service.
		if (bytesAreToSend_ &&
			wsConnectionEstablished == true &&
			statusOfAudioDataTransmissionToSTT != FULL_AUDIO_DATA_SENT_TO_STT) {
			// If an error happened in the STT processing (invalid audio data or
			// other system error), it most likely will force the Watson STT
			// service to terminate the Websocket connection. In that case, Websocket
			// thread may be currently inside its on_close event handler method
			// below in the middle of doing a cleanup. If this sender thread
			// accidentally enters into this if segment of code due to inter-thread
			// racing conditions, we shouldn't proceed further with sending
			// audio data in order to avoid the Websocket "invalid state" exception.
			// We will simply continue our outer while loop thereby to force a
			// short delay and then reestablish the Websocket connection.
			// This is simply defensive coding to avoid that "invalid state" exception.
			if (transcriptionErrorOccurred == true) {
				SPLAPPTRC(L_WARN, traceIntro <<
					" -->WatsonSTT_cpp.cgt: transcriptionErrorOccurred 2 (connection); continue",
					"ws_connection_attempt_failure");
				continue;
			}

			// There are multiple methods (process, audio_blob_sender and on_message) that
			// regularly access (read, write and delete) the vector member variables.
			// All those methods work in parallel inside their own threads.
			// To make that vector access thread safe, we will use this mutex.
			SPL::AutoMutex autoMutex(sttMutex1);

			//
			// We can keep streaming the blob based speech data to the STT service.
			// So the signal for the end of the audio data will be sent to the
			// STT service with a special logic in the case of blob based speech data.
			// Here is how it works.
			// SPL code where this operator is invoked must send an emply blob
			// immediately following the very last fragment of the speech blob data.
			// When an empty blob arrives, we will insert a null pointer into the vector.
			// When we encounter that NULL pointer in the audio blob sender method,
			// we will inform the STT service by signaling the end of the audio data.
			// It is very important for the SPL code to send ONLY ONE empty blob at the
			// end of transmitting all the non-empty blob content belonging to a
			// given audio either in full or in partial fragments.
			//
			unsigned char * buffer = audioBytes.at(0);
			uint64_t sizeOfBlob = audioSize.at(0);

			SPLAPPTRC(L_DEBUG, traceIntro << "-->Sending blob of size " << sizeOfBlob , "ws_audio_blob_sender");
			if (buffer == NULL) {
				// We reached the end of the blob data as sent/streamed from the SPL application.
				// Signal end of the audio data.
				// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-websockets#WSstop
				wsClient->send(wsHandle, "{\"action\" : \"stop\"}" , websocketpp::frame::opcode::text);
				// In a blob based audio data, the entire blob has been sent to the STT service at this time.
				// So set this flag to indicate that.
				statusOfAudioDataTransmissionToSTT = FULL_AUDIO_DATA_SENT_TO_STT;
			} else {
				// Speech data is sent into this operator via a blob buffer rather than via a file.
				// Send the blob data (either in full or in a partial fragment) to the STT service.
				// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-websockets#WSaudio
				// c->get_alog().write(websocketpp::log::alevel::app, "Sent binary Message: " + boost::to_string(buffer.size()));
				wsClient->send(wsHandle, buffer, sizeOfBlob, websocketpp::frame::opcode::binary);
				statusOfAudioDataTransmissionToSTT = AUDIO_BLOB_FRAGMENTS_BEING_SENT_TO_STT;
			} // End of if (buffer == NULL)

			// Remove the items from the vector. It is no longer needed. Also free the original
			// data pointer that we obtained from the blob in the process method.
			if (buffer != NULL) {
				delete[] buffer;
			}

			audioBytes.erase(audioBytes.begin() + 0);
			audioSize.erase(audioSize.begin() + 0);
			// Continue from the top of the while loop.
			continue;
		} // End of if ((audioBytesVectorSize > 0 ||
	} // End of the while loop
	SPLAPPTRC(L_INFO, traceIntro <<
			"-->End of loop ws_audio_blob_sender: getShutdownRequested()=" << splOperator.getPE().getShutdownRequested(),
			"ws_audio_blob_sender");
} // End: WatsonSTTImpl<OP, OT>::ws_audio_blob_sender

// This method initializes the Websocket driver, TLS and then
// opens a connection. This is going to run on its own thread.
// See the commentary in the allPortsReady method above to
// understand our need to run it in a separate thread.
template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::ws_init() {

	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;
	using websocketpp::lib::bind;

	while (not splOperator.getPE().getShutdownRequested()) {
		if (not makeNewWebsocketConnection) {
			// Keep waiting in this while loop until
			// a need arises to make a new Websocket connection.
			// 1 second wait.
			SPLAPPTRC(L_TRACE, traceIntro << "-->No connection request in thread ws_init, block for 1 second", "ws_init");
			SPL::Functions::Utility::block(1.0);
			continue;
		}

		// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-websockets#WSopen
		std::string uri = this->uri;
		// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-input#models
		uri += "?model=" + baseLanguageModel;
		https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-input#logging
		uri += "&x-watson-learning-opt-out=" + std::string(sttRequestLogging ? "false" : "true");

		// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-input#version
		if (baseModelVersion != "") {
			uri += "&base_model_version=" + baseModelVersion;
		}

		// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-input#custom
		// At a time, only one LM customization can be specified.
		// LM custom model chaining is not available as of Aug/2018.
		if (customizationId != "") {
			uri += "&customization_id=" + customizationId;
		}

		if (acousticCustomizationId != "") {
			uri += "&acoustic_customization_id=" + acousticCustomizationId;
		}

		uri += "&access_token=" + accessToken;
		wsConnectionEstablished = false;

		if (wsClient) {
			// If we are going to do a reconnection, then free the
			// previously created Websocket client object.
			SPLAPPTRC(L_DEBUG, traceIntro << "-->Delete client " << uri, "ws_init");
			delete wsClient;
			wsClient = nullptr;
		}

		try {
			SPLAPPTRC(L_INFO, traceIntro <<
				"-->Going to connect to " << uri,
				"ws_init");
			wsClient = new client();
			if ( ! wsClient)
				throw std::bad_alloc();

			// https://docs.websocketpp.org/reference_8logging.html
			// Set the logging policy as needed
			// Turn off or turn on selectively all the Websocket++ access interface and
			// error interface logging channels. Do this based on how the user has
			// configured this operator.
			if (websocketLoggingNeeded == true) {
				// Enable certain error logging channels and certain access logging channels.
				wsClient->set_access_channels(websocketpp::log::alevel::frame_header);
				wsClient->set_access_channels(websocketpp::log::alevel::frame_payload);
			} else {
				// Turn off both the access and error logging channels completely.
				wsClient->clear_access_channels(websocketpp::log::alevel::all);
				wsClient->clear_error_channels(websocketpp::log::elevel::all);
			}

			// Initialize ASIO
			wsClient->init_asio();

			// IBM Watson STT service requires SSL based communication.
			// Set this TLS handler.
			// This technique to pass a class member method as a callback function is from here:
			// https://stackoverflow.com/questions/34757245/websocketpp-callback-class-method-via-function-pointer
			wsClient->set_tls_init_handler(bind(&WatsonSTTImpl<OP, OT>::on_tls_init,this,wsClient,::_1));

			// Register our other event handlers.
			wsClient->set_open_handler(bind(&WatsonSTTImpl<OP, OT>::on_open,this,wsClient,::_1));
			wsClient->set_fail_handler(bind(&WatsonSTTImpl<OP, OT>::on_fail,this,wsClient,::_1));
			wsClient->set_message_handler(bind(&WatsonSTTImpl<OP, OT>::on_message,this,wsClient,::_1,::_2));
			wsClient->set_close_handler(bind(&WatsonSTTImpl<OP, OT>::on_close,this,wsClient,::_1));

			// Create a connection to the given URI and queue it for connection once
			// the event loop starts
			SPLAPPTRC(L_DEBUG, traceIntro << "-->Reached 1 (call back setup)", "ws_init");
			websocketpp::lib::error_code ec;
			// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-basic-request#using-the-websocket-interface
			client::connection_ptr con = wsClient->get_connection(uri, ec);
			SPLAPPTRC(L_DEBUG, traceIntro << "-->Reached 2 (get_connection) ec.value=" << ec.value(), "ws_init");
			if (ec)
				throw ec;

			wsClient->connect(con);
			// A new Websocket connection has just been made. Reset this flag.
			makeNewWebsocketConnection = false;
			SPLAPPTRC(L_DEBUG, traceIntro << "-->Reached 3 (connect)", "ws_init");

			// Start the ASIO io_service run loop
			wsClient->run();
			SPLAPPTRC(L_INFO, traceIntro << "-->Reached 4 (run)", "ws_init");
		} catch (const std::exception & e) {
			SPLAPPTRC(L_ERROR, traceIntro << "-->std::exception: " << e.what(), "ws_init");
			SPL::Functions::Utility::abort(__FILE__, __LINE__);
		} catch (const websocketpp::lib::error_code & e) {
			//websocketpp::lib::error_code is a class -> catching by reference makes sense
			SPLAPPTRC(L_ERROR, traceIntro << "-->websocketpp::lib::error_code: " << e.message(), "ws_init");
			SPL::Functions::Utility::abort(__FILE__, __LINE__);
		} catch (...) {
			SPLAPPTRC(L_ERROR, traceIntro << "-->Other exception in WatsonSTT operator's Websocket initializtion.", "ws_init");
			SPL::Functions::Utility::abort(__FILE__, __LINE__);
		}
	} // End of while loop.
	SPLAPPTRC(L_INFO, traceIntro <<
			"-->End of loop ws_init: getShutdownRequested()=" << splOperator.getPE().getShutdownRequested(),
			"ws_init");
} // End: WatsonSTTImpl<OP, OT>::ws_init

// When the Websocket connection to the Watson STT service is made successfully,
// this callback method will be called from the websocketpp layer.
template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::on_open(client* c, websocketpp::connection_hdl hdl) {
	SPLAPPTRC(L_DEBUG, traceIntro << "-->Reached 6 (on_open)", "on_open");
	// On Websocket connection open, establish a session with the STT service.
	// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-websockets#WSstart
	// We have to form a proper JSON message structure for the
	// STT recognition request start message with all the output features we want.
	// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#output
	std::string msg = "{\"action\" : \"start\"";
	msg += ", \"content-type\" : \"" + contentType + "\"";

	std::string interimResultsNeeded = "false";

	if (sttResultMode == 1 || sttResultMode == 2) {
		// User configured it for either partial utterance or completed utterance.
		interimResultsNeeded = "true";
	}

	// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#interim
	msg += ", \"interim_results\" : " + interimResultsNeeded;

	// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-input#timeouts
	msg += ", \"inactivity_timeout\": -1";

	// Customization weight of 9.9 indicates that the user never configured this parameter in SPL.
	// In that case, we can ignore sending it to the STT service.
	// If it is not 9.9, then we must send it to the STT service.
	if (customizationWeight != 9.9) {
		std::ostringstream strs;
		strs << customizationWeight;

		// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-input#custom
		msg += std::string(", \"customization_weight\" : ") + strs.str();
	}

	std::string profanityFilteringNeeded = "true";

	if (filterProfanity == false) {
		profanityFilteringNeeded = "false";
	}

	// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#profanity_filter
	msg += ", \"profanity_filter\" : " + profanityFilteringNeeded;

	// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#max_alternatives
	msg += ", \"max_alternatives\" : " + boost::to_string(maxUtteranceAlternatives);

	// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#word_alternatives
	if(wordAlternativesThreshold > 0.0) {
		msg += ", \"word_alternatives_threshold\" : " + boost::to_string(wordAlternativesThreshold);
	}

	// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#word_confidence
	if (wordConfidenceNeeded == true) {
		msg += ", \"word_confidence\" : true";
	}

	// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#word_timestamps
	if (wordTimestampNeeded == true) {
		msg += ", \"timestamps\" : true";
	}

	// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#speaker_labels
	if (identifySpeakers == true) {
		msg += ", \"speaker_labels\" : true";
	}

	// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#smart_formatting
	if (smartFormattingNeeded == true) {
		msg += ", \"smart_formatting\" : true";
	}

	// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#keyword_spotting
	if (keywordsSpottingThreshold > 0.0) {
		msg += ", \"keywords_threshold\" : " + boost::to_string(keywordsSpottingThreshold);
		// We have to send all the keywords to be spotted as a JSON list.
		msg += ", \"keywords\" : [";

		// Iterate through the list of keywords and add them to the request message structure.
		for (SPL::int32 idx = 0;
			idx < keywordsToBeSpotted.size(); idx++) {
			// Add a comma after every keyword string.
			if (idx > 0) {
				msg += ", ";
			}

			// Add the keyword now.
			msg += std::string("\"") +
				keywordsToBeSpotted.at(idx) +
				std::string("\"");
		}

		msg += "]";
	}

	msg += std::string("}");

	if (sttJsonResponseDebugging == true) {
		std::cout << traceIntro << "-->X53 Websocket STT recognition request start message=" << msg << std::endl;
	}

	c->send(hdl,msg,websocketpp::frame::opcode::text);
	// Store this handle to be used from process and shutdown methods of this operator.
	wsHandle = hdl;
	// c->get_alog().write(websocketpp::log::alevel::app, "Sent Message: "+msg);
	SPLAPPTRC(L_INFO, traceIntro <<
		"-->A recognition request start message was sent to the Watson STT service: " << msg,
		"on_open");
} //End: WatsonSTTImpl<OP, OT>::on_open

// This recursive templatized function with c++11 syntax is from the
// C++ boost Q&A (how-to) technical discussion here:
// https://stackoverflow.com/questions/48407925/boostproperty-treeptree-accessing-arrays-first-complex-element?noredirect=1&lq=1
// It helps us to directly index an element in a JSON array returned by the Watson STT service.
// To use c++11 syntax in a Streams C++ operator, it is required to add this
// sc (Streams Compiler) option: --c++std=c++11
// function requires no object reference -> make it satic
template <typename Tree>
static Tree query(Tree& pt, typename Tree::path_type path) {
	if (path.empty())
		return pt;

	auto const head = path.reduce();

	auto subscript = head.find('[');
	auto name      = head.substr(0, subscript);
	auto index     = std::string::npos != subscript && head.back() == ']'
		? std::stoul(head.substr(subscript+1))
		: 0u;

	auto matches = pt.equal_range(name);
	if (matches.first==matches.second)
		throw std::out_of_range("name:" + name);

	for (; matches.first != matches.second && index; --index)
		++matches.first;

	if (index || matches.first==matches.second)
		throw std::out_of_range("index:" + head);

	return query(matches.first->second, path);
}

// Whenever a message (transcription result, STT service message or an STT error message) is
// received from the STT service, this callback method will be called from the websocketpp layer.
// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-websockets#WSexample
template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {

	// Short alias for this namespace
	namespace pt = boost::property_tree;

	// c->get_alog().write(websocketpp::log::alevel::app, "Received Reply: "+msg->get_payload());
	//
	// This debugging variable is enabled/disabled via an operator parameter.
	// When enabled, it will simply display the full JSON message and
	// a few important fields resulting from parsing that JSON message.
	// This debugging mode is useful during
	// toolkit development to verify any new fields in the STT result JSON.
	// During the development of this toolkit, this debug variable helped
	// immensely to test the JSON parsing as well as to prepare the data to be
	// sent to the caller. Hence, I decided to leave this code here for future
	// needs. It will simply cost a few extra bytes in the operator's compiled
	// binary code and it will not cause a major performance impact during
	// normal operation when disabled.
	// In general, you can enable this debug mode and send one file at a time to the STT
	// service and carefully observe all the messages that get returned back in order to
	// develop and fine-tune the JSON message parsing logic.
	bool fullTranscriptionCompleted_ = false;
	bool transcriptionResultAvailableForParsing_ = false;

	const std::string & payload_ = msg->get_payload();
	const bool stateListeningFound_ = payload_.find("\"state\": \"listening\"") != std::string::npos;

	SPLAPPTRC(L_DEBUG, traceIntro << "-->Reached 7 (on_message)", "on_message");

	// STT error will have the following message format.
	// {"error": "unable to transcode data stream audio/wav -> audio/x-float-array "}
	const bool sttErrorFound_ = payload_.find("\"error\": \"") != std::string::npos;
	std::string sttErrorString_ = "";
	if (sttErrorFound_) {
		sttErrorString_ = payload_;

		if (sttJsonResponseDebugging == true) {
			std::cout << traceIntro << "-->X3 STT error message=" << sttErrorString_ << std::endl;
		}
		SPLAPPTRC(L_ERROR, traceIntro << "-->X3 STT error message=" << sttErrorString_, "on_message");

	} else if (stateListeningFound_ && not wsConnectionEstablished) {
		// This is the "listening" response from the STT service for the
		// new recognition request that was initiated in the on_open method above.
		// This response is sent only once for every new Websocket connection request made and
		// not for every new transcription request made.
		wsConnectionEstablished = true;
		websocketConnectionErrorOccurred = false;
		numberOfWebsocketConnectionAttempts = 0;

		if (sttJsonResponseDebugging == true) {
			std::cout << traceIntro << "-->X0 STT response payload=" << payload_ << std::endl;
		}

		SPLAPPTRC(L_DEBUG, traceIntro <<
			"-->Reached 8 Websocket connection established with the Watson STT service.",
			"on_message");
		return;
	} else if (not stateListeningFound_ && wsConnectionEstablished) {
		// This must be the response for our audio transcription request.
		// Keep accumulating the transcription result.
		transcriptionResult += payload_;

		if (sttJsonResponseDebugging == true) {
			std::cout << traceIntro << "-->X1 STT response payload=" << payload_ << std::endl;
		}

		SPLAPPTRC(L_DEBUG, traceIntro <<
			"-->Reached 10 Websocket connection established - no listening state.",
			"on_message");

		if (sttResultMode == 1 || sttResultMode == 2) {
			// We can parse this partial or completed
			// utterance result now since the user has asked for it.
			transcriptionResultAvailableForParsing_ = true;
		} else {
			// User didn't ask for the partial or completed utterance result.
			// We will parse the full transcription result later when it is fully completed.
			return;
		}
	} else if (stateListeningFound_ && wsConnectionEstablished) {
		// This is the "listening" response from the STT service for the
		// transcription completion for the audio data that was sent earlier.
		// This response also indicates that the STT service is ready to do a new transcription.
		fullTranscriptionCompleted_ = true;

		if (sttJsonResponseDebugging == true) {
			std::cout << traceIntro << "-->X2 STT task completion message=" << payload_ << std::endl;
		}

		SPLAPPTRC(L_DEBUG, traceIntro <<
			"-->Reached 11 Websocket connection established - transcription completion.",
			"on_message");

		if (sttResultMode == 3) {
			// Neither partial nor completed utterance results were parsed earlier.
			// So, parse the full transcription result now.
			transcriptionResultAvailableForParsing_ = true;
		}
	}

	int32_t utteranceNumber_ = 0;
	std::string utteranceText_ = "";
	SPL::list<SPL::rstring> utteranceAlternatives_;
	SPL::list<SPL::list<SPL::rstring>> wordAlternatives_;
	SPL::list<SPL::list<SPL::float64>> wordAlternativesConfidences_;
	SPL::list<SPL::float64> wordAlternativesStartTimes_;
	SPL::list<SPL::float64> wordAlternativesEndTimes_;
	SPL::list<SPL::rstring> utteranceWords_;
	SPL::list<SPL::float64> utteranceWordsConfidences_;
	SPL::list<SPL::float64> utteranceWordsEndTimes_;
	SPL::float64 utteranceStartTime_ = 0.0;
	SPL::float64 utteranceEndTime_ = 0.0;
	SPL::map<SPL::rstring, SPL::list<SPL::map<SPL::rstring, SPL::float64>>> keywordsSpottingResults_;
	SPL::list<SPL::map<SPL::rstring, SPL::float64>> keywordsSpottingResultsList_;
	SPL::map<SPL::rstring, SPL::float64> keywordsSpottingResultsMap_;

	bool final_ = false;
	float confidence_ = 0.0;
	std::string fullTranscriptionText_ = "";
	float cumulativeConfidenceForFullTranscription_ = 0.0;
	int32_t idx1 = -1;
	int32_t idx2 = -1;
	int32_t idx3 = -1;

	// Create a boost property tree root
	pt::ptree root;
	std::stringstream ss;

	// There are multiple methods (process, audio_blob_sender and on_message) that
	// regularly access (read, write and delete) the vector member variables.
	// All those methods work in parallel inside their own threads.
	// To make that vector access thread safe, we will use this mutex.
	SPL::AutoMutex autoMutex(sttMutex1);

	// If there is valid result from the STT service, parse it now.
	// This parsing can be very involved depending on what kind of
	// output features are configured for the STT service in the
	// on_open method above. There is intricate logic here due to
	// the order in which different sections of the response JSON message arrive.
	// So read and test this code often in a methodical fashion.
	if (transcriptionResultAvailableForParsing_ || sttErrorFound_) {
		// Send either the partial utterance or completed utterance or
		// full text result as an output tuple now.
		// If there is an STT error, send that as well.
		// The oTupleList size check here will ensure we will process only
		// the STT errors happening during transcription and not the
		// STT errors happening during idle time in the absence of any audio data.
		// We will also add here a condition not to process the STT errors in
		// this if segment when such errors occur at the time of establishing
		// a Websocket connection to the STT service.
		if (sttErrorFound_ && wsConnectionEstablished && (oTupleList.size() > 0)) {
			// Parse the STT error string.
			ss << sttErrorString_;
			// Reset the trascriptionResult member variable.
			transcriptionResult = "";
			// Due to this STT error, transcription for the given audio data will not continue.
			// This flag will be checked inside the ws_audio_blob_sender (thread) method.
			transcriptionErrorOccurred = true;

			// Load the json data in the boost ptree
			pt::read_json(ss, root);
			const std::string sttErrorMsg_ = root.get<std::string>("error");

			// Set the STT error message attribute via the corresponding output function.
			splOperator.setErrorAttribute(oTupleList.at(0), sttErrorMsg_);

			// This prepared tuple with the assigned STT message will be
			// sent in the very last if segment in this method below.
		} else if(transcriptionResultAvailableForParsing_) {
			// Parse the relevant fileds from the JSON transcription result.
			ss << transcriptionResult;
			// Reset the trascriptionResult member variable.
			transcriptionResult = "";
			// Load the json data in the boost ptree
			pt::read_json(ss, root);

			// Successful STT result JSON will be at a minimum in the format as shown below.
			// {"results": [{"alternatives": [{"transcript": "name "}], "final_": false}], "result_index": 0}
			//

			// If speaker labels is enabled in our current STT session, we have to
			// check if we received any speaker ids in the STT response message.
			// When this feature is enabled along with interim_results (which is always the
			// case for our stt result mode 1 and 2), STT JSON response sometimes will
			// contain just the speaker_labels field without the results array and result_index fields.
			// Read the Watson STT service speaker labels documentation about it.
			// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#speaker_labels
			// So, we will always read the speaker_labels field first on its own and
			// store it in the member variables of this operator class instead of
			// local variables as it is done below for all the other JSON response fields.
			//
			//
			// A MUST READ IMPORTANT FINDING FROM THE LOCAL LAB TESTS:
			// The way this operator is coded with interim_results always set to true for
			// the STT result mode 1 and 2, speaker_labels will always come as a standalone
			// JSON message right after the finalized utterance result i.e.
			// "results.final" field set to true.
			// In some rare cases, there will be two consecutive speaker_labels messages one after the
			// other with the first one carrying the full set of speaker_labels and the
			// second one carrying the label for the very last word with its final field set to true.
			// In the logic below, we are going to ignore this second speaker_labels message.
			bool secondSpeakerLabelsMessageReceived_ = false;
			bool firstSpeakerLabelTimeMatched_ = false;

			while(identifySpeakers == true) {
				++idx1;
				SPL::int32 speakerId = -999;
				SPL::float64 speakerIdConfidence = 0.0;
				SPL::float64 fromTime = 0.0;

				// In the lab tests, I noticed that the STT service sometimes returns
				// extra speaker labels from the earlier utterances. We have to ignore them.
				// So let us skip the extra ones until we match with the first entry in the
				// timestamps list which got populated in the previous finalized
				// utterance JSON message that came just before the speaker_labels message.
				if (not firstSpeakerLabelTimeMatched_) {
					// Read the from field.
					try {
						fromTime = query(root,
							"speaker_labels.[" + boost::to_string(idx1) + "].from").get_value<float>();

						if (sttJsonResponseDebugging == true) {
							std::cout << traceIntro << "-->X4 fromTime=" << fromTime << std::endl;
						}

					} catch (std::exception const& e) {
						if (sttJsonResponseDebugging == true) {
							std::cout << traceIntro << "-->X5 ERROR JSON parsing error when reading the from field : " << e.what() << std::endl;
						}

						break;
					}

					// If the second speaker_labels message arrives, then we can
					// ignore it as explained in the commentary above. Because,
					// there will be no new speaker_labels data here except for the final field
					// set to true for the very last word of the utterance.
					if (idx1 == 0 && utteranceWordsSpeakers.size() > 0) {
						// We already have populated the speaker id list during the first
						// speaker_labels message that came just before this one.
						// This is clearly the second speaker_lables message.
						secondSpeakerLabelsMessageReceived_ = true;

						if (sttJsonResponseDebugging == true) {
							std::cout << traceIntro << "-->X6 Received the second speaker_labels message." << std::endl;
						}

						break;
					}

					// Ensure that we start processing only from the startTime for the
					// very first word in the current utterance. If it has speaker label
					// entries for words from previous utterances, skip them and
					// move to the next one until we find a match with the startTime of
					// the first word belonging to the most recent finalized utterance.
					if (fromTime == utteranceWordsStartTimes.at(0)) {
						firstSpeakerLabelTimeMatched_ = true;

						if (sttJsonResponseDebugging == true) {
							std::cout << traceIntro << "-->X7 Speaker labels 'from' time matched at idx1=" << idx1 << std::endl;
						}
					} else {
						if (sttJsonResponseDebugging == true) {
							std::cout << traceIntro << "-->X8 Speaker labels 'from' time mismatch at idx1=" << idx1 << std::endl;
						}

						// Skip this one and continue the loop.
						continue;
					}
				} // End of if (firstSpeakerLabelTimeMatched_ == false)

				// Read the speaker field.
				try {
					speakerId = query(root,
						"speaker_labels.[" + boost::to_string(idx1) + "].speaker").get_value<int32_t>();
				} catch (std::exception const& e) {
					if (sttJsonResponseDebugging == true) {
						std::cout << traceIntro << "-->X9 ERROR JSON parsing error when reading the speaker field : " << e.what() << std::endl;
					}

					break;
				}

				// Read the confidence field.
				try {
					speakerIdConfidence = query(root,
						"speaker_labels.[" + boost::to_string(idx1) + "].confidence").get_value<float>();
				} catch (std::exception const& e) {
					if (sttJsonResponseDebugging == true) {
						std::cout << traceIntro << "-->X10 ERROR JSON parsing error when reading the confidence field : " << e.what() << std::endl;
					}

					break;
				}

				// Append them to the list.
				utteranceWordsSpeakers.push_back(speakerId);
				utteranceWordsSpeakersConfidences.push_back(speakerIdConfidence);

				if (sttJsonResponseDebugging == true) {
					std::cout << traceIntro << "-->X11 speaker_labels entry added to the list" << std::endl;
				}
			} // End of while loop to iterate through the speaker_labels array.

			// If we have successfully parsed the previous STT JSON response and
			// prepared an output tuple during the previous Websocket on_message event,
			// it is time now to send that STT result tuple that is waiting to be sent.
			// Do it only if the transcription is still in progress. If STT service
			// sent us a transcription completed signal via "listening" response message,
			// then we will skip sending this tuple right here. Instead, we will send the
			// final tuple for this audio in the very last if segment in this method after
			// setting the output tuple attribute (transcriptionCompleted) to true.
			// If we just now received the second speaker_labels message in a row,
			// that is redundant and we will not send the output tuple at that time.
			if (sttResultTupleWaitingToBeSent &&
				not fullTranscriptionCompleted_ &&
				not secondSpeakerLabelsMessageReceived_) {
				sttResultTupleWaitingToBeSent = false;
				// Send this tuple now.
				// Dereference the oTuple object from the object pointer and send it.
				splOperator.submit(*(oTupleList.at(0)), 0);

				if (sttJsonResponseDebugging == true) {
					std::cout << traceIntro << "-->X52a At the tuple submission point for reporting interim transcription results." << std::endl;
				}
				// Since we are storing the speaker_labels results in lists that are
				// member variables of this class, let us clear them after we are
				// done processing the most recently received JSON message from the STT service.
				utteranceWordsSpeakers.clear();
				utteranceWordsSpeakersConfidences.clear();
				utteranceWordsStartTimes.clear();
			}

			idx1 = -1;

			try {
				// Read the result_index field.
				utteranceNumber_ = root.get<int32_t>("result_index");
			} catch (std::exception const& e) {
				if (sttJsonResponseDebugging == true) {
					std::cout << traceIntro << "-->X12 ERROR JSON parsing error when reading the result_index field : " << e.what() << std::endl;
				}

				// If speaker_labels is enabled, it is possible not to receive the
				// result_index field and the results array field in certain situations with
				// interim_results enabled for stt result mode 1 or 2.
				// Read the Watson STT service speaker labels documentation about it.
				// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#speaker_labels
				// In this case, we will ignote this exception and continue to the next
				// while loop which will also exit that loop due to missing "results" array field.
			}

			// If there are multiple elements in the "results" JSON array, iterate through all of them.
			// If we encountered the speaker_labels message two in a row, ignore the
			// second one and skip this entire loop.
			while(not secondSpeakerLabelsMessageReceived_) {
				++idx1;;
				bool tempFinal = false;
				bool exitThisLoop = false;

				// Read the finalized utterance field.
				try {
					tempFinal = query(root,
						"results.[" + boost::to_string(idx1) + "].final").get_value<bool>();
				} catch (std::exception const& e) {
					if (sttJsonResponseDebugging == true) {
						std::cout << traceIntro << "-->X13 ERROR JSON parsing error when reading the final field : " << e.what() << std::endl;

						if (idx1 == 0) {
							std::cout << traceIntro << "-->X14 idx1=0, size of speaker_labels list=" << utteranceWordsSpeakers.size() << std::endl;
						}
					}

					// It is either an invalid index or we received the
					// speaker_labels data in the previous while loop as a
					// standalone field without the result_index or
					// "results" array field. We must exit this while loop now.
					// This means we processed all the elements in the "results" JSON array (OR)
					// we couldn't process any element due to missing "results" array field.
					exitThisLoop = true;
				}

				if (idx1 == 0 && utteranceWordsSpeakers.size() > 0) {
					// We just now got a standalone speaker_labels message right after
					// processing a finalized utterance (for stt result mode 1 and 2).
					// We must set the final local variable to true now to meet the
					// if conditional logic in the next code segment in order to
					// prepare the output tuple to be sent out.
					final_ = true;

					if (sttJsonResponseDebugging == true) {
						std::cout << traceIntro << "-->X15 Forcing final = true after receiving the speaker_labels message." << std::endl;
					}

				}

				// At this time, we can prepare the output tuple for the
				// utterance we parsed in the previous iteration of this loop.
				// Skip preparing an output tuple during the very first loop iteration.
				// We are delaying the tuple sending by one iteration to set the
				// transcription completed field correctly at the very end of the
				// STT processing of the current audio data. The very last tuple with
				// the transcriptionCompleted attribute set to true will be sent below
				// in the next section (if segment) of this method below.
				//
				// Auto assignment where needed was already done in the process method above.
				// So, set only those attributes that have an explicit assignment via an output function.
				// Do this only as needed depending on the user configured STT result mode.
				//
				// SPECIAL NOTE: We have to prepare an output tuple right after receiving
				// the speaker_labels message right after a finalized utterance with idx1 = 0.
				if ((idx1 == 0 && final_ == true) ||
					((idx1 > 0) && (sttResultMode == 1 ||
					(sttResultMode == 2 && final_ == true) ||
					(sttResultMode == 3 && exitThisLoop == true)))) {
					// If the sttResultMode is 3, we must return only the full transcription text.
					// Hence, reset the utterance related details.
					if (sttResultMode == 3) {
						// Compute the average confidence for the full transcription text.
						confidence_ =
							cumulativeConfidenceForFullTranscription_ / (idx1);

						utteranceNumber_ = -1;
						utteranceText_ = "";
						final_ = false;
					}

					// Assign the output attributes via the output functions as configured in the SPL code.
					//
					// 1) When the logic enters here right after receiving a speaker_labels message i.e.
					// when idx1 == 0 && final_l == true, we must update ONLY the
					// speaker id related attributes. This will ensure that we will not
					// overwrite the values of the non-speaker id related attributes whose values
					// were properly set before the speaker_labels message arrived.
					//
					// 2) At the other time i.e. when idx1 > 0, we must set all the attributes where
					// some will have non-empty results and some such as the speaker id
					// attributes will have empty results.
					if (idx1 > 0) {
						splOperator.setResultAttributes(
								oTupleList.at(0),
								utteranceNumber_ + 1,
								utteranceText_,
								final_,
								confidence_,
								fullTranscriptionText_,
								utteranceAlternatives_,
								wordAlternatives_,
								wordAlternativesConfidences_,
								wordAlternativesStartTimes_,
								wordAlternativesEndTimes_,
								utteranceWords_,
								utteranceWordsConfidences_,
								utteranceWordsEndTimes_,
								utteranceStartTime_,
								utteranceEndTime_,
								keywordsSpottingResults_
						);
					} else {
						// When idx == 0, only these speaker id related
						// output functions must be called to update those two attributes.
						// All other attributes shouldn't be touched since they were
						// already set to proper values during the STT JSON messages that
						// arrived before the speaker_labels message.
						splOperator.setSpeakerResultAttributes(oTupleList.at(0));
					} // End of if (idx1 > 0)

					// Set this flag so that this tuple can be sent out during the
					// next on_message Websocket event. In order to send the tuple,
					// this flag is checked just outside of the while loop we are in now.
					// We have to follow this approach so that we can correctly
					// send the very last tuple for this audio with its
					// transcriptionCompleted attribute set to true.
					// The following if conditional logic works as described below.
					// 1) identifySpeakers == false will permit all the three STT result modes to
					//    set the tupleWaiting flag to true.
					//    speaker id feature is disabled.
					// 2) final_ == false will take care of the non-finalized partial utterance when
					//    stt result mode is 1. It will set the tupleWaiting flag to true in that case.
					// 3) The other combined condition will take care of the finalized utterance when
					//    result mode is 1 or 2 with speaker id feature enabled.
					if (identifySpeakers == false || final_ == false ||
						(identifySpeakers == true &&
						final_ == true && idx1 == 0)) {
						sttResultTupleWaitingToBeSent = true;

						if (sttJsonResponseDebugging == true) {
							std::cout << traceIntro << "-->X16 Setting the tuple in waiting mode to be sent out." << std::endl;
						}
					}
				}

				if (exitThisLoop) {
					if (sttJsonResponseDebugging == true) {
						std::cout << traceIntro << "-->X17 ERROR Exiting the main loop after reaching array index " << idx1 << "." << std::endl;
					}

					break;
				}

				std::string tempUtteranceText = "";
				utteranceText_ = "";
				final_ = tempFinal;
				confidence_ = 0.0;
				SPL::int32 idx2 = -1;
				bool confidenceFound = false;

				// If the user has configured that maxUtteranceAlternatives parameter in SPL with
				// a value greater than 1, then STT service will return more than one
				// item in the alternatives JSON array. We have to retrieve all of them to be
				// returned to the user.
				while(true) {
					++idx2;
					// Read the utterance word confidences if present.
					// Let us now iterate through the "word_confidence" array.
					// This is an optional field within the "results.alternatives" JSON array.
					// Refer to this URL for the correct JSON format:
					// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#word_confidence
					idx3 = -1;
					bool utteranceWordsPopulated = false;

					while(true) {
						++idx3;
						// Read the results.alternatives.word_confidence[0] field i.e. word.
						try {
							std::string utteranceWord = query(root,
								"results.[" + boost::to_string(idx1) + "].alternatives.[" +
								boost::to_string(idx2) + "].word_confidence.[" +
								boost::to_string(idx3) + "].[0]" ).get_value<std::string>();

							// Append this to the list.
							utteranceWords_.push_back(utteranceWord);
							utteranceWordsPopulated = true;

							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X18 utteranceWord=" << utteranceWord << std::endl;
							}
						} catch(std::exception const& e) {
							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X19 ERROR idx2=" << idx2 << ", idx3=" << idx3 <<
									". JSON parsing error when reading the word_confidence[0] word field: " << e.what() << std::endl;
							}

							break;
						}

						// Read the results.alternatives.word_confidence[1] field i.e. confidence.
						try {
							SPL::float64 utteranceWordConfidence = query(root,
								"results.[" + boost::to_string(idx1) + "].alternatives.[" +
								boost::to_string(idx2) + "].word_confidence.[" +
								boost::to_string(idx3) + "].[1]" ).get_value<float>();

							// Append this to the list.
							utteranceWordsConfidences_.push_back(utteranceWordConfidence);

							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X20 utteranceWordConfidence=" << utteranceWordConfidence << std::endl;
							}
						} catch(std::exception const& e) {
							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X21 ERROR idx2=" << idx2 << ", idx3=" << idx3 <<
									". JSON parsing error when reading the word_confidence[1] confidence field: " <<
									e.what() << std::endl;
							}

							break;
						}
					} // End of while for parsing "results.alternatives.word_confidence" JSON array.

					if (sttJsonResponseDebugging == true) {
						std::cout << traceIntro << "-->X22 utteranceWords=" << boost::to_string(utteranceWords_) << std::endl;
						std::cout << traceIntro << "-->X23 utteranceWordsConfidences=" << boost::to_string(utteranceWordsConfidences_) << std::endl;
					}


					// Read the utterance word timestamps if present.
					// Let us now iterate through the "timestamps" array.
					// This is an optional field within the "results.alternatives" JSON array.
					// Refer to this URL for the correct JSON format:
					// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#word_timestamps
					idx3 = -1;

					while(true) {
						++idx3;
						// Read the results.alternatives.timestamps[0] field i.e. word.
						// Minor optimization: Do this only if the individual words were not already
						// populated in the previous while loop for the word_confidence array.
						if (not utteranceWordsPopulated) {
							try {
								std::string utteranceWord = query(root,
									"results.[" + boost::to_string(idx1) + "].alternatives.[" +
									boost::to_string(idx2) + "].timestamps.[" +
									boost::to_string(idx3) + "].[0]" ).get_value<std::string>();

								// Append this to the list.
								utteranceWords_.push_back(utteranceWord);

								if (sttJsonResponseDebugging == true) {
									std::cout << traceIntro << "-->X24 utteranceWord=" << utteranceWord << std::endl;
								}
							} catch(std::exception const& e) {
								if (sttJsonResponseDebugging == true) {
									std::cout << traceIntro << "-->X25 ERROR idx2=" << idx2 << ", idx3=" << idx3 <<
										". JSON parsing error when reading the timestamps[0] word field: " << e.what() << std::endl;
								}

								break;
							}
						} // End of if (utteranceWordsPopulated == false)

						// Read the results.alternatives.timestamps[1] field i.e. startTime.
						try {
							SPL::float64 utteranceWordStartTime = query(root,
								"results.[" + boost::to_string(idx1) + "].alternatives.[" +
								boost::to_string(idx2) + "].timestamps.[" +
								boost::to_string(idx3) + "].[1]" ).get_value<float>();

							// Append this to the list.
							utteranceWordsStartTimes.push_back(utteranceWordStartTime);

							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X26 utteranceWordStartTime=" << utteranceWordStartTime << std::endl;
							}
						} catch(std::exception const& e) {
							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X27 ERROR idx2=" << idx2 << ", idx3=" << idx3 <<
									". JSON parsing error when reading the timestamps[1] startTime field: " << e.what() << std::endl;
							}

							break;
						}

						// Read the results.alternatives.timestamps[2] field i.e. endTime.
						try {
							SPL::float64 utteranceWordEndTime = query(root,
								"results.[" + boost::to_string(idx1) + "].alternatives.[" +
								boost::to_string(idx2) + "].timestamps.[" +
								boost::to_string(idx3) + "].[2]" ).get_value<float>();

							// Append this to the list.
							utteranceWordsEndTimes_.push_back(utteranceWordEndTime);

							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X28 utteranceWordEndTime=" << utteranceWordEndTime << std::endl;
							}
						} catch(std::exception const& e) {
							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X29 ERROR idx2=" << idx2 << ", idx3=" << idx3 <<
									". JSON parsing error when reading the timestamps[2] endTime field: " << e.what() << std::endl;
							}

							break;
						}
					} // End of while for parsing "results.alternatives.timestamps" JSON array.

					// Record the utterance start and end times now.
					if (utteranceWordsStartTimes.size() > 0) {
						utteranceStartTime_ = utteranceWordsStartTimes.at(0);
					}

					if (utteranceWordsEndTimes_.size() > 0) {
						utteranceEndTime_ = utteranceWordsEndTimes_.at(utteranceWordsEndTimes_.size()-1);
					}

					if (sttJsonResponseDebugging == true) {
						std::cout << traceIntro << "-->X30 utteranceWords=" <<
							boost::to_string(utteranceWords_) << std::endl;
						std::cout << traceIntro << "-->X31 utteranceWordsStartTimes=" <<
							boost::to_string(utteranceWordsStartTimes) << std::endl;
						std::cout << traceIntro << "-->X32 utteranceWordsEndTimes=" <<
							boost::to_string(utteranceWordsEndTimes_) << std::endl;
						std::cout << traceIntro << "-->X33 utteranceStartTime=" <<
							boost::to_string(utteranceStartTime_) << std::endl;
						std::cout << traceIntro << "-->X34 utteranceEndTime=" <<
							boost::to_string(utteranceEndTime_) << std::endl;
					}

					// Read the utterance confidence value which will be
					// available only for the finalized utterance.
					if (final_) {
						try {
							confidence_ = query(root,
								"results.[" + boost::to_string(idx1) + "].alternatives.[" +
								boost::to_string(idx2) + "].confidence").get_value<float>();
							cumulativeConfidenceForFullTranscription_ += confidence_;
							confidenceFound = true;

							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X35 confidence=" << confidence_ << std::endl;
							}
						} catch (std::exception const& e) {
							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X36 ERROR idx2=" << idx2 <<
									". JSON parsing error when reading the confidence field: " << e.what() << std::endl;
							}
						}
					}

					// Read either the partial or finalized utterance.
					try {
						tempUtteranceText = query(root,
							"results.[" + boost::to_string(idx1) + "].alternatives.[" +
							boost::to_string(idx2) + "].transcript").get_value<std::string>();

						if (sttJsonResponseDebugging == true) {
							std::cout << traceIntro << "-->X37 idx2=" << idx2 << ". utteranceText=" << tempUtteranceText << std::endl;
						}

						// If the STT result mode is full text, then keep accumulating it.
						// Do it only if it is a finalized utterance.
						if (sttResultMode == 3 && final_ && confidenceFound) {
							fullTranscriptionText_ += tempUtteranceText;
							// Since it is a final utterance, add a period.
							fullTranscriptionText_ += ". ";
						}

						// In the alternatives JSON array, only one element will have the
						// best result combined with confidence. We will store that as the
						// finalized utterance in the n-best alternative hypothesis scenario.
						if (sttResultMode == 2 && confidenceFound) {
							utteranceText_ = tempUtteranceText;
						}

						// In the case of partial utterance, we have to consider every
						// utterance that is being sent irrespective of final or not.
						if (sttResultMode == 1) {
							utteranceText_ = tempUtteranceText;
						}

						// If the STT result mode is 1 (partial utterance) or 2 (full utterance),
						// we must collect the n-best utterance alternatives.
						// For result mode 3 (full transcript), we don't support it.
						if (sttResultMode != 3 && final_) {
							utteranceAlternatives_.push_back(tempUtteranceText);
						}

						confidenceFound = false;
					} catch (std::exception const& e) {
						if (sttJsonResponseDebugging == true) {
							std::cout << traceIntro <<
								"-->X38 ERROR JSON parsing error when reading the transcript field: " << e.what() << std::endl;
						}

						// A special case for STT resule mode 1 (partial utterance) where
						// we must set the utteranceText_ to the very first alternative
						// in the list before we break from this inner loop. Because, that is the
						// correct finalized utterance with the actual confidence field
						// set to a valid value.
						if (sttResultMode == 1 &&
							final_ == true && utteranceAlternatives_.size() > 0) {
							utteranceText_ = utteranceAlternatives_.at(0);
						}

						// This means we have iterated through all the alternatives JSON array elements.
						// We can leave the inner while loop now.
						break;
					}
				} // End of inner while loop for iterating through the "alternatives" array elements.

				if (sttJsonResponseDebugging == true) {
					if (sttResultMode != 3) {
						std::cout << traceIntro <<
							"-->X39 Utterance Alternatives=" << boost::to_string(utteranceAlternatives_) << std::endl;
					}
				}

				// Let us now iterate through the "word_alternatives" array (Confusion Networks).
				// This is an optional field within the "results" JSON array.
				// Refer to this URL for the correct JSON format:
				// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#word_alternatives
				idx2 = -1;
				while(true) {
					++idx2;
					SPL::float64 startTime = 0.0;
					SPL::float64 endTime = 0.0;

					// Read the results.word_alternatives.startTime field.
					try {
						startTime = query(root,
							"results.[" + boost::to_string(idx1) + "].word_alternatives.[" +
							boost::to_string(idx2) + "].start_time").get_value<float>();

						if (sttJsonResponseDebugging == true) {
							std::cout << traceIntro << "-->X40 word_alternatives.startTime=" << startTime << std::endl;
						}
					} catch (std::exception const& e) {
						if (sttJsonResponseDebugging == true) {
							std::cout << traceIntro << "-->X41 ERROR idx2=" << idx2 <<
								". JSON parsing error when reading the word_alternatives.startTime field: " << e.what() << std::endl;
						}

						break;
					}

					// Read the results.word_alternatives.endTime field.
					try {
						endTime = query(root,
							"results.[" + boost::to_string(idx1) + "].word_alternatives.[" +
							boost::to_string(idx2) + "].end_time").get_value<float>();

						if (sttJsonResponseDebugging == true) {
							std::cout << traceIntro << "-->X42 word_alternatives.endTime=" << endTime << std::endl;
						}
					} catch (std::exception const& e) {
						if (sttJsonResponseDebugging == true) {
							std::cout << traceIntro << "-->X43 ERROR idx2=" << idx2 <<
								". JSON parsing error when reading the word_alternatives.endTime field: " << e.what() << std::endl;
						}

						break;
					}

					// Now iterate through the "alternatives" array which is a field
					// within the "word_alternatives" array.
					idx3 = -1;
					SPL::float64 wordConfidence = 0.0;
					std::string word = "";
					bool wordAlternativeFound = false;
					SPL::list<SPL::rstring> words;
					SPL::list<SPL::float64> confidences;

					while(true) {
						++idx3;
						wordConfidence = 0.0;
						word = "";

						// Read the results.word_alternatives.alternatives.confidence field.
						try {
							wordConfidence = query(root,
								"results.[" + boost::to_string(idx1) + "].word_alternatives.[" +
								boost::to_string(idx2) + "].alternatives.[" +
								boost::to_string(idx3) + "].confidence").get_value<float>();

							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro <<
									"-->X44 word_alternatives.alternatives.confidence=" << wordConfidence << std::endl;
							}
						} catch (std::exception const& e) {
							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X45 ERROR idx2=" << idx2 << ", idx3=" << idx3 <<
									". JSON parsing error when reading the word_alternatives.alternatives.confidence field: " <<
									e.what() << std::endl;
							}

							break;
						}

						// Read the results.word_alternatives.alternatives.confidence field.
						try {
							word = query(root,
								"results.[" + boost::to_string(idx1) + "].word_alternatives.[" +
								boost::to_string(idx2) + "].alternatives.[" +
								boost::to_string(idx3) + "].word").get_value<std::string>();

							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X46 word_alternatives.alternatives.word=" << word << std::endl;
							}
						} catch (std::exception const& e) {
							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X47 ERROR idx2=" << idx2 << ", idx3=" << idx3 <<
									". JSON parsing error when reading the word_alternatives.alternatives.word field: " <<
									e.what() << std::endl;
							}

							break;
						}

						// We got an alternative word and its confidence.
						wordAlternativeFound = true;
						// Insert the word and its confidence into individual lists.
						words.push_back(word);
						confidences.push_back(wordConfidence);
					} // End of inner while to read the results.word_alternatives.alternatives array.

					// If we have found at least one word alternative,
					// let us store it in the corresponding lists.
					if (wordAlternativeFound) {
						wordAlternatives_.push_back(words);
						wordAlternativesConfidences_.push_back(confidences);
						wordAlternativesStartTimes_.push_back(startTime);
						wordAlternativesEndTimes_.push_back(endTime);
					}
				} // End of outer while for reading the results.word_alternatives array.

				if (sttJsonResponseDebugging == true) {
					std::cout << traceIntro <<
						"-->X48 wordAlternatives=" << boost::to_string(wordAlternatives_) << std::endl;
					std::cout << traceIntro <<
						"-->X49 wordAlternativesConfidences=" << boost::to_string(wordAlternativesConfidences_) << std::endl;
					std::cout << traceIntro <<
						"-->X50 wordAlternativesStartTimes=" << boost::to_string(wordAlternativesStartTimes_) << std::endl;
					std::cout << traceIntro <<
						"-->X51 wordAlternativesEndTimes=" << boost::to_string(wordAlternativesEndTimes_) << std::endl;
				}

				// Let us now iterate through the "keywords_result" associative array (Keyword Spotting).
				// This is an optional field within the "results" JSON array.
				// Refer to this URL for the correct JSON format:
				// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#keyword_spotting
				idx2 = -1;
				SPL::int32 sizeOfkeywordsToBeSpottedList =
					keywordsToBeSpotted.size();

				while(keywordsSpottingThreshold > 0.0) {
					// If we have finished checking the results for all the keywords, we can exit from this loop.
					if (++idx2 >= sizeOfkeywordsToBeSpottedList) {
						if (sttJsonResponseDebugging == true) {
							std::cout << traceIntro << "-->X60 keywordsSpottingResults=" <<
								boost::to_string(keywordsSpottingResults_) << std::endl;
						}

						break;
					}

					std::string keyword = keywordsToBeSpotted.at(idx2);
					bool keywordMatchFound = false;

					// For this keyword, there may be 0 or more keywordsSpotting matches.
					// Get all the matching results for this keyword.
					idx3 = -1;
					while(true) {
						++idx3;
						SPL::float64 matchStartTime = 0.0;

						try {
							// Read the start_time field.
							matchStartTime = query(root,
								"results.[" + boost::to_string(idx1) + "].keywords_result." +
								keyword + ".[" + boost::to_string(idx3) + "].start_time").get_value<float>();

							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X54 keywords_result." <<
									keyword << ".[" << idx3 << "].start_time=" << matchStartTime << std::endl;
							}
						} catch (std::exception const& e) {
							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X55 ERROR idx2=" << idx2 << ", idx3=" << idx3 <<
									". JSON parsing error when reading the " <<
									"results.[" << boost::to_string(idx1) <<"].keywords_result." <<
									keyword << ".[" << boost::to_string(idx3) << "].start_time field. " << e.what() << std::endl;
							}

							break;
						}

						SPL::float64 matchEndTime = 0.0;

						try {
							// Read the end_time field.
							matchEndTime = query(root,
								"results.[" + boost::to_string(idx1) + "].keywords_result." +
								keyword + ".[" + boost::to_string(idx3) + "].end_time").get_value<float>();

							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X56 keywords_result." <<
									keyword << ".[" << idx3 << "].end_time=" << matchEndTime << std::endl;
							}
						} catch (std::exception const& e) {
							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X57 ERROR idx2=" << idx2 << ", idx3=" << idx3 <<
									". JSON parsing error when reading the " <<
									"results.[" << boost::to_string(idx1) << "].keywords_result." <<
									keyword << ".[" << boost::to_string(idx3) << "].end_time field. " << e.what() << std::endl;
							}

							break;
						}

						SPL::float64 matchConfidence = 0.0;

						try {
							// Read the confidence field.
							matchConfidence = query(root,
								"results.[" + boost::to_string(idx1) + "].keywords_result." +
								keyword + ".[" + boost::to_string(idx3) + "].confidence").get_value<float>();

							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X58 keywords_result." <<
									keyword << ".[" << idx3 << "].confidence=" << matchConfidence << std::endl;
							}
						} catch (std::exception const& e) {
							if (sttJsonResponseDebugging == true) {
								std::cout << traceIntro << "-->X59 ERROR idx2=" << idx2 << ", idx3=" << idx3 <<
									". JSON parsing error when reading the " <<
									"results.[" << boost::to_string(idx1) << "].keywords_result." <<
									keyword << ".[" + boost::to_string(idx3) << "].confidence field. " << e.what() << std::endl;
							}

							break;
						}

						// We got all the three values. Store them in a map.
						keywordsSpottingResultsMap_["start_time"] = matchStartTime;
						keywordsSpottingResultsMap_["end_time"] = matchEndTime;
						keywordsSpottingResultsMap_["confidence"] = matchConfidence;
						// Add this map to the list.
						keywordsSpottingResultsList_.push_back(keywordsSpottingResultsMap_);
						// Since we added it to the list, clear the map now.
						keywordsSpottingResultsMap_.clear();
						keywordMatchFound = true;
					} // End of while(true)

					if (keywordMatchFound) {
						// If we have found a result for a given keyword, add the
						// list containing the results for that keyword into the final results map.
						keywordsSpottingResults_[keyword] = keywordsSpottingResultsList_;
						// Since we added the results list to the final results map,
						// we can now delete the list.
						keywordsSpottingResultsList_.clear();
					}
				} // End of while(keywordsSpottingThreshold > 0.0)
			} // End of while looping through the "results" array elements.
		} // End of the else segment in if (sttErrorString_ != "")
	} // End of if (transcriptionResultAvailableForParsing_ == true || sttErrorString_ != "")

	if (fullTranscriptionCompleted_ || sttErrorFound_) {
		if (sttResultTupleWaitingToBeSent) {
			// Send the final tuple with transcriptionCompleted field set to true.
			// Set the STT error message attribute via the corresponding output function.
			splOperator.setTranscriptionCompleteAttribute(oTupleList.at(0));
		}

		// Reset these member variables since we fully completed the
		// transcription or encountered an STT error.
		transcriptionResult = "";
		sttResultTupleWaitingToBeSent = false;
		utteranceWordsSpeakers.clear();
		utteranceWordsSpeakersConfidences.clear();
		utteranceWordsStartTimes.clear();

		// If there is any STT error during Websocket connection establishment time,
		// that could be mostly due to invalid recognition request start parameters
		// in the on_open method. That means our STT connection has not yet been
		// established. In that case, we should perform the entire logic in this
		// if segment only if our Websocket connection is currently established.
		if (wsConnectionEstablished && oTupleList.size() > 0) {
			// Send either the very last tuple with transcriptionCompleted set to true or
			// with the STT error message set.
			// Dereference the oTuple object from the object pointer and send it.
			splOperator.submit(*(oTupleList.at(0)), 0);

			numberOfFullAudioConversationsTranscribed++;
			// Update the operator metric only if the user asked for a live update.
			if (sttLiveMetricsUpdateNeeded) {
				nFullAudioConversationsTranscribedMetric->setValueNoLock(numberOfFullAudioConversationsTranscribed);
			}

			if (sttJsonResponseDebugging == true) {
				std::string tempString = "transcription completion.";

				if (sttErrorFound_) {
					tempString = "STT error.";
				}

				std::cout << traceIntro <<
					"-->X52b At the tuple submission point for reporting " <<
					tempString << " Total audio conversation received=" <<
					numberOfFullAudioConversationsReceived <<
					", Total audio conversations transcribed=" <<
					numberOfFullAudioConversationsTranscribed << std::endl;
			}

			// Free the oTuple object since it is no longer needed.
			delete oTupleList.at(0);
			// Remove that vector element as well.
			oTupleList.erase(oTupleList.begin() + 0);

			// Some important cleanup logic here that needs to be understood and
			// validated for its correctness.
			// If this operator is configured to receive and process audio blob fragments instead of
			// reading and processing the entire audio content from an audio file and if we
			// removed the oTuple object above due to an STT error and the audio blob sender
			// thread above has not fully sent all the audio blob fragements for the
			// audio transcription that we just stopped due to an STT error, it is important for
			// us to clean up the remaining audio blob fragments from that audio converstion that are
			// still waiting in the vector to be sent to the STT service.
			if (sttErrorFound_ &&
				statusOfAudioDataTransmissionToSTT == AUDIO_BLOB_FRAGMENTS_BEING_SENT_TO_STT) {
				// We got an STT error in the middle of a transcription of the
				// partial audio blob fragments. If all the blob fragments
				// have not yet been sent to the STT service, we will clear the
				// remaining audio blob fragments in the vector that are
				// waiting to be sent to the STT service.
				while(audioBytes.size() > 0) {
					unsigned char * buffer = audioBytes.at(0);
					// Remove the items from the vector. It is no longer needed. Also free the original
					// data pointer that we obtained from the blob in the process method.
					audioBytes.erase(audioBytes.begin() + 0);
					audioSize.erase(audioSize.begin() + 0);

					if (buffer != NULL) {
						delete buffer;
					} else {
						// We removed all the remaininng audio blob fragments from the
						// audio conversation for which we got an STT error.
						break;
					}
				} // End of while(audioBytes.size() > 0)

				// If there are no more audio fragments left in the vector, reset the fragment count to 0.
				if(audioBytes.size() <= 0) {
					numberOfAudioBlobFragmentsReceivedInCurrentConversation = 0;
				}
			} // End of if (sttErrorFound_ &&
		} // End of if (wsConnectionEstablished && oTupleList.size() > 0)

		if (not wsConnectionEstablished && sttErrorFound_) {
			websocketConnectionErrorOccurred = true;
			// Always display this STT error message happening during the
			// Websocket connection establishment phase.
			std::cout << traceIntro <<
				"-->Error received from the Watson Speech To Text service: " << sttErrorString_ << std::endl;
			SPLAPPTRC(L_ERROR, traceIntro <<
					"-->Error received from the Watson Speech To Text service: " << sttErrorString_,
					"STT_Result_Processing");
		}

		// Reset this flag to indicate that STT service has no full audio data at this time.
		// i.e. no active transcription in progress now.
		statusOfAudioDataTransmissionToSTT = NO_AUDIO_DATA_SENT_TO_STT;
	} // End of if (fullTranscriptionCompleted_ || sttErrorFound_)
} // End of the on_message method.

// Whenever our existing Websocket connection to the Watson STT service is closed,
// this callback method will be called from the websocketpp layer.
template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::on_close(client* c, websocketpp::connection_hdl hdl) {
	// In the lab tests, I noticed that occasionally a Websocket connection can get
	// closed right after an on_open event without actually receiving the "listening" response
	// in the on_message event from the Watson STT service. This condition clearly means
	// that this is not a normal connection closure. Instead, the connection attempt has failed.
	// We must flag this as a connection error so that a connection retry attempt
	// can be triggered inside the ws_audio_blob_sender method.
	if (not wsConnectionEstablished) {
		// This connection was not fully established before.
		// This closure happened during an ongoing connection attempt.
		// Let us flag this as a connection error.
		websocketConnectionErrorOccurred = true;
		SPLAPPTRC(L_ERROR, traceIntro <<
			"-->Partially established Websocket connection closed with the Watson STT service during an ongoing connection attempt.",
			"on_close");
	} else {
		wsConnectionEstablished = false;
		// c->get_alog().write(websocketpp::log::alevel::app, "Websocket connection closed with the Watson STT service.");
		SPLAPPTRC(L_ERROR, traceIntro <<
			"-->Fully established Websocket connection closed with the Watson STT service.",
			"on_close");
	}
}

// When a Websocket connection handshake happens with the Watson STT serice for enabling
// TLS security, this callback method will be called from the websocketpp layer.
template<typename OP, typename OT>
context_ptr WatsonSTTImpl<OP, OT>::on_tls_init(client* c, websocketpp::connection_hdl) {
	//m_tls_init = std::chrono::high_resolution_clock::now();
	//context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv1);
	context_ptr ctx =
		websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

	try {
		ctx->set_options(boost::asio::ssl::context::default_workarounds |
			boost::asio::ssl::context::no_sslv2 |
			boost::asio::ssl::context::no_sslv3 |
			boost::asio::ssl::context::single_dh_use);
	} catch (std::exception& e) {
		SPLAPPTRC(L_ERROR, traceIntro << "-->" << e.what(), "on_tls_init");
	}

	return ctx;
}

// When a connection attempt to the Watson STT service fails, then this
// callback method will be called from the websocketpp layer.
template<typename OP, typename OT>
void WatsonSTTImpl<OP, OT>::on_fail(client* c, websocketpp::connection_hdl hdl) {
	websocketConnectionErrorOccurred = true;
	// c->get_alog().write(websocketpp::log::alevel::app, "Websocket connection to the Watson STT service failed.");
	SPLAPPTRC(L_ERROR, traceIntro << "-->Websocket connection to the Watson STT service failed.", "on_fail");
}

}}}}

#endif /* COM_IBM_STREAMS_STTGATEWAY_WATSONSTTIMPL_HPP_ */
