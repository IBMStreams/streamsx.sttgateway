---
title: "Operator Usage Patterns"
permalink: /docs/user/WatsonSTTUsagePatterns/
excerpt: "Describes the WatsonSTT operator usage patterns."
last_modified_at: 2019-09-05T11:23:48+01:00
redirect_from:
   - /theme-setup/
sidebar:
   nav: "userdocs"
---
{% include toc %}
{%include editme %}

## Important details needed for using the WatsonSTT operator
As described in the other documentation pages, the WatsonSTT operator uses the Websocket interface to connect to the IBM Watson Speech To Text service running on the IBM public cloud or on IBM Cloud Pak for Data (CP4D i.e. private cloud). In order to use this operator, the following three values must be available with you at the time of launching the Streams application.

1. Base language model name that you want to use for transcribing your audio data. Supported base language models are listed in this URL: [Base language models](https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-input#models)

2. Content type of your audio data. Supported content types are listed in this URL: [Content type](https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-audio-formats#audio-formats)

3. Your public cloud STT service instance's API key or a CP4D access token will have to be provided as a submission time parameter at the time of launching the IBM Streams application. That API key will be used by a utility SPL composite available in the streamsx.sttgateway toolkit to generate an IAM access token needed for securely accessing the IBM Watson STT service on public cloud. You must ensure that you have a valid API key for public cloud or an access token for CP4D to provide as a submission time parameter value. The use of the IAM access token is outlined in this URL: [IAM Access Token](https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-websockets#WSopen)

4. If you are using the STT service on Cloud Pak for Data (CP4D), then you must also provide your STT service instance URI obtained specifically from your CP4D web console.

These four important values are passed via the corresponding operator parameters. There are also other STT optional parameters that you can configure for additional STT features that your application may require.

## WatsonSTT operator's result mode parameter
WatsonSTT operator can process the given audio data and return the transcription results in one of the following three configurable modes. This is controlled by the operator parameter named `sttResultMode` which takes a value of 1 or 2 or 3 as explained below in that order.

1. __Partial utterance result__: As the audio data is getting transcribed, this operator will keep returning partial utterance results as output tuples. For a single spoken sentence, there will be many partial utterances emitted followed by one finalized utterance.

2. __Finalized utterance result__: In this STT result mode, it will send only one output tuple for every finalized utterance and there will be no partial utterances sent as output tuples.

3. __Fully transcribed result__: In this STT result mode, this operator will emit only one output tuple for the entire audio after the transcription is completed for that entire audio. This single output tuple will contain an rstring attribute holding the fully transcribed text comprised of all the finalized utterances representing all the spoken sentences in that audio conversation.

Users can specify a suitable value for the `sttResultMode` parameter in their operator invocation SPL code segment.

## Required input data for the WatsonSTT operator
This operator has one input port where it will receive incoming tuples and one output port where it will send its output tuples to the downstream operators. Crucial input data for this operator is the actual speech/audio data that needs to be transcribed. Applications invoking this operator must provide details about the speech data via the input port of the WatsonSTT operator. Input port schema for this operator must have an attribute named `speech`. This attribute must adhere to the following description.

* **speech** (required, rstring/blob) - In the case of file based input (.wav, .mp3 etc. for batch workload), the expected value will be an absolute path of a file as an rstring. In the case of RAW audio data (received from a network switch for real-time workload), the expected input is of type blob. 

**Note:** _Raw audio data belonging to a given speech/audio conversation can be sent in full as a single blob to this operator or it can be sent in partial blob fragments via multiple input tuples. After sending the entire raw audio of an ongoing speech/audio conversation, the application code must send a tuple with an empty blob to indicate the end of that conversation. It is very important to send exactly ONLY ONE empty blob at the end of transmitting all the non-empty blob content (either in full or in partial fragments) belonging to a given audio conversation. This operator relies on this empty blob to inform the Watson STT service about the end of an ongoing audio conversation. So, remember to do this whether you are sending the audio blob data after reading it from a file or receiving it from a network switch._

In addition to the mandatory `speech` attribute, one can have any number of other optional attributes as needed in the tuples sent to this operator. A commonly used optional attribute is the one shown below which is an application-specific speech conversation id. It will be useful in correlating the transcription results with the corresponding audio conversation later in the other downstream operators. It can also serve as a partition key inside a parallel region configured with multiple WatsonSTT operator instances to achieve concurrent Speech To Text processing of many audio conversations. Such a partition key will provide affinity (stickiness) between an ongoing audio conversation and an operator instance inside a parallel region that must keep processing that audio conversation from beginning to end. 

* **conversationId** (optional, rstring) - An rstring conversationId field for identifying the origin of the audio data that is being sent for transcription (either an audio filename or a call center specific call identifier).

All the extra attributes present in the input port schema will be forwarded if matching attributes are found in the output port schema.

## Configuration parameters for the WatsonSTT operator
This operator provides about two dozen parameters to configure the way in which this operator will function to return different values in the output tuples containing the transcribed text. For the basic transcription i.e. sending some audio data and getting back just the fully transcribed text of the entire audio conversation, you can simply use only the three important parameters as discussed at the very top of this page. For getting back a lot more than just the fully transcribed text in the output tuple, you will have to use the other operator parameters as listed in the previous page.

## Input stream schema for the WatsonSTT operator
At the very basic level, input stream schema can be as shown below with just two attributes. But, in addition to these two attributes, there can also be other attributes as needed. 

__For a file based batch workload:__ `type AudioInput_t = rstring conversationId, rstring speech;`

__For a network switch based real-time workload:__ `type AudioInput_t = rstring conversationId, blob speech;`


## Output stream schema for the WatsonSTT operator
At the full scope of this operator, output stream schema can be as shown below with all possible attributes. It is shown here to explain the basic and very advanced features of the Watson STT service. Not all real life applications will need all these attributes. You can decide to include or omit these attributes based on the specific STT features your application will need. Trimming the unused attributes will also help in reducing the STT processing overhead and in turn help in receiving the STT results faster.

```
type STTResult_t = 
    rstring conversationId, int32 utteranceNumber,
    rstring utteranceText, boolean finalizedUtterance,
    float32 confidence, rstring fullTranscriptionText,
    rstring sttErrorMessage, boolean transcriptionCompleted,
    list<rstring> utteranceAlternatives, // n-best utterance alternative hypotheses
    list<list<rstring>> wordAlternatives, // Confusion Networks
    list<list<float64>> wordAlternativesConfidences,
    list<float64> wordAlternativesStartTimes,
    list<float64> wordAlternativesEndTimes,
    list<rstring> utteranceWords,
    list<float64> utteranceWordsConfidences,
    list<float64> utteranceWordsStartTimes,
    list<float64> utteranceWordsEndTimes,
    float64 utteranceStartTime,
    float64 utteranceEndTime,
    list<int32> utteranceWordsSpeakers,
    list<float64> utteranceWordsSpeakersConfidences,
    map<rstring, list<map<rstring, float64>>> keywordsSpottingResults;
```

## Invoking the WatsonSTT operator
In your SPL application, this operator can be invoked with either all operator parameters or a subset of the operator parameters. Similarly, in the output clause of the operator body, you can either call all the available custom output functions or a subset of those custom output functions to do your output stream attribute assignments. You can decide on the total number of operator parameters and the total number of custom output functions to use based on the real needs of your application.

You can invoke one or more instances of the WatsonSTT operator depending on the total amount of speech data available for transcription. You can send the audio data for a given audio file or an ongoing audio conversation to this operator all at once or you can send the audio data for the live usecase as it becomes available from your telephony network switch in multiple blob fragments.

**NOTE:** The WatsonSTT operator allows fusing multiple instances of this operator into a single PE. This will help in reducing the total number of CPU cores used in running your application. If you use multiple operator instances inside a Streams parallel region, you can decide to enable the partitioning feature for the parallel channels via a partition key e-g: by using an application-specific conversationId as a partition key. In general, audio data ingested from files will not require partitioning of the parallel channels. It will make better sense to use the partitioning of the parallel channels for real-time usecases where audio data is received from a network switch as several blob fragments for an ongoing audio conversation.

```
// Invoke one or more instances of the WatsonSTT operator.
// Avoid feeding audio data coming from more than one data source into this 
// parallel region which may cause erroneous transcription results.
// NOTE: The WatsonSTT operator allows fusing multiple instances of
// this operator into a single PE. This will help in reducing the 
// total number of CPU cores used in running the application.
//
@parallel(width = $numberOfSTTEngines, broadcast=[IAT])
stream<STTResult_t> STTResult = WatsonSTT(AudioFileName as AFN; IamAccessToken as IAT) {
   logic
      state: {
         mutable int32 _conversationCnt = 0;
      }
				
      onTuple AFN: {
         appTrc(Trace.error, "Channel " + (rstring)getChannel() + 
            ", Speech input " + (rstring)++_conversationCnt +
            ": " + (rstring)AFN.speech);
      }
			
   // Just to demonstrate, we are using all the operator parameters below.
   // Except for the first three parameters, every other parameter is an
   // optional one. In real-life applications, such optional parameters
   // can be omitted unless you want to change the default behavior of them.
   param
      uri: $sttUri;
      baseLanguageModel: $sttBaseLanguageModel;
      contentType: $contentType;
      sttResultMode: $sttResultMode;
      sttRequestLogging: $sttRequestLogging;
      filterProfanity: $filterProfanity;
      sttJsonResponseDebugging: $sttJsonResponseDebugging;
      maxUtteranceAlternatives: $maxUtteranceAlternatives;
      wordAlternativesThreshold: $wordAlternativesThreshold;
      wordConfidenceNeeded: $wordConfidenceNeeded;
      wordTimestampNeeded: $wordTimestampNeeded;
      identifySpeakers: $identifySpeakers;
      smartFormattingNeeded: $smartFormattingNeeded;
      keywordsSpottingThreshold: $keywordsSpottingThreshold;
      keywordsToBeSpotted: $keywordsToBeSpotted;
      websocketLoggingNeeded: $websocketLoggingNeeded;
      cpuYieldTimeInAudioSenderThread: $cpuYieldTimeInAudioSenderThread;
      waitTimeBeforeSTTServiceConnectionRetry : $waitTimeBeforeSTTServiceConnectionRetry;
      maxAllowedConnectionAttempts : $maxAllowedConnectionAttempts;
      sttLiveMetricsUpdateNeeded : $sttLiveMetricsUpdateNeeded;
								
      // Use the following operator parameters as needed.
      // Point to a specific version of the base model if needed.
      //
      // e-g: "en-US_NarrowbandModel.v07-06082016.06202016"
      baseModelVersion: $baseModelVersion;
      // Language model customization id to be used for the transcription.
      // e-g: "74f4807e-b5ff-4866-824e-6bba1a84fe96"
      customizationId: $customizationId;
      // Acoustic model customization id to be used for the transcription.
      // e-g: "259c622d-82a4-8142-79ca-9cab3771ef31"
      acousticCustomizationId: $acousticCustomizationId;
      // Relative weight to be given to the words in the custom Language model.
      customizationWeight: $customizationWeight;
			
   // Just for demonstrative purposes, we are showing below the output attribute
   // assignments using all the available custom output functions. In your
   // real-life applications, it is sufficient to do the assignments via
   // custom output functions only as needed.
   //
   // Some of the important output functions that must be used to check
   // the result of the transcription are:
   // getSTTErrorMessage --> It tells whether the transcription succeeded or not.
   // isFinalizedUtterance --> In sttResultMode 1, it tells whether this is a 
   //                          partial utterance or a finalized utterance.
   // isTranscriptionCompleted --> It tells whether the transcription is 
   //                              completed for the current audio conversation or not.
   //
   output
      STTResult: conversationId = speech, 
      utteranceNumber = getUtteranceNumber(),
      utteranceText = getUtteranceText(),
      finalizedUtterance = isFinalizedUtterance(),
      confidence = getConfidence(),
      fullTranscriptionText = getFullTranscriptionText(),
      sttErrorMessage = getSTTErrorMessage(),
      transcriptionCompleted = isTranscriptionCompleted(),
      // n-best utterance alternative hypotheses.
      utteranceAlternatives = getUtteranceAlternatives(),
      // Confusion networks (a.k.a. Consensus)
      wordAlternatives = getWordAlternatives(),
      wordAlternativesConfidences = getWordAlternativesConfidences(),
      wordAlternativesStartTimes = getWordAlternativesStartTimes(),
      wordAlternativesEndTimes = getWordAlternativesEndTimes(),
      utteranceWords = getUtteranceWords(),
      utteranceWordsConfidences = getUtteranceWordsConfidences(),
      utteranceWordsStartTimes = getUtteranceWordsStartTimes(),
      utteranceWordsEndTimes = getUtteranceWordsEndTimes(),
      utteranceStartTime = getUtteranceStartTime(),
      utteranceEndTime = getUtteranceEndTime(),
      // Speaker label a.k.a. Speaker id
      utteranceWordsSpeakers = getUtteranceWordsSpeakers(),
      utteranceWordsSpeakersConfidences = getUtteranceWordsSpeakersConfidences(),
      // Results from keywords spotting (matching) in an utterance.
      keywordsSpottingResults = getKeywordsSpottingResults();

   // If needed, you can decide not to fuse the WatsonSTT operator instances and
   // keep each instance of this operator on its own PE (a.k.a Linux process) by
   // removing the block comment around this config clause.
   /*
   config
      placement : partitionExlocation("sttpartition");
   */
}

```

The operator invocation code snippet shown above is ideally suited for a batch workload where the audio data is made available through files. If it is a real-time workload where the audio data is streamed via a network switch, then the operator invocation head will be slightly different and the rest of the code will be similar to what is done above for a file based audio input. That slightly different operator invocation head is shown below. You can notice the difference in the input stream (AudioFileName versus AudioBlobContent) as well as in the use of a partition key within the parallel region annotation code segment.

```
@parallel(width = $numberOfSTTEngines, 
partitionBy=[{port=ABC, attributes=[conversationId]}])
stream<STTResult_t> STTResult = WatsonSTT(AudioBlobContent as ABC) {
   // Rest of the code within this block is similar to the 
   // one shown in the previous code snippet above.
}

```

## Using the custom output functions in the WatsonSTT operator
This operator does the automatic attribute value assignment from the input tuple to the output tuple for those matching output tuple attributes for which there is no explicit value assignment done in the output clause. Users can decide to assign values to the output tuple attributes via custom output functions as per the needs of the application. It is also important to note that many of the custom output functions that are applicable only at an utterance level will not be meaningful in the situations where the `sttResultMode` operator parameter is set to `3` to transcribe the entire audio as a whole instead of utterance by utterance. So, use the custom output functions appropriately depending on whether a choice is made to do the transcription at the utterance level or at the level of the entire audio as a whole. The description of the output functions will give indications about whether a given output function is utterance specific or not.

At the very basic level, users should call the very first three custom output functions shown below at all times. Rest of the custom output functions can be either called or omitted as dictated by your application requirements.

**getSTTErrorMessage()** is used to find out if there is any transcription error returned by the operator. It is applicable when the `sttResultMode` operator parameter is set to 1 or 2 or 3.

**isTranscriptionCompleted()** is used to find out if the transcription is completed for the entire audio of an ongoing speech conversation. It is applicable when the `sttResultMode` operator parameter is set to 1 or 2 or 3.

**getFullTranscriptionText()** or **getUtteranceText()** is used either to get the full transcription text for the entire audio when the operator parameter `sttResultMode` is set to 3 or in the latter case to get either the partial or finalized utterance text when the operator parameter `sttResultMode` is set to 1 or 2.

There are many other custom output functions available in this operator that can be used as needed. They are all documented well in the previous  page as well as in the SPLDoc for this toolkit. For additional reading, relevant pointers are given below directly to go to the documentation pages for the Watson STT service.

**getUtteranceNumber()** is used to get the current utterance number in the given audio conversation being transcribed. It is applicable when the `sttResultMode` operator parameter is set to 1 or 2.

**isFinalizedUtterance()** is used to find out if a given utterance is a partial one or a finalized one. It is applicable when the `sttResultMode` operator parameter is set to 1.

**getConfidence()** is used to find out the confidence score for a finalized utterance when the `sttResultMode` operator parameter is set to 1 or 2.

**getUtteranceAlternatives()** is used to obtain the n-best utterance alternative hypotheses. It is applicable when the `sttResultMode` operator parameter is set to 1 or 2. It returns a list of rstring values. For more details, read [here](https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#max_alternatives).

**getWordAlternatives()** is used to obtain the Confusion Networks. It is applicable when the `sttResultMode` operator parameter is set to 1 or 2. It returns a nested list of rstring values. For more details, read [here](https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#word_alternatives).

**getWordAlternativesConfidences()** is used to obtain the confidences of the word alternatives (Confusion Networks) for a given utterance. It is applicable when the `sttResultMode` operator parameter is set to 1 or 2. It returns a nested list of float64 values. 

**getWordAlternativesStartTimes()** is used to obtain the start times of the word alternatives (Confusion Networks) for a given utterance. It is applicable when the `sttResultMode` operator parameter is set to 1 or 2. It returns a list of float64 values.

**getWordAlternativesEndTimes()** is used to obtain the end times of the word alternatives (Confusion Networks) for a given utterance. It is applicable when the `sttResultMode` operator parameter is set to 1 or 2. It returns a list of float64 values.

**getUtteranceWords()** is used to obtain all the words present in a given utterance. It is applicable when the `sttResultMode` operator parameter is set to 1 or 2. It returns a list of rstring values. 

**getUtteranceWordsConfidences()** is used to obtain the confidence score for the individual words present in a given utterance. It is applicable when the `sttResultMode` operator parameter is set to 1 or 2. It returns a list of float64 values. For more details, read [here](https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#word_confidence).

**getUtteranceWordsStartTimes()** is used to obtain the start times of the individual words present in a given utterance. It is applicable when the `sttResultMode` operator parameter is set to 1 or 2. It returns a list of float64 values. For more details, read [here](https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#word_timestamps).

**getUtteranceWordsEndTimes()** is used to obtain the end times of the individual words present in a given utterance. It is applicable when the `sttResultMode` operator parameter is set to 1 or 2. It returns a list of float64 values.

**getUtteranceStartTime()** is used to get the utterance start time in the overall audio conversation. It is applicable when the `sttResultMode` operator parameter is set to 1 or 2.

**getUtteranceEndTime()** is used to get the utterance end time in the overall audio conversation. It is applicable when the `sttResultMode` operator parameter is set to 1 or 2.

**getUtteranceWordsSpeakers()** is used to obtain the speaker ids present in a finalized utterance. It is applicable when the `sttResultMode` operator parameter is set to 1 or 2. It returns a list of int32 values. For more details, read [here](https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#speaker_labels).

**getUtteranceWordsSpeakersConfidences()** is used to obtain the confidence score for the speaker ids present in a finalized utterance. It is applicable when the `sttResultMode` operator parameter is set to 1 or 2. It returns a list of float64 values.

**getKeywordsSpottingResults()** is used to obtain the result of certain keywords to be spotted in the given audio conversation. It is applicable when the `sttResultMode` operator parameter is set to 1 or 2. It returns a `map<rstring, list<map<rstring, float64>>>`. This map contains all the rstring matching keywords as its keys. For every such key, it will hold a list containing another map which will have an rstring key and a float64 value. In this inner map, it will have these as keys along with their corresponding values: `start_time, end_time and confidence`. For more details, read [here](https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-output#keyword_spotting).

## Using customized Language Model (LM) and Acoustic Model (AM) in the WatsonSTT operator
In addition to using the base language model provided by the IBM Watson STT service, users can do their own customization for Language model (LM) and Acoustic Model (AM). This will help to increase the transcription accuracy by reducing the word error rate (WER). This can be done via the published tools and techniques supported by the IBM Watson STT service. After completing the customization, users can obtain the customization ids for their custom LM and AM and pass them to the WatsonSTT operator via the following parameters.

```
// Point to a specific version of the base model if needed.
//
// e-g: "en-US_NarrowbandModel.v07-06082016.06202016"
baseModelVersion: $baseModelVersion;
// Language model customization id to be used for the transcription.
// e-g: "74f4807e-b5ff-4866-824e-6bba1a84fe96"
customizationId: $customizationId;
// Acoustic model customization id to be used for the transcription.
// e-g: "259c622d-82a4-8142-79ca-9cab3771ef31"
acousticCustomizationId: $acousticCustomizationId;
// Relative weight to be given to the words in the custom Language model.
customizationWeight: $customizationWeight;
```

For the steps required to create your own custom language model (LM), read [here](https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-languageCreate#languageCreate).
For the steps required to create your own custom acoustic model (AM), read [here](https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-acoustic#acoustic).

## Custom metrics available in the WatsonSTT operator
This operator provides the following custom metrics that can be queried via the IBM Streams REST/JMX APIs or viewed via the commonly used utilities such as streamtool and Streams Web Console. The Counter kind metrics (1 and 4 below) will be updated when the operator starts. But, the Gauge kind metrics (2 and 3 below) will be updated live during transcription only when the sttLiveMetricsUpdateNeeded operator parameter is set to true.

1. **nWebsocketConnectionAttempts**: It shows how many connection attempts it took to connect to the STT service from this operator. It is useful to know whether the Websocket connection to the Watson STT service was made within the allowed number of connection attempts or not.

2. **nFullAudioConversationsReceived**: It shows a running total of the number of audio conversations received for processing by this operator.

3. **nFullAudioConversationsTranscribed**: It shows a running total of the number of audio conversations transcribed by this operator.

4. **nSTTResultMode**: It shows how this operator is configured to return the STT results. 1 = Return partial utterances, 2 = Return only finalized utterances, 3 = Return only the full transcribed text after completing the transcription for the entire audio conversation.

## Running the example applications that use the WatsonSTT operator
There are two working examples included within this toolkit. You can use them as a reference to learn more about putting this operator to use in your own applications. You can use similar streamtool submitjob commands as shown below in your own applications.

1. To use this operator with default values for all the operator parameters:

```
cd   streamsx.sttgateway/samples/AudioFileWatsonSTT
make
st  submitjob  -d  <YOUR_STREAMS_DOMAIN>  -i  <YOUR_STREAMS_INSTANCE>  output/com.ibm.streamsx.sttgateway.sample.watsonstt.AudioFileWatsonSTT.sab  -P  sttApiKey=<YOUR_WATSON_STT_SERVICE_API_KEY>
```

2. To override the default values with your own as needed for the various STT options:

```
cd   streamsx.sttgateway/samples/AudioRawWatsonSTT
make
st submitjob  -d  <YOUR_STREAMS_DOMAIN>  -i  <YOUR_STREAMS_INSTANCE>  output/com.ibm.streamsx.sttgateway.sample.watsonstt.AudioRawWatsonSTT.sab -P  sttApiKey=<YOUR_WATSON_STT_SERVICE_API_KEY>  -P sttResultMode=2   -P sttBaseLanguageModel=en-US_NarrowbandModel  -P contentType="audio/wav"    -P filterProfanity=true   -P keywordsSpottingThreshold=0.294   -P keywordsToBeSpotted="['country', 'learning', 'IBM', 'model']"   -P smartFormattingNeeded=true   -P identifySpeakers=true   -P wordTimestampNeeded=true   -P wordConfidenceNeeded=true   -P wordAlternativesThreshold=0.251   -P maxUtteranceAlternatives=5   -P audioBlobFragmentSize=32768   -P audioDir=<YOUR_AUDIO_FILES_DIRECTORY>   -P numberOfSTTEngines=100
```

## Conclusion
As explained in the chapters thus far, this operator is a powerful one to integrate the IBM Streams applications with the IBM Watson STT service in a highly scalable and secure manner. It fully enables Speech To Text transcription of the audio data stored in files (batch mode) or received from a network switch (real-time mode). 

**Cheers and good luck in finding impactful insights from your audio data.**
