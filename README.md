# README for the STT Gateway toolkit for IBM Streams

## Documentation
1. The official toolkit documentation is available at this URL: https://ibmstreams.github.io/streamsx.sttgateway/

2. Another file named sttgateway-tech-brief.txt available at this tooolkit's top-level directory also provides a good amount of information about what this toolkit does, how it can be built and how it can be used in the Streams applications.

## Requirements
1. Network connectivity to the Watson Speech To Text (STT) service running either on the public or the private cloud is needed from the IBM Streams Linux machines where this toolkit will be used.
   
2. A valid authentication token is needed to use the Watson STT service. This toolkit uses Websocket to communicate with the Watson STT cloud service. For that Websocket interface, one must use the auth tokens and not the usual cloud service credentials. So, users of this toolkit must generate their own authentication token and provide it when launching the Streams application(s) that will have a dependency on this toolkit. To generate your own auth token, please do more reading from [here](https://console.bluemix.net/docs/services/speech-to-text/input.html#tokens).

3. On the IBM Streams application development machine (where the application code is compiled to create the application bundle), it is necessary to download and install the boost_1_67_0 or a higher version as well as the websocketpp version 0.8.1. Please note that this is not needed on the Streams application execution machines. For the steps required to meet this requirement, please refer to the file named sttgateway-tech-brief.txt available at this tooolkit's top-level directory.

## Example usage of this toolkit inside a Streams application
Here is a code snippet that shows how to invoke the WatsonSTT operator available in this toolkit:

```
use com.ibm.streamsx.sttgateway.watson::*;

/*
Invoke one or more instances of the WatsonSTT operator.
You can send the audio data to this operator all at once or 
you can send the audio data for the live-use case as it becomes
available from your telephony network switches.

NOTE: The WatsonSTT operator allows fusing multiple instances of
this operator into a single PE. This will help in reducing the 
total number of CPU cores used in running the application.
*/
@parallel(width = $numberOfSTTEngines, 
partitionBy=[{port=ABC, attributes=[conversationId]}])
(stream<STTResult_t> STTResult) as STT = WatsonSTT(AudioBlobContent as ABC) {
   param
      uri: $sttUri;
      authToken: $sttAuthToken;
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
st  submitjob  -d  <YOUR_STREAMS_DOMAIN>  -i  <YOUR_STREAMS_INSTANCE>  output/com.ibm.streamsx.sttgateway.sample.watsonstt.AudioFileWatsonSTT.sab  -P  sttAuthToken=<YOUR_WATSON_STT_SERVICE_AUTH_TOKEN>
```

Following IBM Streams job sumission command shows how to override the default values with your own as needed for the various STT options:

```
cd   streamsx.sttgateway/samples/AudioRawWatsonSTT
make
st submitjob  -d  <YOUR_STREAMS_DOMAIN>  -i  <YOUR_STREAMS_INSTANCE>  output/com.ibm.streamsx.sttgateway.sample.watsonstt.AudioRawWatsonSTT.sab -P  sttAuthToken=<YOUR_WATSON_STT_SERVICE_AUTH_TOKEN>  -P sttResultMode=2   -P sttBaseLanguageModel=en-US_NarrowbandModel  -P contentType="audio/wav"    -P filterProfanity=true   -P keywordsSpottingThreshold=0.294   -P keywordsToBeSpotted="['country', 'learning', 'IBM', 'model']"   -P smartFormattingNeeded=true   -P identifySpeakers=true   -P wordTimestampNeeded=true   -P wordConfidenceNeeded=true   -P wordAlternativesThreshold=0.251   -P maxUtteranceAlternatives=5   -P audioBlobFragmentSize=32768   -P audioDir=<YOUR_AUDIO_FILES_DIRECTORY>   -P numberOfSTTEngines=100
```

## WHATS NEW

v1.0.0
  - Initial release of this toolkit with support for all the major features available in the Watson Speech To Text cloud service.
