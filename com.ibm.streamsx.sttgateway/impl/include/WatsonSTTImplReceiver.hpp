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
#include <atomic>

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

#include <WatsonSTTConfig.hpp>
//#include <SttGatewayResource.h>

namespace com { namespace ibm { namespace streams { namespace sttgateway {

// Websocket related type definitions.
typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
// Pull out the type of messages sent by our config
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;

enum class WsState : char {
	idle,       // initial idle state
	start,      // start a new connection
	connecting, // trying to connect to stt
	open,       // connection is opened
	listening,  // listening received
	error,      // error received during transcription
	closed,     // connection has closed
	failed,     // connection has failed
	crashed     // Error was caught in ws_init tread
};
// helper function to make a pretty print
const char * wsStateToString(WsState ws);
// helper function to determine whether the receiver has reached an inactive state
bool receiverHasStopped(WsState ws);

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
	bool transcriptionFinalized;

	// This values are set from receiver thread when state is connecting
	// the sender thread requires the values but should not use them during connecting state
	client *wsClient;
	websocketpp::connection_hdl wsHandle;

	// The tuple with assignments from the input port
	// to be used if transcription results or error indications have to be sent
	// the sender thread guarantees that here is an valid value until transcriptionFinalized is flagged
	// this otuple may change during a transcription when the input receives new input tuples for the
	// current transcription. The previous value remains valid until transcriptionFinalized is flagged.
	std::atomic<OT *> recentOTuple;
	// when the on_message method is about to send something, this member is used to store the
	// output tuple pointer. The member is reset, after the tuple was submitted
	OT * oTupleUsedForSubmission;

	// the access token for the sender thread
	// the value is copied from the sender thread before makeNewWebsocketConnection has been flagged
	std::string accessToken;

private:
	std::atomic<SPL::int64> nWebsocketConnectionAttemptsCurrent;
	std::atomic<SPL::int64> nFullAudioConversationsTranscribed;
	std::atomic<SPL::int64> nFullAudioConversationsFailed;

	//StatusOfAudioDataTransmission statusOfAudioDataTransmissionToSTT;

private:
	// values used from sender thread only
	std::string transcriptionResult;
	bool sttResultTupleWaitingToBeSent;

	SPL::list<SPL::int32> utteranceWordsSpeakers;
	SPL::list<SPL::float64> utteranceWordsSpeakersConfidences;
	SPL::list<SPL::float64> utteranceWordsStartTimes;

private:
	// Custom metrics for this operator.
	SPL::Metric * const nWebsocketConnectionAttemptsCurrentMetric;
	SPL::Metric * const nFullAudioConversationsTranscribedMetric;
	SPL::Metric * const nFullAudioConversationsFailedMetric;
	SPL::Metric * const wsConnectionStateMetric;
protected:
	// These are the output attribute assignment functions
	inline SPL::list<SPL::float64> getUtteranceWordsStartTimes() { return(utteranceWordsStartTimes); }
	inline SPL::list<SPL::int32> getUtteranceWordsSpeakers() { return(utteranceWordsSpeakers); }
	inline SPL::list<SPL::float64> getUtteranceWordsSpeakersConfidences() { return(utteranceWordsSpeakersConfidences); }
	/*SPL::map<SPL::rstring, SPL::list<SPL::map<SPL::rstring, SPL::float64>>>
		getKeywordsSpottingResults(SPL::map<SPL::rstring,
		SPL::list<SPL::map<SPL::rstring, SPL::float64>>> const & keywordsSpottingResults) { return(keywordsSpottingResults); }*/

	// Helper functions
	void setWsState(WsState ws);
	inline void incrementNWebsocketConnectionAttemptsCurrent();
	inline void incrementNFullAudioConversationsTranscribed();
	inline void incrementNFullAudioConversationsFailed();
	inline SPL::float64 getNWebsocketConnectionAttemptsCurrent() { return nWebsocketConnectionAttemptsCurrent.load(); };
};

template<typename OP, typename OT>
WatsonSTTImplReceiver<OP, OT>::WatsonSTTImplReceiver(OP & splOperator_,Config config_)
:
		Config(config_),
		splOperator(splOperator_),

		wsState{WsState::idle},
		transcriptionFinalized(true),
		wsClient(nullptr),
		wsHandle{},
		recentOTuple{},
		oTupleUsedForSubmission{},

		accessToken{},

		nWebsocketConnectionAttemptsCurrent{0},
		nFullAudioConversationsTranscribed{0},
		nFullAudioConversationsFailed{0},
		transcriptionResult{},
		sttResultTupleWaitingToBeSent{false},
		utteranceWordsSpeakers{},
		utteranceWordsSpeakersConfidences{},
		utteranceWordsStartTimes{},

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

			// Create a connection to the given URI and queue it for connection once
			// the event loop starts
			SPLAPPTRC(L_DEBUG, traceIntro << "-->RE3 (after call back setup)", "ws_receiver");
			websocketpp::lib::error_code ec;
			// https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-basic-request#using-the-websocket-interface
			client::connection_ptr con = wsClient->get_connection(uri, ec);
			SPLAPPTRC(L_DEBUG, traceIntro << "-->RE4 (after get_connection) ec.value=" << ec.value(), "ws_receiver");
			if (ec)
				throw ec;

			wsClient->connect(con);
			// A new Websocket connection has just been made. Reset this flag.
			SPLAPPTRC(L_DEBUG, traceIntro << "-->RE5 (after connect)", "ws_receiver");

			// Start the ASIO io_service run loop
			wsClient->run();
			SPLAPPTRC(L_INFO, traceIntro << "-->RE10 (after run)", "ws_receiver");
		} catch (const std::exception & e) {
			SPLAPPTRC(L_ERROR, traceIntro << "-->RE11 std::exception: " << e.what(), "ws_receiver");
			setWsState(WsState::error);
			//SPL::Functions::Utility::abort(__FILE__, __LINE__);
		} catch (const websocketpp::lib::error_code & e) {
			//websocketpp::lib::error_code is a class -> catching by reference makes sense
			SPLAPPTRC(L_ERROR, traceIntro << "-->RE12 websocketpp::lib::error_code: " << e.message(), "ws_receiver");
			setWsState(WsState::error);
			//SPL::Functions::Utility::abort(__FILE__, __LINE__);
		} catch (...) {
			SPLAPPTRC(L_ERROR, traceIntro << "-->RE13 Other exception in WatsonSTT operator's Websocket initializtion.", "ws_receiver");
			setWsState(WsState::error);
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
			"-->X53 A recognition request start message was sent to the Watson STT service:" <<
			msg, "ws_receiver");
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
void WatsonSTTImplReceiver<OP, OT>::on_message(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
	// Short alias for this namespace
	namespace pt = boost::property_tree;

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
		throw std::runtime_error("Unexpected entryState in ws on_message; state: " + std::string(wsStateToString(entryState)));

	// Local state variables
	bool fullTranscriptionCompleted_ = false;	// Is set when a second listening event is received
	bool transcriptionResultAvailableForParsing_ = false;
	bool wsConnectionEstablished_ = entryState == WsState::listening;

	const std::string & payload_ = msg->get_payload();
	SPLAPPTRC(L_TRACE, traceIntro << "-->on_message payload_: " << payload_, "ws_receiver");
	const bool stateListeningFound_ = payload_.find("\"state\": \"listening\"") != std::string::npos;
	// STT error will have the following message format.
	// {"error": "unable to transcode data stream audio/wav -> audio/x-float-array "}
	const bool sttErrorFound_ = payload_.find("\"error\": \"") != std::string::npos;
	if (sttErrorFound_ && stateListeningFound_)
		throw std::runtime_error("Unexpected condition in ws response: sttErrorFound_ && stateListeningFound_; payload: " + payload_);

	std::string sttErrorString_ = "";
	if (sttErrorFound_) {

		sttErrorString_ = payload_;
		setWsState(WsState::error);

		if (sttJsonResponseDebugging == true) {
			std::cout << traceIntro << "-->X3 STT error message=" << sttErrorString_ << std::endl;
		}
		SPLAPPTRC(L_ERROR, traceIntro << "-->X3 STT error message=" << sttErrorString_, "ws_receiver");

	} else {
		// no error
		if (entryState != WsState::listening) {
			if (stateListeningFound_) {
				// This is the "listening" response from the STT service for the
				// new recognition request that was initiated in the on_open method above.
				// This response is sent only once for every new Websocket connection request made and
				// not for every new transcription request made.
				setWsState(WsState::listening);
				wsConnectionEstablished_ = true;
				nWebsocketConnectionAttemptsCurrent = 0;
				nWebsocketConnectionAttemptsCurrentMetric->setValueNoLock(nWebsocketConnectionAttemptsCurrent);

				if (sttJsonResponseDebugging == true) {
					std::cout << traceIntro << "-->X0 STT response payload=" << payload_ << std::endl;
				}

				SPLAPPTRC(L_DEBUG, traceIntro <<
					"-->X0 state listening reached. Websocket connection established with the Watson STT service.",
					"ws_receiver");
				return;
			} else {
				SPLAPPTRC(L_ERROR, traceIntro << "-->Unexpected on_message not in state listening and not "
						"listening found! Ignore message" << payload_, "ws_receiver");
				//TODO: return??
			}
		} else { //state listening
			if (not stateListeningFound_) {
				// This must be the response for our audio transcription request.
				// Keep accumulating the transcription result.
				transcriptionResult += payload_;

				if (sttJsonResponseDebugging == true) {
					std::cout << traceIntro << "-->X1 STT response payload=" << payload_ << std::endl;
				}

				SPLAPPTRC(L_DEBUG, traceIntro <<
						"-->X1 Websocket connection established - no listening state.", "ws_receiver");

				if (sttResultMode == 1 || sttResultMode == 2) {
					// We can parse this partial or completed
					// utterance result now since the user has asked for it.
					transcriptionResultAvailableForParsing_ = true;
				} else {
					// User didn't ask for the partial or completed utterance result.
					// We will parse the full transcription result later when it is fully completed.
					return;
				}
			} else { // (stateListeningFound_ && wsConnectionEstablished) {
				// This is the "listening" response from the STT service for the
				// transcription completion for the audio data that was sent earlier.
				// This response also indicates that the STT service is ready to do a new transcription.
				wsConnectionEstablished_ = true;
				fullTranscriptionCompleted_ = true;

				if (sttJsonResponseDebugging == true) {
					std::cout << traceIntro << "-->X2 STT task completion message=" << payload_ << std::endl;
				}

				SPLAPPTRC(L_DEBUG, traceIntro <<
					"-->X2 Websocket connection established - transcription completion.", "ws_receiver");

				if (sttResultMode == 3) {
					// Neither partial nor completed utterance results were parsed earlier.
					// So, parse the full transcription result now.
					transcriptionResultAvailableForParsing_ = true;
				}
			}
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
	SPL::float64 confidence_ = 0.0;
	std::string fullTranscriptionText_ = "";
	SPL::float64 cumulativeConfidenceForFullTranscription_ = 0.0;
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
	//SPL::AutoMutex autoMutex(stateMutex);
	// This is always an single thread and this function does touch recentOTuple

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
		OT * myRecentOTuple = recentOTuple.load(); // load atomic
		if (sttErrorFound_ && wsConnectionEstablished_ && myRecentOTuple) {
			// Parse the STT error string.
			ss << sttErrorString_;
			// Reset the trascriptionResult member variable.
			transcriptionResult = "";

			// Load the json data in the boost ptree
			pt::read_json(ss, root);
			const std::string sttErrorMsg_ = root.get<std::string>("error");

			// Set the STT error message attribute via the corresponding output function.
			if (not oTupleUsedForSubmission)
				oTupleUsedForSubmission = myRecentOTuple;
			splOperator.appendErrorAttribute(oTupleUsedForSubmission, sttErrorMsg_);

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
				if (oTupleUsedForSubmission) {
					splOperator.submit(*oTupleUsedForSubmission, 0);
					oTupleUsedForSubmission = nullptr;
				} else {
					throw std::runtime_error("on_message oTupleUsedForSubmission is null");
				}

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
					if (not oTupleUsedForSubmission)
						oTupleUsedForSubmission = recentOTuple;
					if (idx1 > 0) {
						std::string const * utteranceTextPtr_ = &utteranceText_;
						if (sttResultMode == complete)
							utteranceTextPtr_ = &fullTranscriptionText_;
						splOperator.setResultAttributes(
								oTupleUsedForSubmission,
								utteranceNumber_ + 1,
								*utteranceTextPtr_,
								final_,
								confidence_,
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
						splOperator.setSpeakerResultAttributes(oTupleUsedForSubmission);
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
			if (not oTupleUsedForSubmission)
				oTupleUsedForSubmission = recentOTuple;
			splOperator.setTranscriptionCompleteAttribute(oTupleUsedForSubmission);
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
		if (wsConnectionEstablished_ && recentOTuple) {
			// Send either the very last tuple with transcriptionCompleted set to true or
			// with the STT error message set.
			// Dereference the oTuple object from the object pointer and send it.
			if (oTupleUsedForSubmission) {
				splOperator.submit(*recentOTuple, 0);
				oTupleUsedForSubmission = nullptr;
			} else {
				throw std::runtime_error("on_message oTupleUsedForSubmission is null II");
			}

			// Count conversations
			if (not sttErrorFound_) {
				incrementNFullAudioConversationsTranscribed();
				SPLAPPTRC(L_INFO, "Successful transcription", "STT_Result_Processing");
			} else {
				incrementNFullAudioConversationsFailed();
				SPLAPPTRC(L_ERROR, "Error transcription", "STT_Result_Processing");
			}

			if (sttJsonResponseDebugging == true) {
				std::string tempString = "transcription completion.";

				if (sttErrorFound_) {
					tempString = "STT error.";
				}

				std::cout << traceIntro <<
					"-->X52b At the tuple submission point for reporting " <<
					tempString << ", Total audio conversations transcribed=" <<
					nFullAudioConversationsTranscribed << std::endl;
			}

		} // End of if (wsConnectionEstablished && oTupleList.size() > 0)

		if (not wsConnectionEstablished_ && sttErrorFound_) {
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
		//statusOfAudioDataTransmissionToSTT = NO_AUDIO_DATA_SENT_TO_STT;
		// end criterion
		if (fullTranscriptionCompleted_) {
				recentOTuple.store(nullptr);
				splOperator.submit(SPL::Punctuation::WindowMarker, 0);
				transcriptionFinalized = true;
		}
	} // End of if (fullTranscriptionCompleted_ || sttErrorFound_)
} // End of the on_message method.

// Whenever our existing Websocket connection to the Watson STT service is closed,
// this callback method will be called from the websocketpp layer.
// Close will be called exactly once for every connection that open was called for. Close is not called for failed connections.
template<typename OP, typename OT>
void WatsonSTTImplReceiver<OP, OT>::on_close(client* c, websocketpp::connection_hdl hdl) {
	// In the lab tests, I noticed that occasionally a Websocket connection can get
	// closed right after an on_open event without actually receiving the "listening" response
	// in the on_message event from the Watson STT service. This condition clearly means
	// that this is not a normal connection closure. Instead, the connection attempt has failed.
	// We must flag this as a connection error so that a connection retry attempt
	// can be triggered inside the ws_audio_blob_sender method.
	recentOTuple.store(nullptr);
	WsState st = wsState.load();
	if ((st != WsState::listening) && (st != WsState::error)) {
		// This connection was not fully established before.
		// This closure happened during an ongoing connection attempt.
		// Let us flag this as a connection error.
		SPLAPPTRC(L_ERROR, traceIntro <<
				"-->Partially established Websocket connection closed with the Watson STT service during an ongoing "
				"connection attempt. wsState=" << wsStateToString(st), "ws_receiver");
	} else {
		// c->get_alog().write(websocketpp::log::alevel::app, "Websocket connection closed with the Watson STT service.");
		SPLAPPTRC(L_ERROR, traceIntro <<
				"-->Fully established Websocket connection closed with the Watson STT service."
				" wsState=" << wsStateToString(st), "ws_receiver");
	}
	setWsState(WsState::closed);
}

// When a Websocket connection handshake happens with the Watson STT serice for enabling
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
	SPLAPPTRC(L_ERROR, traceIntro << "-->Websocket connection to the Watson STT service failed.", "ws_receiver");
}

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
}}}}

#endif /* COM_IBM_STREAMS_STTGATEWAY_WATSONSTTIMPLRECEIVER_HPP_ */
