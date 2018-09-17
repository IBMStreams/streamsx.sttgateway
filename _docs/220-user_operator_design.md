---
title: "Operator Design"
permalink: /docs/user/OperatorDesign/
excerpt: "Describes the design of the Message Hub toolkit operators."
last_modified_at: 2018-09-10T12:37:48+01:00
redirect_from:
   - /theme-setup/
sidebar:
   nav: "userdocs"
---
{% include toc %}
{%include editme %}

This IBM Watson STT Gateway toolkit contains the following operator to enable the Speech To Text feature inside the Streams applications via the IBM Watson STT public/private cloud service.

 * **WatsonSTT** - this operator is a Websocket based C++ operator that will perform the Speech To Text transcription.

Main goal of this operator is to make the task of the Speech To Text transcription as simple as possible for the user. With that goal in mind, this operator provides the following set of core features. High level description of such features is provided below and a detailed explanation is given in the next section titled "WatsonSTT Usage Patterns".

 * It allows the user to specify any supported base language model and the audio content type at the time of launching the application.

 * It allows the user to select one of the three supported STT result modes to define the granularity of the transcription result.

 * Apart from the transcription result, it allows the user to enable speaker identification, word alternatives a.k.a. Confusion Networks, utterance alternatives a.k.a. n-best hypotheses, utterance words, utterance words confidences, utterance words timestamps etc.

 * It also allows the user to specify language model and acoustic model customization ids along with customization weight to augment the STT service's use of its chosen base model along with user's own custom models to improve the transcription accuracy.

 * To provide scalability, it lets the user to specify many WatsonSTT operator instances to work in parallel for transcribing concurrent audio conversations in real time (via network packets) or in batch (via stored files).

 * WatsonSTT operator instances running in parallel can be fused as needed to optimally use the available number of CPU cores.

 * In addition to letting the user choose the core STT capabilities, it also allows the user to take advantage of the other value added features such as profanity filtering, smart formatting, keywords spotting etc.


### WatsonSTT operator parameters
Following are the parameters accepted by the WatsonSTT operator. Some parameters are mandatory with user-provided values and others are optional with default values assigned within the C++ operator logic.

| Parameter Name | Type | Default | Description |
| --- | --- | --- | --- |
| uri | rstring | `User must provide this value. No default.` | This parameter specifies the Watson STT web socket service URI. |
| authToken | rstring | `User must provide this value. No default.` | This parameter specifies the auth token needed to access the Watson STT service. |
| baseLanguageModel | rstring | `User must provide this value. No default.` | This parameter specifies the name of the Watson STT base language model that should be used. |
| contentType | rstring | `audio/wav` | This parameter specifies the content type to be used for transcription. |
| sttResultMode | int32 | `3` | This parameter specifies what type of STT result is needed: 1 to get partial utterances, 2 to get completed utterance, 3 to get the full text after transcribing the entire audio. |
| sttRequestLogging | boolean | `false` | This parameter specifies whether request logging should be done for every STT audio transcription request. |
| baseModelVersion | rstring | `Empty string` | This parameter specifies a particular base model version to be used for transcription. |
| customizationId | rstring | `Empty string` | This parameter specifies a custom language model to be used for transcription. |
| customizationWeight | float64 | `0.0` | This parameter specifies a relative weight for a custom language model as a float64 between 0.0 to 1.0 |
| acousticCustomizationId | rstring | `Empty string` | This parameter specifies a custom acoustic model to be used for transcription. |
| filterProfanity | boolean | `false` | This parameter indicates whether profanity should be filtered from a transcript. |
| sttJsonResponseDebugging | boolean | `false` | This parameter is used for debugging the STT JSON response message. Mostly for IBM internal use. |
| maxUtteranceAlternatives | int32 | `1` | This parameter indicates the required number of n-best alternative hypotheses for the transcription results. |
| wordAlternativesThreshold | float64 | `0.0` | This parameter controls the density of the word alternatives results (a.k.a. Confusion Networks). A value of 0.0 disables this feature. Valid value must be less than 1.0 |
| wordConfidenceNeeded | boolean | `false` | This parameter indicates whether the transcription result should include individual words and their confidences or not. |
| wordTimestampNeeded | bollean | `false` | This parameter indicates whether the transcription result should include individual words and their timestamps or not. |
| identifySpeakers | boolean | `false` | This parameter indicates whether the speakers of the individual words in an utterance result should be identified. |
| smartFormattingNeeded | boolean | `false` | This parameter indicates whether to convert date, time, phone numbers, currency values, email and URLs into conventional representations. |
| keywordsSpottingThreshold | float64 | `0.0` | This parameter specifies the minimum confidence level that the STT service must have for an utterance word to match a given keyword. A value of 0.0 disables this feature. Valid value must be less than 1.0. |
| keywordsToBeSpotted | list<rstring> | `Empty list` | This parameter specifies a list (array) of strings to be spotted. |
| websocketLoggingNeeded | boolean | `false` | This parameter specifies whether logging is needed from the Websocket library. |
| cpuYieldTimeInAudioSenderThread | float64 | `0.001 i.e. 1 millisecond` | This parameter specifies the CPU yield time (in seconds) needed inside the audio sender thread's tight loop spinning to look for new audio data to be sent to the STT service. It should be >= 0.0 |
| waitTimeBeforeSTTServiceConnectionRetry | float64 | `3.0` | This parameter specifies the time (in seconds) to wait before retrying a connection attempt to the Watson STT service. It should be >= 1.0 |
| maxAllowedConnectionAttempts | int32 | `10` | This parameter specifies the maximum number of attempts to make a Websocket connection to the STT service. It should be >= 1 |


### WatsonSTT operator's custom output functions
Following are the custom output functions supported by the WatsonSTT operator. These functions can be called as needed within the output clause of this operator's SPL invocation code.

| Output function name | Description |
| --- | --- |
| int32 getUtteranceNumber() | Returns an int32 number indicating the utterance number. |
| rstring getUtteranceText() | Returns the transcription of audio in the form of a single utterance. |
| boolean isFinalizedUtterance() | Returns a boolean value to indicate if this is an interim partial utterance or a finalized utterance. |
| float32 getConfidence() | Returns a float32 confidence value for an interim partial utterance or for a finalized utterance or for the full text. |
| rstring getFullTranscriptionText() | Returns the transcription of audio in the form of full text after completing the entire transcription. |
| rstring getSTTErrorMessage() | Returns the Watson STT error message if any during transcription. |
| boolean isTranscriptionCompleted() | Returns a boolean value to indicate whether the full transcription is completed. |
| list<rstring> getUtteranceAlternatives() | Returns a list of n-best alternative hypotheses for an utterance result. List will have the very best guess first followed by the next best ones in that order. |
| list<list<rstring>> getWordAlternatives() | Returns a nested list of word alternatives (Confusion Networks). |
| list<list<float64>> getWordAlternativesConfidences() | Returns a nested list of word alternatives confidences (Confusion Networks). |
| list<float64> getWordAlternativesStartTimes() | Returns a list of word alternatives start times (Confusion Networks). |
| list<float64> getWordAlternativesEndTimes() | Returns a list of word alternatives end times (Confusion Networks). |
| list<rstring> getUtteranceWords() | Returns a list of words in an utterance result. |
| list<float64> getUtteranceWordsStartTimes() | Returns a list of start times of the words in an utterance result relative to the start of the audio. |
| list<float64> getUtteranceWordsEndTimes() | Returns a list of end times of the words in an utterance result relative to the start of the audio. |
| float64 getUtteranceStartTime() | Returns the start time of an utterance relative to the start of the audio. |
| float64 getUtteranceEndTime() | Returns the end time of an utterance relative to the start of the audio. |
| list<int32> getUtteranceWordsSpeakers() | Returns a list of speaker ids for the individual words in an utterance result. |
| list<float64> getUtteranceWordsSpeakersConfidences() | Returns a list of confidences in identifying the speakers of the individual words in an utterance result. |
| map<rstring, list<map<rstring, float64>>> getKeywordsSpottingResults() | Returns the STT keywords spotting results as a map of key/value pairs. Read this toolkit's documentation to learn about the map contents. |
