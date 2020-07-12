---
title: "Operator Design"
permalink: /docs/user/OperatorDesign/
excerpt: "Describes the design of the Message Hub toolkit operators."
last_modified_at: 2020-07-07T08:47:48+01:00
redirect_from:
   - /theme-setup/
sidebar:
   nav: "userdocs"
---
{% include toc %}
{%include editme %}

This IBM Watson STT Gateway toolkit contains the following operators to enable the Speech To Text feature inside the Streams applications via the IBM Watson STT service on public cloud or on the Cloud Pak for Data CP4D i.e. private cloud.

1. IBMVoiceGatewaySource
2. WatsonSTT

In a Streams application, these two operators can either be used together or independent of each other. In a real-time speech analytics scenario for transcribing live voice calls into text, both of these operators can be used as part of a Streams application graph. In a batch mode operation to convert a set of prerecorded audio files stored in a file system directory into text, WatsonSTT operator can be used on its own in a Streams application graph. So, your application requirements will determine whether you will need only one or both of those operators. In the section below, you will see a detailed explanation of both the operators.

*******************************

* **IBMVoiceGatewaySource** - this is a Websocket server based C++ source operator that can receive the speech data in binary format from many concurrent live voice calls happening between different pairs of speakers e-g: customers and call center agents.

Main goal of this operator is to receive speech data from the iBM Voice Gateway. This operator internally runs a Websocket server that can accept persistent bidirectional client connections made by the IBM Voice Gateway and receive the speech data from the beginning to the end of any given voice call. It then emits chunks of the speech conversations received from the IBM Voice Gateway into output tuples to be consumed by the downstream operators for speech to text transcription. This operator is designed to work with the IBM Voice Gateway v1.0.3.0 and higher. In order to make this operator work well with the IBM Voice Gateway, there must be certain configuration that needs to be done in the IBM Voice Gateway. It includes enabling the voice call metadata transfer to the IBM Streams application, pointing to the Websocket URL endpoint of the IBM Streams application, creating a TLS/SSL certificate and pointing it in both the IBM Voice Gateway and the IBM Streams application etc. These steps must be performed before this source operator can be used in the context of an IBM Streams application. For clear instructions about this required configuration, please refer to a previous chapter titled "Toolkit Overview [Technical]" and focus on step 6 of one of its sections titled "Requirements for this toolkit".

The IBMVoiceGatewaySource operator is designed to ingest speech data from the IBM Voice Gateway product. This speech data is ingested in binary format from the IBM Voice Gateway into this operator via the Websocket interface. Such speech data arrives here in multiple fragments directly from a live voice call. This operator is capable of receiving speech data from multiple calls that can all happen at the very same time between different pairs of speakers. For every voice call it handles in real-time, the IBM Voice Gateway product will open two Websocket connections into this operator and start sending the live speech data on both of those connections. One of those connections will carry the speech data of the agent and the other connection will carry the speech data of the customer. This operator will keep sending the audio chunks received on those two Websocket connections via its output stream for consumption by the downstream operators. At the end of the any given call, IBM Voice Gateway will close the two WebSocket connections it opened into this operator. This operator has a second output port that produces periodic output tuples to give an indication about the end of a specific speaker (i.e. channel) in a voice call that was in progress moments ago for the given IBM Voice Gateway session id. Downstream operators can make use of this "End Of Voice Call" signal as they see fit.

### IBMVoiceGatewaySource operator parameters
Following are the parameters accepted by the IBMVoiceGatewaySource operator. Some parameters are mandatory with user-provided values and others are optional with default values assigned within the C++ operator logic.

| Parameter Name | Type | Default | Description |
| --- | --- | --- | --- |
| tlsPort | `uint32` | `443` | This parameter specifies the WebSocket TLS port number. |
| certificateFileName | `rstring` | `etc/ws-server.pem present inside the Streams application` | This parameter specifies the full path of the WebSocket server PEM certificate file name. |
| nonTlsEndpointNeeded | `boolean` | `false` | This parameter specifies whether a WebSocket (plain) non-TLS endpoint is needed. |
| nonTlsPort | `uint32` | `80` | This parameter specifies the WebSocket (plain) non-TLS port number. |
| initDelay | `float64` | `0.0` | This parameter specifies a one time delay in seconds for which this source operator should wait before start generating its first tuple. |
| vgwLiveMetricsUpdateNeeded | `boolean` | `true` | This parameter specifies whether live update for this operator's custom metrics is needed. |
| websocketLoggingNeeded | `boolean` | `false` | This parameter specifies whether logging is needed from the WebSocket library. |
| vgwSessionLoggingNeeded | `boolean` | `false` | This parameter specifies whether logging is needed when the IBM Voice Gateway session is in progress with this operator. |
| vgwStaleSessionPurgeInterval | `uint32` | `10800` | This parameter specifies periodic time interval in seconds during which any stale Voice Gateway sessions should be purged to free up memory usage. |
| ipv6Available | `boolean` | `true` | This parameter indicates whether the ipv6 protocol stack is available in the Linux machine where the IBMVoiceGatewaySource operator is running. |

### IBMVoiceGatewaySource operator's custom output functions
Following are the custom output functions supported by the IBMVoiceGatewaySource operator. These functions can be called as needed within the output clause of this operator's SPL invocation code.

| Output function name | Description |
| --- | --- |
| `rstring getIBMVoiceGatewaySessionId()` | Returns an rstring value indicating the IBM Voice Gateway session id that corresponds to the current output tuple. |
| `boolean isCustomerSpeechData()` | Returns a boolean value to indicate if this is a customer's speech data or not. |
| `int32 getTupleCnt()` | Returns an int32 value indicating the total number of output tuples emitted so far for the given channel in a IBM Voice Gateway session id. |
| `int32 getTotalSpeechDataBytesReceived()` | Returns an int32 value indicating the total number of speech data bytes received so far for the given channel in a IBM Voice Gateway session id. |
| `int32 getVoiceChannelNumber()` | Returns an int32 value indicating the voice channel number in which the speech data bytes were received for a IBM Voice Gateway session id. |
| `rstring getCallerPhoneNumber()` | Returns an rstring value with details about the caller's phone number. |
| `rstring getAgentPhoneNumber()` | Returns an rstring value with details about the agent's phone number. |
| `rstring getCallStartDateTime()` | Returns an rstring value with the call start date time i.e. system clock time. |


*******************************

 * **WatsonSTT** - this is a Websocket client based C++ analytic operator that can perform the Speech To Text transcription.

Main goal of this operator is to make the task of the Speech To Text transcription as simple as possible for the user. With that goal in mind, this operator provides the following set of core features. High level description of such features is provided below and a detailed explanation is given in the next section titled "Operator Usage Patterns".

 * It allows the user to specify any supported base language model and the audio content type at the time of launching the application.

 * It allows the user to select one of the three supported STT result modes to define the granularity of the transcription result.

 * Apart from the transcription result, it allows the user to enable speaker identification, word alternatives a.k.a. Confusion Networks, utterance alternatives a.k.a. n-best hypotheses, utterance words, utterance words confidences, utterance words timestamps etc.

 * It also allows the user to specify language model and acoustic model customization ids along with customization weight to augment the STT service's use of its chosen base model along with user's own custom models to improve the transcription accuracy.

 * To provide scalability, it lets the user to specify many WatsonSTT operator instances to work in parallel for transcribing concurrent audio conversations in real time (via network packets) or in batch (via stored files).

 * WatsonSTT operator instances running in parallel can be fused as needed to optimally use the available number of CPU cores. In general, fusion is fine as long as there is a total of 10 instances of this operator. Above that limit, it is better to avoid fusion for the correct functioning of the application logic.

 * In addition to letting the user choose the core STT capabilities, it also allows the user to take advantage of the other value added features such as profanity filtering, smart formatting, keywords spotting etc.


### WatsonSTT operator parameters
Following are the parameters accepted by the WatsonSTT operator. Some parameters are mandatory with user-provided values and others are optional with default values assigned within the C++ operator logic.

| Parameter Name | Type | Default | Description |
| --- | --- | --- | --- |
| uri | `rstring` | `User must provide this value. No default.` | This parameter specifies the Watson STT web socket service URI. |
| baseLanguageModel | `rstring` | `User must provide this value. No default.` | This parameter specifies the name of the Watson STT base language model that should be used. |
| contentType | `rstring` | `audio/wav` | This parameter specifies the content type to be used for transcription. |
| sttResultMode | `custom literal` | `partial or complete` | This parameter specifies what type of STT result is needed: partial utterances or a complete utterance to get the full text after transcribing the entire audio. |
| nonFinalUtterancesNeeded | `boolean` | `false` | If `sttResultMode` equals `partial` this parameter controls the output of non final utterances. If `sttResultMode` equals `complete` this parameter is ignored. |
| sttRequestLogging | `boolean` | `false` | This parameter specifies whether request logging should be done for every STT audio transcription request. |
| baseModelVersion | `rstring` | `Empty string` | This parameter specifies a particular base model version to be used for transcription. |
| customizationId | `rstring` | `Empty string` | This parameter specifies a custom language model to be used for transcription. |
| customizationWeight | `float64` | `0.0` | This parameter specifies a relative weight for a custom language model as a float64 between 0.0 to 1.0 |
| acousticCustomizationId | `rstring` | `Empty string` | This parameter specifies a custom acoustic model to be used for transcription. |
| filterProfanity | `boolean` | `false` | This parameter indicates whether profanity should be filtered from a transcript. |
| maxUtteranceAlternatives | `int32` | `1` | This parameter indicates the required number of n-best alternative hypotheses for the transcription results. |
| wordAlternativesThreshold | `float64` | `0.0` | This parameter controls the density of the word alternatives results (a.k.a. Confusion Networks). A value of 0.0 disables this feature. Valid value must be less than 1.0 |
| smartFormattingNeeded | `boolean` | `false` | This parameter indicates whether to convert date, time, phone numbers, currency values, email and URLs into conventional representations. |
| keywordsSpottingThreshold | `float64` | `0.0` | This parameter specifies the minimum confidence level that the STT service must have for an utterance word to match a given keyword. A value of 0.0 disables this feature. Valid value must be less than 1.0. |
| keywordsToBeSpotted | `list<rstring>` | `Empty list` | This parameter specifies a list (array) of strings to be spotted. |
| websocketLoggingNeeded | `boolean` | `false` | This parameter specifies whether logging is needed from the Websocket library. |
| cpuYieldTimeInAudioSenderThread | `float64` | `0.001 i.e. 1 millisecond` | This parameter specifies the CPU yield time (in seconds) needed inside the audio sender thread's tight loop spinning to look for new audio data to be sent to the STT service. It should be >= 0.0 |
| maxAllowedConnectionAttempts | `int32` | `10` | This parameter specifies the maximum number of attempts to make a Websocket connection to the STT service. It should be >= 1 |
| maxConnectionRetryDelay | `float64` | `60.0` | The maximum wait time in seconds before a connection retry is made. The retry delay of connection to the STT service increases exponentially starting from 2 seconds but not exceeding 'maxConnectionRetryDelay'. It must be greater 1.0 |
| sttLiveMetricsUpdateNeeded | `boolean` | `true` | This parameter specifies whether live update for this operator's custom metrics is needed. |

### WatsonSTT operator's custom output functions
Following are the custom output functions supported by the WatsonSTT operator. These functions can be called as needed within the output clause of this operator's SPL invocation code.

| Output function name | Description |
| --- | --- |
| `int32 getUtteranceNumber()` | Returns an int32 number indicating the utterance number. |
| `rstring getUtteranceText()` | Returns the transcription of audio in the form of a single utterance. |
| `boolean isFinalizedUtterance()` | Returns a boolean value to indicate if this is an interim partial utterance or a finalized utterance. |
| `float32 getConfidence()` | Returns a float32 confidence value for an interim partial utterance or for a finalized utterance or for the full text. |
| `rstring getSTTErrorMessage()` | Returns the Watson STT error message if any during transcription. |
| `boolean isTranscriptionCompleted()` | Returns a boolean value to indicate whether the full transcription is completed. |
| `list<rstring> getUtteranceAlternatives()` | Returns a list of n-best alternative hypotheses for an utterance result. List will have the very best guess first followed by the next best ones in that order. |
| `list<list<rstring>> getWordAlternatives()` | Returns a nested list of word alternatives (Confusion Networks). |
| `list<list<float64>> getWordAlternativesConfidences()` | Returns a nested list of word alternatives confidences (Confusion Networks). |
| `list<float64> getWordAlternativesStartTimes()` | Returns a list of word alternatives start times (Confusion Networks). |
| `list<float64> getWordAlternativesEndTimes()` | Returns a list of word alternatives end times (Confusion Networks). |
| `list<rstring> getUtteranceWords()` | Returns a list of words in an utterance result. |
| `list<float64> getUtteranceWordsStartTimes()` | Returns a list of start times of the words in an utterance result relative to the start of the audio. |
| `list<float64> getUtteranceWordsEndTimes()` | Returns a list of end times of the words in an utterance result relative to the start of the audio. |
| `float64 getUtteranceStartTime()` | Returns the start time of an utterance relative to the start of the audio. |
| `float64 getUtteranceEndTime()` | Returns the end time of an utterance relative to the start of the audio. |
| `list<int32> getUtteranceWordsSpeakers()` | Returns a list of speaker ids for the individual words in an utterance result. |
| `list<float64> getUtteranceWordsSpeakersConfidences()` | Returns a list of confidences in identifying the speakers of the individual words in an utterance result. |
| `map<rstring, list<map<rstring, float64>>> getKeywordsSpottingResults()` | Returns the STT keywords spotting results as a map of key/value pairs. Read the Operator Usage Patterns section to learn about the map contents. |
