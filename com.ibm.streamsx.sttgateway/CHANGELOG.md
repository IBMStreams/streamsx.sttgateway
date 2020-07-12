# Changes

## v2.2.3
* Jul/12/2020
* Allow only TLS v1.2 in the IBMVoiceGatewaySource operator.
* Added a feature to query the call start date time string via an output function in the IBMVoiceGatewaySource operator.
* Added a feature so that both the agent and caller phone numbers can be queried anytime via output functions irrespective of who is talking during a voice call session.
* Simplified the TLS connection clean-up logic in the IBMVoiceGatewaySource operator.
* Added new operator metrics to display the TLS and non-TLS ports being used by the IBMVoiceGatewaySource operator.
* Minor changes in the AudioFile and AudioRaw examples to change the sink file name and start the file sequence from 1 instead of 0.
* Added the very important live call recording and call replay features to the Voice Gateway related examples.
* Replaced the use of the HTTPPost operator from the inet tookkit with HttpPost from the websocket toolkit inside the Voice Gateway related examples. This is in preparaion to move to WebSocket based transcription result exchange in the future to handle large number of concurrent calls.
* Changed the VoiceDataSimulator test client application to negotiate only TLS v1.2 connections with the remote server.
* In the Voice Gateway related examples, added a call replay test data generator script which will help in the scale testing of transcribing many concurrent calls.
* Added a colorful architecture diagram for this toolkit.
* Changed the stt_results_http_receiver example to use the WebSocketSource operator to receive the transcription results via both HTTP and WebSocket.

## v2.2.2
* [#50](https://github.com/IBMStreams/streamsx.sttgateway/issues/50) Library update: boost 1.73.0; websocketpp 0.8.2
* [#51](https://github.com/IBMStreams/streamsx.sttgateway/issues/51) Remove Compiler Warning in WatsonSTT Operator
* [#52](https://github.com/IBMStreams/streamsx.sttgateway/issues/52) Remove Compiler Warning in include/boost/bind.hpp
* [#53](https://github.com/IBMStreams/streamsx.sttgateway/issues/53) Test updates
* [#54](https://github.com/IBMStreams/streamsx.sttgateway/issues/54) Avoid boost cmake files in lib directory
* Resolved: [#56](https://github.com/IBMStreams/streamsx.sttgateway/issues/56) The Eclipse CDT compiler settings are sporadically removed
* Documentation updates

## v2.2.1
* Include the operator *.pm files into the toolkit bundle

## v2.2.0
* WatsonSTT: Enhancement: Output Speaker Label Updates COF getUtteranceWordsSpeakerUpdates (issue #38)
* WatsonSTT: Use useful default paramters: maxUtteranceAlternatives, wordAlternativesThreshold and keywordsSpottingThreshold (issue #40)
* WatsonSTT: Make error logs more verbose in case of connection failure
* WatsonSTT: Output should use default outputs (issue #43)
* WatsonSTT: Keyword spotting results are now available as typle type COF getKeywordsSpottingResults
* WatsonSTT: Relax output and parameter checks, make errors to warnings: Output schema may be used in all sstResult modes
* WatsonSTT: Correction: Sporadicaly Transcription Failures (issue #44)

## v2.1.2
* WatsonSTT: Correction issue #39: There should be no output tuples if non final utterances are received and parameter nonFinalUtterancesNeeded is false.

## v2.1.1
* Download boost from other repo
* Correction: Operator WatsonSTT remove previous speaker label attributes from output tuple before utterance attributes

## v2.1.0
* WatsonSTT: Reimplemented JSON decoder of the receiver task contains corrections
* WatsonSTT: The output functions: getWordAlternatives, getWordAlternativesConfidences,
getWordAlternativesStartTimes, getWordAlternativesEndTimes, getUtteranceWords, getUtteranceWordsConfidences, getUtteranceWordsEndTimes,
 getUtteranceStartTime, getUtteranceWordsSpeakers, getUtteranceWordsSpeakersConfidences and getKeywordsSpottingResults are also available in sttResultMode complete
* WatsonSTT: Speaker label consistency check. Speaker label statrt times are checked against the start times of the utterance words list
* WatsonSTT: The results are send out immediately. Send the final tuple of a conversation only if function isTranscriptionCompleted was requested
* WatsonSTT: remove paramter sttJsonResponseDebugging
* Restore samples VoiceGatewayToStreamsToWatsonSTT, VoiceGatewayToStreamsToWatsonS2T and stt_results_http_receiver

## v2.0.0:
* Many changes in implementation and many corrections
* WatsonSTT move implementation to implementation class
* Samples: print results to console with time stamp
* Samples: added simplified samples AudioFileWatsonSTT and AudioRawWatsonSTT
* Samples: added AccessTokenGenerator
* Samples: WatsonSTTRaw avoid SIGFPE
* Demos: Adapt demos to new S2T toolkit release 3.6.0
* Corrects metrics for operator WatsonSTT and IBMVoiceGatewaySource
* New metrics for operator WatsonSTT: nWebsocketConnectionAttemptsFailed, nWebsocketConnectionAttemptsCurrent,
  wsConnectionState, nAudioBytesSend and nAudioBytesSend
* Improved parameter description
* Complete initialization of state variables
* Functional logging for Operator WatsonSTT
* Graceful connection close on operator shutdown
* Use Streams Operator threads instead of boost-threads in Operator WatsonSTT
* Improvements in re-Connection handling of WatsonSTT operator, new parameter maxConnectionRetryDelay
* Logging of failed filenames
* Remove Potential Ressource Problem see #29, Sender works now in input port process thread
* WatsonSTT: generate back pressure if no access token is available
* WatsonSTT emit window marker on conversation end; Delay final marker until conversation end
* WatsonSTT: make parameter sttResultMode to custom literal; invalidate functions and params in sttResultMode complete
* WatsonSTT: new parameter nonFinalUtterancesNeeded
* WatsonSTT: Use empty blob or Window punctuation marker as conversation end identifier
* New implementation of New IAMAccessTokenGenerator with many new features
* Support application configuration for connection parameters and credentials
* Move external libraries to toolkit directory folder include and lib
* Toolkit internationalization
* Add complete test suite

## v1.0.6:
* Nov/14/2019
* Added a new ipv6Available parameter to support both the dual stack (ipv4/ipv6) and the single stack (ipv4 only) environments.
* Added new logic to assign the caller and agent telephone number attributes based on the vgwIsCaller metadata field.
* Fixed a typo in an operator parameter name.
* Added new logic in the examples to send the STT results to a file as well as to an HTTP endpoint.
* Added a new example VoiceGatewayToStreamsToWatsonS2T to showcase an architectural design pattern where all the three IBM products (IBM Voice Gateway, IBM Streams and Watson S2T engine embedded in a Streams operator) can come together to work seamlessly.

## v1.0.5:
* Oct/23/2019
* Added code for the new IBMVoiceGatewaySource operator.
* Added a new example VoiceGatewayToStreamsToWatsonSTT to showcase an architectural design pattern where all the three IBM products (IBM Voice Gateway, IBM Streams and IBM Watson Speech To Text) can come together to work seamlessly.

## V1.0.4:
* Sep/05/2019
* Made changes for the user to provide either the API key for public cloud or the access token for IBM Cloud Pak for Data.
* Made the necessary changes to use either the STT service on public cloud or the STT service on the IBM Cloud Pak for Data (CP4D).

## v1.0.3:
* Jun/12/2019
* Replaced the use of the auth token with the IAM access token.
* Added a utility composite named IAMAccessTokenGenerator to generate/refresh an IAM access token. This utility composite can be invoked within the IBM Streams applications that make use of this toolkit to perform speech to text transcription.

## v1.0.2:
* Sep/24/2018
* Added a .gitkeep file in the data, impl, include and lib empty folders.

## v1.0.1:
* Sep/21/2018
* In the internal threads of the WatsonSTT operator code, the CPU yield time during idleness and in between the Websocket connection attempts was increased from a few milliseconds to 1 second.
* New operator parameter sttLiveMetricsUpdateNeeded was added to give the users a way to turn the custom operator metrics reporting on and off in the time critical path of audio transcription.
* Use of the internal custom metrics update API was changed from setValue to setValueNoLock. 
* Corresponding documentation refinements were also done.

## v1.0.0:
* Sep/17/2018
* Very first release of this toolkit that was tested to support all the major features available in the IBM Watson Speech To Text (STT) cloud service.
