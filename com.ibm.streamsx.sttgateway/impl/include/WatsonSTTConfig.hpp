/*
 * WatsonSTTConfig.hpp
 *
 * Licensed Materials - Property of IBM
 * Copyright IBM Corp. 2019, 2021
 *
 *  Created on:  Jan 14, 2020
 *  Modified on: Sep 12, 2021
 *  Author(s): Senthil, joergboe
 */

#ifndef COM_IBM_STREAMS_STTGATEWAY_WATSONSTTCONFIG_HPP_
#define COM_IBM_STREAMS_STTGATEWAY_WATSONSTTCONFIG_HPP_

namespace com { namespace ibm { namespace streams { namespace sttgateway {

struct WatsonSTTConfig {
	enum SttResultMode { partial = 1, final, complete };
	const std::string operatorPhysicalName;
	const SPL::int32 udpChannelNumber;
	const std::string traceIntro;

	//Configuration values
	const bool websocketLoggingNeeded;
	const SPL::float64 cpuYieldTimeInAudioSenderThread;
	const SPL::float64 maxConnectionRetryDelay;
	const bool sttLiveMetricsUpdateNeeded;
	const std::string uri;
	const std::string baseLanguageModel;
	const std::string contentType;
	SttResultMode sttOutputResultMode;
	bool  nonFinalUtterancesNeeded;
	const bool sttRequestLogging;
	const std::string baseModelVersion;
	const std::string customizationId;
	SPL::float64 customizationWeight;
	const std::string acousticCustomizationId;
	const bool filterProfanity;
	const SPL::int32 maxUtteranceAlternatives;
	const SPL::float64 wordAlternativesThreshold;
	const bool wordConfidenceNeeded;
	const bool wordTimestampNeeded;
	const bool identifySpeakers;
	const bool speakerUpdatesNeeded;
	const bool smartFormattingNeeded;
	const bool redactionNeeded;
	SPL::float64 keywordsSpottingThreshold;
	const SPL::list<SPL::rstring> keywordsToBeSpotted;
	const bool isTranscriptionCompletedRequested;
	SPL::float64 speechDetectorSensitivity;
	SPL::float64 backgroundAudioSuppression;
	SPL::float64 characterInsertionBias;

	// Some definitions
	//This time becomes effective, when the connectionAttemptsThreshold limit is exceeded
	static constexpr SPL::float64 receiverWaitTimeWhenIdle = 0.2;
	static constexpr SPL::float64 senderWaitTimeForTranscriptionFinalization = 1.0;
	static constexpr SPL::float64 senderWaitTimeForFinalReceiverState = 0.5;
	static constexpr SPL::float64 senderWaitTimeEmptyAccessToken = 10.0;
	//static constexpr SPL::float64 senderPingPeriod = 5.0;
};

}}}}
#endif /* COM_IBM_STREAMS_STTGATEWAY_WATSONSTTCONFIG_HPP_ */
