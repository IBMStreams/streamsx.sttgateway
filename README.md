# STT Gateway toolkit for IBM Streams

## Purpose
This toolkit is designed to ingest audio data either stored in files (.wav, .mp3 etc. for a batch workload) or streamed through a network switch (for a real-time workload). It then transcribes that audio into text via the IBM Watson STT (Speech To Text) service running on the IBM public cloud or on the IBM Cloud Pak (ICP).

## Documentation
1. The official toolkit documentation with extensive details is available at this URL: https://ibmstreams.github.io/streamsx.sttgateway/

2. A file named sttgateway-tech-brief.txt available at this tooolkit's top-level directory also provides a good amount of information about what this toolkit does, how it can be built and how it can be used in the IBM Streams applications.

## Requirements
1. Network connectivity to the IBM Watson Speech To Text (STT) service running either on the public cloud or on the cloud pak is needed from the IBM Streams Linux machines where this toolkit will be used.
   
2. A valid IAM access token is needed to use the Watson STT service. This toolkit uses Websocket to communicate with the Watson STT cloud service. For that Websocket interface, one must use the IAM access token and not the usual cloud service credentials. So, users of this toolkit must provide their STT service instance's API key when launching the Streams application(s) that will have a dependency on this toolkit. Using that API key, a utility SPL composite named IAMAccessTokenGenerator available in this toolkit will be able to generate the IAM access token and then subsequently refresh that token to keep it valid. A Streams application employing this toolkit can make use of that utility composite to generate the necessary IAM access token. Please do more reading about the IAM access token from [here](https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-websockets#WSopen).

3. On the IBM Streams application development machine (where the application code is compiled to create the application bundle), it is necessary to download and install the boost_1_67_0 or a higher version as well as the websocketpp version 0.8.1. Please note that this is not needed on the Streams application execution machines. For the essential steps to meet this requirement, please refer to the above-mentioned documentation URL or a file named sttgateway-tech-brief.txt available at this tooolkit's top-level directory.

## Example usage of this toolkit inside a Streams application
Here is a code snippet that shows how to invoke the WatsonSTT operator available in this toolkit with a subset of supported features:

```
use com.ibm.streamsx.sttgateway.watson::*;

/*
Invoke one or more instances of the WatsonSTT operator.
You can send the audio data to this operator all at once or 
you can send the audio data for the live-use case as it becomes
available from your telephony network switch.
Avoid feeding audio data coming from more than one data source into this 
parallel region which may cause erroneous transcription results.

NOTE: The WatsonSTT operator allows fusing multiple instances of
this operator into a single PE. This will help in reducing the 
total number of CPU cores used in running the application.
*/
@parallel(width = $numberOfSTTEngines, 
partitionBy=[{port=ABC, attributes=[conversationId]}], broadcast=[IAT])
(stream<STTResult_t> STTResult) as STT = WatsonSTT(AudioBlobContent as ABC; IamAccessToken as IAT) {
   param
      uri: $sttUri;
      baseLanguageModel: $sttBaseLanguageModel;
			
   output
      STTResult: conversationId = conversationId, 
                 utteranceNumber = getUtteranceNumber(),
                 utteranceText = getUtteranceText(),
                 utteranceStartTime = getUtteranceStartTime(),
                 utteranceEndTime = getUtteranceEndTime(),
                 finalizedUtterance = isFinalizedUtterance(),
                 transcriptionCompleted = isTranscriptionCompleted(),
                 fullTranscriptionText = getFullTranscriptionText(),
                 sttErrorMessage = getSTTErrorMessage();
}
```

A built-in example inside this toolkit can be compiled and launched with the default STT options as shown below:

```
cd   streamsx.sttgateway/samples/AudioFileWatsonSTT
make
st  submitjob  -d  <YOUR_STREAMS_DOMAIN>  -i  <YOUR_STREAMS_INSTANCE>  output/com.ibm.streamsx.sttgateway.sample.watsonstt.AudioFileWatsonSTT.sab  -P  sttApiKey=<YOUR_WATSON_STT_SERVICE_API_KEY>
```

Following IBM Streams job sumission command shows how to override the default values with your own as needed for the various STT options:

```
cd   streamsx.sttgateway/samples/AudioRawWatsonSTT
make
st submitjob  -d  <YOUR_STREAMS_DOMAIN>  -i  <YOUR_STREAMS_INSTANCE>  output/com.ibm.streamsx.sttgateway.sample.watsonstt.AudioRawWatsonSTT.sab -P  sttApiKey=<YOUR_WATSON_STT_SERVICE_API_KEY>  -P sttResultMode=2   -P sttBaseLanguageModel=en-US_NarrowbandModel  -P contentType="audio/wav"    -P filterProfanity=true   -P keywordsSpottingThreshold=0.294   -P keywordsToBeSpotted="['country', 'learning', 'IBM', 'model']"   -P smartFormattingNeeded=true   -P identifySpeakers=true   -P wordTimestampNeeded=true   -P wordConfidenceNeeded=true   -P wordAlternativesThreshold=0.251   -P maxUtteranceAlternatives=5   -P audioBlobFragmentSize=32768   -P sttLiveMetricsUpdateNeeded=true  -P audioDir=<YOUR_AUDIO_FILES_DIRECTORY>   -P numberOfSTTEngines=100
```

## WHATS NEW

v1.0.3:
* Jun/12/2019
* Replaced the use of the auth token with the IAM access token.
* Added a utility composite named IAMAccessTokenGenerator to generate/refresh an IAM access token. This utility composite can be invoked within the IBM Streams applications that make use of this toolkit to perform speech to text transcription.

v1.0.2:
* Sep/24/2018
* Added a .gitkeep file in the data, impl, include and lib empty folders.

v1.0.1:
- Sep/21/2018
- In the internal threads of the WatsonSTT operator code, the CPU yield time during idleness and in between the Websocket connection attempts was increased from a few milliseconds to 1 second.
- New operator parameter sttLiveMetricsUpdateNeeded was added to give the users a way to turn the custom operator metrics reporting on and off in the time critical path of audio transcription.
- Use of the internal custom metrics update API was changed from setValue to setValueNoLock. 
- Corresponding documentation refinements were also done.

v1.0.0:
- Sep/17/2018
- Very first release of this toolkit that was tested to support all the major features available in the IBM Watson Speech To Text (STT) cloud service.
