/*
 * WatsonSTTImpl.hpp
 *
 * Licensed Materials - Property of IBM
 * Copyright IBM Corp. 2019, 2020
 *
 *  Created on: Jan 14, 2020
 *      Author: joergboe
*/

#ifndef COM_IBM_STREAMS_STTGATEWAY_WATSONSTTIMPLRECEIVER_HPP_
#define COM_IBM_STREAMS_STTGATEWAY_WATSONSTTIMPLRECEIVER_HPP_

#include <string>
#include <vector>
#include <atomic>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>

// This operator heavily relies on the Websocket++ header only library.
// https://docs.websocketpp.org/index.html
// This C++11 library code does the asynchronous full duplex Websocket communication with
// the Watson STT service via a series of event handlers (a.k.a callback methods).
// The websocketpp gives c++11 support also for older compilers
// With c++11 capable compiler the stdlib is used
#include <websocketpp/common/functional.hpp>

// Bulk of the logic in this operator class appears in those event handler methods below.
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

// A nice read in this URL about using property_tree for JSON parsing:
// http://zenol.fr/blog/boost-property-tree/en.html
// #include <boost/property_tree/ptree.hpp>
// #include <boost/property_tree/json_parser.hpp>

#include <boost/exception/to_string.hpp>

// SPL Operator related includes
#include <SPL/Runtime/Type/SPLType.h>
#include <SPL/Runtime/Function/SPLFunctions.h>
#include <SPL/Runtime/Common/Metric.h>
#include <SPL/Runtime/Utility/Mutex.h>

#include "WatsonSTTConfig.hpp"
#include "Decoder.hpp"

//#include <SttGatewayResource.h>

namespace com { namespace ibm { namespace streams { namespace sttgateway {

// Websocket related type definitions.
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
// Pull out the type of messages sent by our config
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;

enum class WsState : char {
	idle,       // initial idle state
	start,      // start a new connection request (set from sender thread) - transient
	connecting, // connection attempt started (from receiver tread) - transient
	open,       // connection is opened (on open event) - transient
	listening,  // listening received (on message event) - stable state
	closing,    // connection is closing (set from receiver thread thread) - transient
	error,      // error received during transcription (error message received on message) - transient
	closed,     // connection has closed (on close event) - stable state
	failed,     // connection has failed (on fail event) - stable state
	crashed     // Error was caught in ws_init (receiver thread)- stable state
};
// helper function to make a pretty print
const char * wsStateToString(WsState ws);
// helper function to determine whether the receiver has reached an inactive state
bool receiverHasStopped(WsState ws);
// helper function to determine whether the receiver is in a transient state
bool receiverHasTransientState(WsState ws);

class SpeakerProcessor;
class KeywordProcessor;

/*
 * Implementation class for operator Watson STT
 * Move almost of the c++ code of the operator into this class to take the advantage of c++ editor support
 *
 * Template argument : OP: Operator type (must inherit from SPL::Operator)
 *                     OT: Output Tuple type
 */
template<typename OP, typename OT>
class WatsonSTTImplReceiver : public WatsonSTTConfig {
public:
	typedef WatsonSTTConfig Config;
	typedef struct { SPL::float64 startTime; SPL::int32 speaker; SPL::float64 confidence; } SpeakerUpdatesStruct;
	//Constructors
	WatsonSTTImplReceiver(OP & splOperator_, Config config_);
	//WatsonSTTImpl(WatsonSTTImpl const &) = delete;

	//Destructor
	~WatsonSTTImplReceiver();
protected:
	// Notify port readiness
	void allPortsReady();

	// Notify pending shutdown
	void prepareToShutdown();

	// Processing for websocket receiver threads
	void process(uint32_t idx);


private:
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

	//bool on_ping(client* c, websocketpp::connection_hdl hdl, std::string mess);

	//void on_pong(client* c, websocketpp::connection_hdl hdl, std::string mess);

	// Websocket initialization thread method
	void ws_init();

protected:
	OP & splOperator;

	// Websocket operations related member variables.
	// values set from receiver thread and read from sender side
	// All values are primitive atomic values, no locking
	std::atomic<WsState> wsState;

	// this flag us set from receiver thread and reset from sender thread
	// it is set at the end of the on_message method, when the stt service sends a 'listening' event
	// after transcription
	std::atomic<bool> transcriptionFinalized;

	// This is set from the sender thread when the the next conversation is queued
	// If this flag is set when a transcription completes, the connection is kept
	// Otherwise the connection is closed to avoid the race condition when the next conversation arrives
	std::atomic<bool> nextConversationQueued;

	// This values are set from receiver thread when state is connecting
	// the sender thread requires the values but should not use them during connecting state
	client *wsClient;
	websocketpp::connection_hdl wsHandle;

	// the access token used in receiver-thread during ws_init after wsState changes to 'start'
	// the value is copied from the sender- to receiver-thread before a 'makeNewWebsocketConnection' has been flagged
	// this is a change of wsStae from any state to 'start'
	std::string accessToken;

	// The tuple with assignments from the input port
	// to be used if transcription results or error indications have to be sent
	// the sender thread guarantees that here is an valid value until transcriptionFinalized is flagged
	// this otuple may change during a transcription when the input receives new input tuples for the
	// current transcription. The previous value remains valid until transcriptionFinalized is flagged.
	std::atomic<OT *> recentOTuple;
private:
	// when the on_message method is about to send something, this member is used to store the
	// output tuple pointer.
	// First we expect the results for utterance, alternatives, and word alternatives
	// Then we expect the speaker results.
	// This value is used to store the non output tuple contains with the utterances and related attributes
	// until the appropriate speaker result is received.
	// The member is reset, after the tuple was submitted
	OT * oTupleUsedForSubmission;

	std::atomic<SPL::int64> nWebsocketConnectionAttemptsCurrent;
	std::atomic<SPL::int64> nFullAudioConversationsTranscribed;
	std::atomic<SPL::int64> nFullAudioConversationsFailed;

	// Decoder class for json decoding
	Decoder dec;
	// list of the words start times used for the speaker label consistency check
	SPL::list<SPL::float64> myUtteranceWordsStartTimes;
	// Custom metrics for this operator.
	SPL::Metric * const nWebsocketConnectionAttemptsCurrentMetric;
	SPL::Metric * const nFullAudioConversationsTranscribedMetric;
	SPL::Metric * const nFullAudioConversationsFailedMetric;
	SPL::Metric * const wsConnectionStateMetric;

	static const SpeakerProcessor emptySpeakerResults;
	static const KeywordProcessor emptyKeywordProcessor;

protected:
	// Helper functions
	void setWsState(WsState ws);
	inline void incrementNWebsocketConnectionAttemptsCurrent();
	inline void incrementNFullAudioConversationsTranscribed();
	inline void incrementNFullAudioConversationsFailed();
	inline SPL::float64 getNWebsocketConnectionAttemptsCurrent() { return nWebsocketConnectionAttemptsCurrent.load(); };

private:
	// send out the error with the wit the specified reason
	// This function consumes a non finalized output tuple (oTupleUsedForSubmission) if any
	// Or uses a the recent output tuple (recentOTuple).
	// If no recent output tuple is available, no tuple is sent and an error log is emitted
	// increment the FullAudioConversationsFailed
	void sendErrorTuple(const std::string & reason);

	// send the finalization tuple of an conversation
	void sendTranscriptionCompletedTuple(OT * otuple);
};

/*
 * Data structure and and functions for the processing of the speaker labels
 * The run function splits the speaker labels into two lists:
 * 1. the list that corresponds to the utterance word list
 * 2. the speaker updates lists
 * Get the first list with getUtteranceWordsSpeakers() and getUtteranceWordsSpeakersConfidences()
 * Get the second list with getUtteranceWordsSpeakerUpdates()
 */
class SpeakerProcessor {
	const rapidjson::SizeType spkSize;
	const SPL::list<SPL::float64> & spkFrom;
	const SPL::list<SPL::int32> &   spkSpk;
	const SPL::list<SPL::float64> & spkCfd;
	const size_t wordListSize;
	const SPL::list<SPL::float64> & myUtteranceWordsStartTimes;
	const std::string & traceIntro;
	const std::string & payload;
	// the speaker result lists - each entry corresponds to the entry in the word list
	SPL::list<SPL::float64> spkFromNew;
	SPL::list<SPL::int32>   spkSpkNew;
	SPL::list<SPL::float64> spkCfdNew;
	// indexes of speaker update
	std::vector<rapidjson::SizeType> spkUpdateIndexes;

public:
	SpeakerProcessor(
			const Decoder & dec_,
			const SPL::list<SPL::float64> & myUtteranceWordsStartTimes_,
			const std::string & traceIntro_,
			const std::string & payload_);

	SpeakerProcessor();

	SpeakerProcessor(const SpeakerProcessor&) = delete;
	SpeakerProcessor(SpeakerProcessor&&) = delete;
	SpeakerProcessor& operator=(const SpeakerProcessor&) = delete;
	SpeakerProcessor& operator=(const SpeakerProcessor&&) = delete;

	void run();

	SPL::list<SPL::int32> getUtteranceWordsSpeakers() const { return spkSpkNew; }

	SPL::list<SPL::float64> getUtteranceWordsSpeakersConfidences() const { return spkCfdNew; }

	template<typename TUPLE>
	SPL::list<TUPLE> getUtteranceWordsSpeakerUpdates() const;
};

/* data struct and function to get the keyword result */
class KeywordProcessor {
	const bool isEmpty;
	const DecoderKeywordsResult::ResultMapType & keywordResults;

public:
	KeywordProcessor(const DecoderKeywordsResult::ResultMapType & keywordResults_);
	KeywordProcessor();
	KeywordProcessor(const KeywordProcessor&) = delete;
	KeywordProcessor(KeywordProcessor&&) = delete;
	KeywordProcessor& operator=(const KeywordProcessor&) = delete;
	KeywordProcessor& operator=(const KeywordProcessor&&) = delete;
	template<typename T>
	void getKeywordsSpottingResults(SPL::map<SPL::rstring, SPL::list<T> > & destination) const;
};

typename SPL::map<SPL::rstring, SPL::float64> KeyWordEmergenceMap;

template<typename OP, typename OT>
WatsonSTTImplReceiver<OP, OT>::WatsonSTTImplReceiver(OP & splOperator_,Config config_)
:
		Config(config_),
		splOperator(splOperator_),

		wsState{WsState::idle},
		transcriptionFinalized(true),
		nextConversationQueued(false),
		wsClient(nullptr),
		wsHandle{},

		accessToken{},

		recentOTuple{},
		oTupleUsedForSubmission{},

		nWebsocketConnectionAttemptsCurrent{0},
		nFullAudioConversationsTranscribed{0},
		nFullAudioConversationsFailed{0},
		dec(*this),
		myUtteranceWordsStartTimes(),

		// Custom metrics for this operator are already defined in the operator model XML file.
		// Hence, there is no need to explicitly create them here.
		// Simply get the custom metrics already defined for this operator.
		// The update of metrics nFullAudioConversationsTranscribed and nFullAudioConversationsFailed depends on parameter sttLiveMetricsUpdateNeeded
		nWebsocketConnectionAttemptsCurrentMetric{ & splOperator.getContext().getMetrics().getCustomMetricByName("nWebsocketConnectionAttemptsCurrent")},
		nFullAudioConversationsTranscribedMetric{ & splOperator.getContext().getMetrics().getCustomMetricByName("nFullAudioConversationsTranscribed")},
		nFullAudioConversationsFailedMetric{ & splOperator.getContext().getMetrics().getCustomMetricByName("nFullAudioConversationsFailed")},
		wsConnectionStateMetric{ & splOperator.getContext().getMetrics().getCustomMetricByName("wsConnectionState")}
{
	std::cout << "nFullAudioConversationsTranscribed.is_lock_free()= " << nFullAudioConversationsTranscribed.is_lock_free() << std::endl;
}

template<typename OP, typename OT>
WatsonSTTImplReceiver<OP, OT>::~WatsonSTTImplReceiver() {
	if (wsClient) {
		delete wsClient;
	}
}

template<typename OP, typename OT>
void WatsonSTTImplReceiver<OP, OT>::allPortsReady() {
	// create the operator receiver thread
	uint32_t userThreadIndex = splOperator.createThreads(1);
	if (userThreadIndex != 0) {
		throw std::invalid_argument(traceIntro +" WatsonSTTImpl invalid userThreadIndex");
	}
}

template<typename OP, typename OT>
void WatsonSTTImplReceiver<OP, OT>::prepareToShutdown() {
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
				wsClient->close(wsHandle, websocketpp::close::status::internal_endpoint_error, "Shutdown");
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
void WatsonSTTImplReceiver<OP, OT>::process(uint32_t idx) {
	SPLAPPTRC(L_INFO, traceIntro << "-->Run thread idx=" << idx, "ws_receiver");
	// run the operator receiver thread
	ws_init();
}

// This method initializes the Websocket driver, TLS and then
// opens a connection. This is going to run on its own thread.
template<typename OP, typename OT>
void WatsonSTTImplReceiver<OP, OT>::ws_init() {

	while (not splOperator.getPE().getShutdownRequested()) {
		if (wsState.load() != WsState::start) {
			// Keep waiting in this while loop until
			// a need arises to make a new Websocket connection.
			//SPLAPPTRC(L_TRACE, traceIntro << "-->RE0: wsState=" << wsStateToString(wsState.load()) <<
			//		" No connection request in thread ws_init, block for " <<
			//		receiverWaitTimeWhenIdle << " second", "ws_receiver");
			SPL::Functions::Utility::block(receiverWaitTimeWhenIdle);
			continue;
		}
		// here we are in state WsState::start
		setWsState(WsState::connecting);
		oTupleUsedForSubmission = nullptr;

		// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-websockets#WSopen
		std::string uri = this->uri;
		// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-input#models
		uri += "?model=" + baseLanguageModel;
		// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-input#logging
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
		//wsConnectionEstablished = false;

		if (wsClient) {
			// If we are going to do a reconnection, then free the
			// previously created Websocket client object.
			SPLAPPTRC(L_DEBUG, traceIntro << "-->RE1: Delete client " << uri, "ws_receiver");
			delete wsClient;
			wsClient = nullptr;
		}

		try {
			SPLAPPTRC(L_INFO, traceIntro << "-->RE2: Going to connect to " << uri, "ws_receiver");

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
			wsClient->set_tls_init_handler(bind(&WatsonSTTImplReceiver<OP, OT>::on_tls_init,this,wsClient,::_1));

			// Register our other event handlers.
			wsClient->set_open_handler(bind(&WatsonSTTImplReceiver<OP, OT>::on_open,this,wsClient,::_1));
			wsClient->set_fail_handler(bind(&WatsonSTTImplReceiver<OP, OT>::on_fail,this,wsClient,::_1));
			wsClient->set_message_handler(bind(&WatsonSTTImplReceiver<OP, OT>::on_message,this,wsClient,::_1,::_2));
			wsClient->set_close_handler(bind(&WatsonSTTImplReceiver<OP, OT>::on_close,this,wsClient,::_1));
			//wsClient->set_ping_handler(bind(&WatsonSTTImplReceiver<OP, OT>::on_ping,this,wsClient,::_1,::_2));
			//wsClient->set_pong_handler(bind(&WatsonSTTImplReceiver<OP, OT>::on_pong,this,wsClient,::_1,::_2));

			// Create a connection to the given URI and queue it for connection once
			// the event loop starts
			SPLAPPTRC(L_DEBUG, traceIntro << "-->RE3 (after call back setup)", "ws_receiver");
			websocketpp::lib::error_code ec;
			// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-basic-request#using-the-websocket-interface
			client::connection_ptr con = wsClient->get_connection(uri, ec);
			SPLAPPTRC(L_DEBUG, traceIntro << "-->RE4 (after get_connection) ec=" << ec, "ws_receiver");
			if (ec)
				throw ec;

			wsClient->connect(con);
			// A new Websocket connection has just been made. Reset this flag.
			SPLAPPTRC(L_DEBUG, traceIntro << "-->RE5 (after connect)", "ws_receiver");

			// Start the ASIO io_service run loop
			wsClient->run();
			SPLAPPTRC(L_INFO, traceIntro << "-->RE10 (after run)", "ws_receiver");
		} catch (const std::exception & e) {
			SPLAPPTRC(L_ERROR, traceIntro << "-->RE91 " << typeid(e).name() << ": "<< e.what(), "ws_receiver");
			setWsState(WsState::crashed);
			//SPL::Functions::Utility::abort(__FILE__, __LINE__);
		} catch (const websocketpp::lib::error_code & e) {
			//websocketpp::lib::error_code is a class -> catching by reference makes sense
			SPLAPPTRC(L_ERROR, traceIntro << "-->RE92 websocketpp::lib::error_code: e=" << e <<
					" message=" << e.message(), "ws_receiver");
			setWsState(WsState::crashed);
			//SPL::Functions::Utility::abort(__FILE__, __LINE__);
		} catch (...) {
			SPLAPPTRC(L_ERROR, traceIntro << "-->RE93 Other exception in WatsonSTT operator's Websocket initializtion.", "ws_receiver");
			setWsState(WsState::crashed);
			//SPL::Functions::Utility::abort(__FILE__, __LINE__);
		}
		// finally delete recentOTuple
		recentOTuple.store(nullptr);
	} // End of while loop.
	SPLAPPTRC(L_INFO, traceIntro <<
			"-->End of loop ws_init: getShutdownRequested()=" << splOperator.getPE().getShutdownRequested(),
			"ws_receiver");
} // End: WatsonSTTImpl<OP, OT>::ws_init

// When the Websocket connection to the Watson STT service is made successfully,
// this callback method will be called from the websocketpp layer.
// Either open or fail will be called for each connection. Never both.
template<typename OP, typename OT>
void WatsonSTTImplReceiver<OP, OT>::on_open(client* c, websocketpp::connection_hdl hdl) {

	setWsState(WsState::open);

	SPLAPPTRC(L_DEBUG, traceIntro << "-->RE6 (on_open)", "ws_receiver");
	// On Websocket connection open, establish a session with the STT service.
	// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-websockets#WSstart
	// We have to form a proper JSON message structure for the
	// STT recognition request start message with all the output features we want.
	// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#output
	std::string msg = "{\"action\" : \"start\"";
	msg += ", \"content-type\" : \"" + contentType + "\"";

	std::string interimResultsNeeded = "false";

	if (sttOutputResultMode == 1 || sttOutputResultMode == 2) {
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

	c->send(hdl,msg,websocketpp::frame::opcode::text);
	// Store this handle to be used from process and shutdown methods of this operator.
	wsHandle = hdl;
	// c->get_alog().write(websocketpp::log::alevel::app, "Sent Message: "+msg);
	SPLAPPTRC(L_INFO, traceIntro <<
			"-->RE7 A recognition request start message was sent to the Watson STT service at host:" <<
			c->get_con_from_hdl(hdl)->get_host() << " message=" << msg, "ws_receiver");
} //End: WatsonSTTImpl<OP, OT>::on_open

// Whenever a message (transcription result, STT service message or an STT error message) is
// received from the STT service, this callback method will be called from the websocketpp layer.
// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-websockets#WSexample
template<typename OP, typename OT>
void WatsonSTTImplReceiver<OP, OT>::on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {

	// c->get_alog().write(websocketpp::log::alevel::app, "Received Reply: "+msg->get_payload());
	//
	// This debugging variable is enabled/disabled via an operator parameter.
	// When enabled, it will display the full JSON message and
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

	// Entry state check
	WsState entryState = wsState.load();
	SPLAPPTRC(L_DEBUG, traceIntro << "-->on_message entyState: " << wsStateToString(entryState), "ws_receiver");
	if ((entryState != WsState::open) && (entryState != WsState::listening))
		throw std::runtime_error(traceIntro + "-->RE80 Unexpected entryState in ws on_message; state: " + std::string(wsStateToString(entryState)));

	// Local state variables
	bool fullTranscriptionCompleted_ = false;	// Is set when a second listening event is received

	// Do json decoding work
	const std::string & payload_ = msg->get_payload();
	bool completeResults = Config::sttOutputResultMode == Config::complete;
	SPLAPPTRC(L_TRACE, traceIntro << "-->RE7 on_message payload_: " << payload_, "ws_receiver");
	dec.doWork(payload_);

	// STT error will have the following message format.
	// {"error": "unable to transcode data stream audio/wav -> audio/x-float-array "}
	// STT ws message to start the transcription has the format.
	// {"state":"listening"}
	// get the condition matrix
	// we expect either a state, error, result or a speaker message
	// a resultIndex must come along with a result
	// a state message must always be 'listening'
	const bool stateFound_           = dec.DecoderState::hasResult();
	const bool stateListeningFound_  = dec.isListening();
	const bool sttErrorFound_        = dec.DecoderError::hasResult();
	const bool utteranceResultFound_ = dec.DecoderResults::hasResult();
	const bool speakerResultFound_   = dec.DecoderSpeakerLabels::hasResult();
	const bool resultIndexFound_     = dec.DecoderResultIndex::hasResult();
	// Check the input expectations
	int numFound = 0;
	if (stateFound_)
		++numFound;
	if (sttErrorFound_)
		++numFound;
	if (utteranceResultFound_)
		++numFound;
	if (speakerResultFound_)
		++numFound;
	// in general we expect only one of the results state, error, utterance or speaker
	// the result index may be there along with utterance and is not checked here
	// in sttResultMode 3 (complete result) utterance and speaker may be in one responmse message
	if ((numFound > 2) || ((numFound == 2) && (not utteranceResultFound_ || not speakerResultFound_))) {
		std::stringstream message;
		message << traceIntro << "-->RE88 Unexpected decoding results - stateFound_ : " << stateFound_ << " sttErrorFound_: "
				<< sttErrorFound_ << " utteranceResultFound_: " << utteranceResultFound_ << " speakerResultFound_: "
				<< speakerResultFound_ << " payload:" << payload_;
		throw std::runtime_error(message.str());
	}
	if (stateFound_ && not stateListeningFound_) {
		throw std::runtime_error(traceIntro + "-->RE83 Unexpected decoding results - stateFound_ && not stateListeningFound_ - payload_: " + payload_);
	}
	if (not stateFound_ && not sttErrorFound_ && not utteranceResultFound_ && not speakerResultFound_)
		SPLAPPTRC(L_WARN, traceIntro << "-->RE84 None decoding results found! Ignore message. payload_:" << payload_, "ws_receiver");

	// no error
	if (stateListeningFound_) {
		if (entryState == WsState::open) {
			// This is the "listening" response from the STT service for the
			// new recognition request that was initiated in the on_open method above.

			nWebsocketConnectionAttemptsCurrent = 0;
			nWebsocketConnectionAttemptsCurrentMetric->setValueNoLock(nWebsocketConnectionAttemptsCurrent);

			SPLAPPTRC(L_DEBUG, traceIntro <<
				"-->RE20 state listening reached. Websocket connection established with the Watson STT service.",
				"ws_receiver");

			setWsState(WsState::listening);
			return;

		} else { //state listening
			// This is the "listening" response from the STT service for the
			// transcription completion for the audio data that was sent earlier.
			// This response also indicates that the STT service is ready to do a new transcription.
			fullTranscriptionCompleted_ = true;
			// But we close the connection now if further conversation is queued:
			// see issue #44
			// There is no reliable way to keep the connection forever open (except to send idle payload?)
			// Ping messages do not keep the connection open
			// The session timeout can not be switched off https://cloud.ibm.com/docs/speech-to-text?topic=speech-to-text-websockets#WSkeep
			// Thus the stt service closes the connection after approx. 30 sec.
			// This may produce is rare cases a race condition of the connection close from stt and the transmission of
			// new speech samples. In this case a whole file may get lost.
			if (nextConversationQueued.load()) {
				SPLAPPTRC(L_DEBUG, traceIntro <<
					"-->RE85 Transcription completion and nextConversationQueued - Keep connection", "ws_receiver");
			} else {
				setWsState(WsState::closing);
				wsClient->close(hdl, websocketpp::close::status::going_away, "");
				SPLAPPTRC(L_DEBUG, traceIntro <<
					"-->RE86 Transcription completion and no nextConversationQueued. Going to close", "ws_receiver");
			}
		}
	}

	// Tuple output logic
	// Error
	if (sttErrorFound_) {

		std::string sttErrorString_ = dec.DecoderError::getResult();
		SPLAPPTRC(L_ERROR, traceIntro << "-->RE25 STT error message=" << sttErrorString_, "ws_receiver");
		sendErrorTuple(sttErrorString_);
		setWsState(WsState::error);
		return;

	}

	// end of conversation reached
	if (fullTranscriptionCompleted_) {
		// metric
		incrementNFullAudioConversationsTranscribed();

		// The conversation should end with a completed submission cycle (speaker labels and utterances are sent)
		if (oTupleUsedForSubmission) {
			SPLAPPTRC(L_ERROR, traceIntro << "-->RE29 fullTranscriptionCompleted_ but non finalized oTupleUsedForSubmission available", "ws_receiver");
			splOperator.submit(*oTupleUsedForSubmission, 0);
			oTupleUsedForSubmission = nullptr;
		}
		if (Config::isTranscriptionCompletedRequested) {
			OT * myRecentOTuple = recentOTuple.load(); // load atomic
			// there should be a conversation which means recentOTuple must not be null
			// log an error if not
			if (myRecentOTuple) {
				sendTranscriptionCompletedTuple(myRecentOTuple);
			} else {
				SPLAPPTRC(L_ERROR, traceIntro << "-->RE 31 no recent output tuple available but non finalized fullTranscriptionCompleted_. payload_: " << payload_, "ws_receiver");
			}
		}
		SPLAPPTRC(L_DEBUG, traceIntro << "-->RE 30a send window punctuation marker.", "ws_receiver");
		// delete the recentOTuple if end of conversation was reached
		// flag transcriptionFinalized
		recentOTuple.store(nullptr);
		// flag the conversation end in any case
		splOperator.submit(SPL::Punctuation::WindowMarker, 0);
		transcriptionFinalized.store(true);

		return;
	}

	// utterance result(s) found
	if (utteranceResultFound_) {

		if (oTupleUsedForSubmission) {
			SPLAPPTRC(L_ERROR, traceIntro << "-->RE32 utteranceResultFound_ but non finalized oTupleUsedForSubmission available", "ws_receiver");
			splOperator.submit(*oTupleUsedForSubmission, 0);
			oTupleUsedForSubmission = nullptr;
		}

		OT * myRecentOTuple = recentOTuple.load(); // load atomic
		if (myRecentOTuple) {
			bool finalUtteranceOrModeComplete = false;
			// if sttResultgMode is complete use a fixed value of true for value final
			if (Config::sttOutputResultMode == WatsonSTTConfig::complete) {
				finalUtteranceOrModeComplete = true;
			} else {
				if (dec.DecoderResults::getSize() > 1) {
					SPLAPPTRC(L_ERROR, traceIntro << "-->RE33 more than one result received in partial mode", "ws_receiver");
				}
				if (dec.DecoderResults::getSize() == 0) {
					SPLAPPTRC(L_ERROR, traceIntro << "-->RE34 zero results received in partial mode", "ws_receiver");
				} else {
					finalUtteranceOrModeComplete = dec.DecoderFinal::getResult(0);
				}
			}
			// prepare speaker label consistency check
			if (Config::identifySpeakers) {
				myUtteranceWordsStartTimes = dec.DecoderAlternatives::getUtteranceWordsStartTimes();
			}
			// clean speaker values which are probably set
			if (Config::identifySpeakers)
				splOperator.setSpeakerResultAttributes(myRecentOTuple, emptySpeakerResults);
			// prepare keywords
			const KeywordProcessor keywordProc(dec.DecoderKeywordsResult::getKeywordsSpottingResults());
			// set utterance result attributes
			splOperator.setResultAttributes(
					myRecentOTuple,
					dec.DecoderResultIndex::getResult(),
					finalUtteranceOrModeComplete,
					// utterances
					dec.DecoderAlternatives::getConfidence(),
					dec.DecoderAlternatives::getUtteranceStartTime(),
					dec.DecoderAlternatives::getUtteranceEndTime(),
					dec.DecoderAlternatives::getUtteranceText(),
					// alternatives
					dec.DecoderAlternatives::getUtteranceAlternatives(),
					// word alternatives
					dec.DecoderAlternatives::getUtteranceWords(),
					dec.DecoderAlternatives::getUtteranceWordsConfidences(),
					dec.DecoderAlternatives::getUtteranceWordsStartTimes(),
					dec.DecoderAlternatives::getUtteranceWordsEndTimes(),
					// confusion
					dec.DecoderWordAlternatives::getWordAlternatives(),
					dec.DecoderWordAlternatives::getWordAlternativesConfidences(),
					dec.DecoderWordAlternatives::getWordAlternativesStartTimes(),
					dec.DecoderWordAlternatives::getWordAlternativesEndTimes(),
					keywordProc
			);

			// output logic of the tuple
			if (finalUtteranceOrModeComplete) {
				// send or queue final utterances
				if (Config::identifySpeakers) {
					SPLAPPTRC(L_DEBUG, traceIntro << "-->RE35 queue utterance results until speaker labels are available", "ws_receiver");
					oTupleUsedForSubmission = myRecentOTuple;
				} else {
					SPLAPPTRC(L_DEBUG, traceIntro << "-->RE36a send utterance results tuple", "ws_receiver");
					splOperator.submit(*myRecentOTuple, 0);
				}
			} else {
				// send non final utterances only if they are requested
				if (Config::nonFinalUtterancesNeeded) {
					SPLAPPTRC(L_DEBUG, traceIntro << "-->RE36 send utterance results tuple", "ws_receiver");
					splOperator.submit(*myRecentOTuple, 0);
				}
			}

		} else {
			throw std::runtime_error(traceIntro +
					"-->no recent output tuple available but non finalized fullTranscriptionCompleted_. payload_: " + payload_);
		}

		// fall through to speaker label decoder if utterance and speakers are in one response (complete result)
	}

	// speaker results found
	if (speakerResultFound_) {
		if (not Config::identifySpeakers) {
			SPLAPPTRC(L_WARN, traceIntro << "-->RE37 ignore speaker labels because they are not requested", "ws_receiver");
		} else {
			if (oTupleUsedForSubmission) {
				SPLAPPTRC(L_DEBUG, traceIntro << "-->RE38 send queued utterance results tuple with speaker info", "ws_receiver");
				SpeakerProcessor spkproc(dec, myUtteranceWordsStartTimes, traceIntro, payload_);
				spkproc.run();
				// assign speaker labels to output tuple
				splOperator.setSpeakerResultAttributes(oTupleUsedForSubmission, spkproc);
				splOperator.submit(*oTupleUsedForSubmission, 0);
				oTupleUsedForSubmission = nullptr;
			} else {
				SPLAPPTRC(L_WARN, traceIntro << "-->RE39 ignore speaker info because no oTuple with utterances is available. payload_:" << payload_, "ws_receiver");
			}
		}
		return;
	}
} // End of the on_message method.

SpeakerProcessor::SpeakerProcessor(
		const Decoder & dec_,
		const SPL::list<SPL::float64> & myUtteranceWordsStartTimes_,
		const std::string & traceIntro_,
		const std::string & payload_) :
	spkSize(dec_.DecoderSpeakerLabels::getSize()),
	spkFrom(dec_.DecoderSpeakerLabels::getFrom()),
	spkSpk(dec_.DecoderSpeakerLabels::getSpeaker()),
	spkCfd(dec_.DecoderSpeakerLabels::getConfidence()),
	wordListSize(myUtteranceWordsStartTimes_.size()),
	myUtteranceWordsStartTimes(myUtteranceWordsStartTimes_),
	traceIntro(traceIntro_),
	payload(payload_),
	spkFromNew(),
	spkSpkNew(),
	spkCfdNew(),
	spkUpdateIndexes() {
		spkFromNew.reserve(wordListSize);
		spkSpkNew.reserve(wordListSize);
		spkCfdNew.reserve(wordListSize);
}

SpeakerProcessor::SpeakerProcessor() :
	spkSize(0),
	// the references to the temporary lists are invalid but are never used in this instance in get...
	// spkUpdateIndexes.size() is zero in this instance
	spkFrom(SPL::list<SPL::float64>()),
	spkSpk(SPL::list<SPL::int32>()),
	spkCfd(SPL::list<SPL::float64>()),
	wordListSize(0),
	myUtteranceWordsStartTimes(SPL::list<SPL::float64>()),
	traceIntro(""),
	payload(""),
	spkFromNew(),
	spkSpkNew(),
	spkCfdNew(),
	spkUpdateIndexes() {
}

void SpeakerProcessor::run() {
	// speaker consistency check - check the from time of the speaker labels against the from time of the words list
	// this test guarantees that for each word in word list, a speaker label is correctly assigned
	// if a speaker label is missing for a specific word from time, the value -1 is assigned
	// speaker index map - key: skpFromTime value: speaker list index
	std::unordered_map<SPL::float64, rapidjson::SizeType> spkIndexMap;
	for (rapidjson::SizeType i = 0; i < spkSize; i++) {
		spkIndexMap.insert(std::pair<const SPL::float64, rapidjson::SizeType>(spkFrom[i], i));
	}
	// size check
	auto wordListSize = myUtteranceWordsStartTimes.size();
	if (spkSize != wordListSize) {
		SPLAPPTRC(L_DEBUG, traceIntro << "-->RE41 Word list size " << wordListSize <<
				" and speaker list size " << spkSize << " are not equal.", "ws_receiver");
	}
	// the result lists - each entry corresponds to the entry in the word list
	// a set with all indexes used from the word list
	std::unordered_set<rapidjson::SizeType> usedSpkIndexes;
	for (rapidjson::SizeType i = 0; i < wordListSize; i++) {
		SPL::float64 startt = myUtteranceWordsStartTimes[i];
		auto it = spkIndexMap.find(startt);
		if (it != spkIndexMap.end()) {
			rapidjson::SizeType idx = it->second;
			spkFromNew.push_back(spkFrom[idx]);
			spkSpkNew.push_back(spkSpk[idx]);
			spkCfdNew.push_back(spkCfd[idx]);
			usedSpkIndexes.insert(idx);
		} else {
			SPLAPPTRC(L_ERROR, traceIntro << "-->RE40 No speaker label at: " << startt << " insert -1. payload_: " << payload, "ws_receiver");
			spkFromNew.push_back(startt);
			spkSpkNew.push_back(-1);
			spkCfdNew.push_back(-1.0);
		}
	}
	// the skp updates list
	for (rapidjson::SizeType i = 0; i < spkSize; i++) {
		std::unordered_set<rapidjson::SizeType>::const_iterator got = usedSpkIndexes.find(i);
		if (got == usedSpkIndexes.end()) {
			spkUpdateIndexes.push_back(i);
		}
	}
}

template<typename TUPLE>
SPL::list<TUPLE> SpeakerProcessor::getUtteranceWordsSpeakerUpdates() const {
	SPL::list<TUPLE> destination;
	// spkUpdateIndexes.size() is zero in empty instance, so no invalid references are used
	for (size_t i = 0; i < spkUpdateIndexes.size(); ++i) {
		TUPLE theTuple;
		auto indx = spkUpdateIndexes[i];
		theTuple.set_startTime(spkFrom[indx]);
		theTuple.set_speaker(spkSpk[indx]);
		theTuple.set_confidence(spkCfd[indx]);
		destination.push_back(theTuple);
	}
	return destination;
}

KeywordProcessor::KeywordProcessor(const DecoderKeywordsResult::ResultMapType & keywordResults_) :
	isEmpty(false),
	keywordResults(keywordResults_) {
}

KeywordProcessor::KeywordProcessor() :
	// the references to the keywordResults is invalid in this instance
	// the reference is never used due to isEmpty == true
	isEmpty(true),
	keywordResults(DecoderKeywordsResult::ResultMapType()) {
}

template<typename T>
void KeywordProcessor::getKeywordsSpottingResults(SPL::map<SPL::rstring, SPL::list<T> > & destination) const {
	destination.clear();
	if (not isEmpty) {
		for (const auto & keyw : keywordResults) {
			const auto & keyword = keyw.first;
			SPL::list<T> i;
			for (const auto & emergence : keyw.second) {
				T t;
				t.set_start_time(emergence.start_time);
				t.set_end_time(emergence.end_time);
				t.set_confidence(emergence.confidence);
				i.push_back(t);
			}
			destination.insert(typename SPL::map<SPL::rstring, SPL::list<T> >::value_type(keyword, i));
		}
	}
}

template<>
void KeywordProcessor::getKeywordsSpottingResults<SPL::map<SPL::rstring, SPL::float64> >(SPL::map<SPL::rstring, SPL::list<SPL::map<SPL::rstring, SPL::float64> > > & destination) const {
	destination.clear();
	if (not isEmpty) {
		for (const auto & keyw : keywordResults) {
			const auto & keyword = keyw.first;
			SPL::list<SPL::map<SPL::rstring, SPL::float64> > i;
			for (const auto & emergence : keyw.second) {
				SPL::map<SPL::rstring, SPL::float64> t;
				t.insert(SPL::map<SPL::rstring, SPL::float64>::value_type("start_time", emergence.start_time));
				t.insert(SPL::map<SPL::rstring, SPL::float64>::value_type("set_end_time", emergence.end_time));
				t.insert(SPL::map<SPL::rstring, SPL::float64>::value_type("confidence", emergence.confidence));
				i.push_back(t);
			}
			destination.insert(SPL::map<SPL::rstring, SPL::list<SPL::map<SPL::rstring, SPL::float64>> >::value_type(keyword, i));
		}
	}
}

// Whenever our existing Websocket connection to the Watson STT service is closed,
// this callback method will be called from the websocketpp layer.
// Close will be called exactly once for every connection that open was called for. Close is not called for failed connections.
template<typename OP, typename OT>
void WatsonSTTImplReceiver<OP, OT>::on_close(client* c, websocketpp::connection_hdl hdl) {
	// In the lab tests, I noticed that occasionally a Websocket connection can get
	// closed right after an on_open event without actually receiving the "listening" response
	// in the on_message event from the Watson STT service. This condition clearly means
	// that this is not a normal connection closure. Instead, the connection attempt has failed.
	// In this case the connection does not reach the state listening and the connection attempt will be repeated in
	// the connect function of the sender thread

	// get information from ws lib
	client::connection_ptr con = c->get_con_from_hdl(hdl);
	int val = con->get_ec().value();
	std::string mess = con->get_ec().message();
	int closecode = con->get_remote_close_code();
	const std::string & closemess = con->get_remote_close_reason();

	WsState st = wsState.load();
	if (st == WsState::closing) {
		// a connection close was requested
		SPLAPPTRC(L_INFO, traceIntro <<
				"-->RE87 Connection closed. wsState=" << wsStateToString(st) << " ec.value=" << val << " ec.message=" << mess <<
				" remote_close_code=" << closecode << " remote_close_mesage=" << closemess, "ws_receiver");

	} else if (st == WsState::listening) {
		// This connection was fully established before but a error was not received in on_message
		// Sent the error tuple now
		std::stringstream errmess;
		errmess << traceIntro << "-->RE81 Websocket connection closed from listening state ec.value=" << val <<
				" ec.message=" << mess << " remote_close_code=" << closecode << " remote_close_mesage=" << closemess;
		sendErrorTuple(errmess.str());
		SPLAPPTRC(L_ERROR, errmess.str(), "ws_receiver");

		// send a end tuple and window punctuation if a conversation was ongoing
		OT * myRecentOTuple = recentOTuple.load(); // load atomic
		if (myRecentOTuple) {
			if (Config::isTranscriptionCompletedRequested)
				sendTranscriptionCompletedTuple(myRecentOTuple);
			splOperator.submit(SPL::Punctuation::WindowMarker, 0);
		}

	} else if (st == WsState::error) {
		// This connection was not fully established before.
		// This closure happened during an ongoing connection.
		// The error tuple was sent in on_message
		SPLAPPTRC(L_ERROR, traceIntro <<
				"-->RE81a Fully established Websocket connection closed with the Watson STT service."
				" wsState=" << wsStateToString(st) << " ec.value=" << val << " ec.message=" << mess <<
				" remote_close_code=" << closecode << " remote_close_mesage=" << closemess, "ws_receiver");

		// send a end tuple and window punctuation if a conversation was ongoing
		OT * myRecentOTuple = recentOTuple.load(); // load atomic
		if (myRecentOTuple) {
			if (Config::isTranscriptionCompletedRequested)
				sendTranscriptionCompletedTuple(myRecentOTuple);
			splOperator.submit(SPL::Punctuation::WindowMarker, 0);
		}

	} else {
		// c->get_alog().write(websocketpp::log::alevel::app, "Websocket connection closed with the Watson STT service.");
		SPLAPPTRC(L_ERROR, traceIntro <<
				"-->RE82 Partially established Websocket connection closed with the Watson STT service during an ongoing "
				"connection attempt. wsState=" << wsStateToString(st) << " ec.value=" << val << " ec.message=" << mess <<
				" remote_close_code=" << closecode << " remote_close_mesage=" << closemess, "ws_receiver");
	}

	recentOTuple.store(nullptr);
	setWsState(WsState::closed);
}

// When a Websocket connection handshake happens with the Watson STT service for enabling
// TLS security, this callback method will be called from the websocketpp layer.
template<typename OP, typename OT>
context_ptr WatsonSTTImplReceiver<OP, OT>::on_tls_init(client* c, websocketpp::connection_hdl) {
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
		SPLAPPTRC(L_ERROR, traceIntro << "-->" << e.what(), "ws_receiver");
	}

	return ctx;
}

// When a connection attempt to the Watson STT service fails, then this
// callback method will be called from the websocketpp layer.
// Either open or fail will be called for each connection. Never both.
template<typename OP, typename OT>
void WatsonSTTImplReceiver<OP, OT>::on_fail(client* c, websocketpp::connection_hdl hdl) {
	recentOTuple.store(nullptr);
	setWsState(WsState::failed);
	// c->get_alog().write(websocketpp::log::alevel::app, "Websocket connection to the Watson STT service failed.");
	client::connection_ptr con = c->get_con_from_hdl(hdl);
	int val = con->get_ec().value();
	std::string mess = con->get_ec().message();
	SPLAPPTRC(L_ERROR, traceIntro << "-->RE89 Websocket connection to the Watson STT service failed. ec.value=" << val <<
			" ec.message=" << mess, "ws_receiver");
}

template<typename OP, typename OT>
void WatsonSTTImplReceiver<OP, OT>::sendErrorTuple(const std::string & reason) {
	// send out the error with the non finalized output if any
	if (oTupleUsedForSubmission) {
		SPLAPPTRC(L_ERROR, traceIntro << "-->RE26 send a non finalized oTupleUsedForSubmission", "ws_receiver");
		splOperator.appendErrorAttribute(oTupleUsedForSubmission, reason);
		splOperator.submit(*oTupleUsedForSubmission, 0);
		splOperator.clearErrorAttribute(oTupleUsedForSubmission);
		oTupleUsedForSubmission = nullptr;
	} else {

		// send stand alone error tuple if there is a recent otuple
		OT * myRecentOTuple = recentOTuple.load(); // load atomic
		if (myRecentOTuple) {
			SPLAPPTRC(L_DEBUG, traceIntro << "-->RE27 append error attribute and send error tuple", "ws_receiver");
			incrementNFullAudioConversationsFailed();
			// clean the previous set values in the output tuple
			splOperator.setResultAttributes(myRecentOTuple, -1, false, -1.0, 0.0, 0.0, "", SPL::list<SPL::rstring>(),
				SPL::list<SPL::rstring>(), SPL::list<SPL::float64>(), SPL::list<SPL::float64>(), SPL::list<SPL::float64>(),
				SPL::list<SPL::list<SPL::rstring> >(), SPL::list<SPL::list<SPL::float64> >(),
				SPL::list<SPL::float64>(), SPL::list<SPL::float64>(),
				emptyKeywordProcessor);
			if (Config::identifySpeakers)
				splOperator.setSpeakerResultAttributes(myRecentOTuple, emptySpeakerResults);
			// set required output values
			splOperator.appendErrorAttribute(myRecentOTuple, reason);
			splOperator.submit(*myRecentOTuple, 0);
			splOperator.clearErrorAttribute(myRecentOTuple);
		} else {
			SPLAPPTRC(L_ERROR, traceIntro << "-->RE28 no recent output tuple: send no error tuple", "ws_receiver");
		}
	}
}

template<typename OP, typename OT>
void WatsonSTTImplReceiver<OP, OT>::sendTranscriptionCompletedTuple(OT * otuple) {
	SPLAPPTRC(L_DEBUG, traceIntro << "-->RE 30 send transcription completed tuple.", "ws_receiver");
	// clean the previous set values in the output tuple
	splOperator.setResultAttributes(otuple, -1, false, -1.0, 0.0, 0.0, "", SPL::list<SPL::rstring>(),
		SPL::list<SPL::rstring>(), SPL::list<SPL::float64>(), SPL::list<SPL::float64>(), SPL::list<SPL::float64>(),
		SPL::list<SPL::list<SPL::rstring> >(), SPL::list<SPL::list<SPL::float64> >(),
		SPL::list<SPL::float64>(), SPL::list<SPL::float64>(),
		emptyKeywordProcessor);
	if (Config::identifySpeakers)
		splOperator.setSpeakerResultAttributes(otuple, emptySpeakerResults);
	// set required output values
	splOperator.setTranscriptionCompleteAttribute(otuple);
	splOperator.submit(*otuple, 0);
}

/*template<typename OP, typename OT>
bool WatsonSTTImplReceiver<OP, OT>::on_ping(client* c, websocketpp::connection_hdl hdl, std::string mess) {
	SPLAPPTRC(L_DEBUG, traceIntro << "-->RE99 Websocket ping message=" << mess, "ws_receiver");
	return true;
}

template<typename OP, typename OT>
void WatsonSTTImplReceiver<OP, OT>::on_pong(client* c, websocketpp::connection_hdl hdl, std::string mess) {
	SPLAPPTRC(L_DEBUG, traceIntro << "-->RE99 Websocket pong message=" << mess, "ws_receiver");
}*/

template<typename OP, typename OT>
void WatsonSTTImplReceiver<OP, OT>::setWsState(WsState ws) {
	wsState.store(ws);
	wsConnectionStateMetric->setValueNoLock(static_cast<SPL::int64>(ws));
}

template<typename OP, typename OT>
void WatsonSTTImplReceiver<OP, OT>::incrementNWebsocketConnectionAttemptsCurrent() {
	++nWebsocketConnectionAttemptsCurrent;
	nWebsocketConnectionAttemptsCurrentMetric->setValueNoLock(nWebsocketConnectionAttemptsCurrent);
}

template<typename OP, typename OT>
void WatsonSTTImplReceiver<OP, OT>::incrementNFullAudioConversationsTranscribed() {
	++nFullAudioConversationsTranscribed;
	// Update the operator metric only if the user asked for a live update.
	if (sttLiveMetricsUpdateNeeded) {
		nFullAudioConversationsTranscribedMetric->setValueNoLock(nFullAudioConversationsTranscribed);
	}
}

template<typename OP, typename OT>
void WatsonSTTImplReceiver<OP, OT>::incrementNFullAudioConversationsFailed() {
	++nFullAudioConversationsFailed;
	// Update the operator metric only if the user asked for a live update.
	if (sttLiveMetricsUpdateNeeded) {
		nFullAudioConversationsFailedMetric->setValueNoLock(nFullAudioConversationsFailed);
	}
}

const char * wsStateToString(WsState ws) {
	switch (ws) {
		case WsState::idle:       return "idle";
		case WsState::start:      return "start";
		case WsState::connecting: return "connecting";
		case WsState::open:       return "open";
		case WsState::listening:  return "listening";
		case WsState::closing:    return "closing";
		case WsState::error:      return "error";
		case WsState::closed:     return "closed";
		case WsState::failed:     return "failed";
		case WsState::crashed:    return "crashed";
	}
	return "*****";
}

bool receiverHasStopped(WsState ws) {
	return (ws == WsState::closed) || (ws == WsState::failed) || (ws == WsState::crashed);
}
bool receiverHasTransientState(WsState ws) {
	return ws == WsState::start || ws == WsState::connecting || ws == WsState::open || ws == WsState::closing;
}

template<typename OP, typename OT>
const SpeakerProcessor WatsonSTTImplReceiver<OP, OT>::emptySpeakerResults;

template<typename OP, typename OT>
const KeywordProcessor WatsonSTTImplReceiver<OP, OT>::emptyKeywordProcessor;

}}}}

#endif /* COM_IBM_STREAMS_STTGATEWAY_WATSONSTTIMPLRECEIVER_HPP_ */
