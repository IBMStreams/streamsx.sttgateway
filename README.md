# STT Gateway toolkit for IBM Streams

## Purpose
This toolkit is designed to ingest audio data either stored in files (.wav, .mp3 etc. for a batch workload) or streamed through a telephony infrastructure (for a real-time workload). It then transcribes that audio into text via the IBM Watson STT (Speech To Text) service running on the IBM public cloud or on the IBM Cloud Pak for Data (CP4D i.e. private cloud).

It provides the following two operators to realize that purpose.

**IBMVoiceGatewaySource** is a source operator that can be used to ingest speech data from the IBM Voice Gateway product v1.0.3.0 or higher. Such speech data comes from multiple live telephone conversations happening between different pairs of speakers e-g: customers and call center agents.

**WatsonSTT** is an analytic operator that can be used to transcribe speech data into text either in real-time or in batch mode.

## Architectural patterns enabled by this toolkit
1. For the **real-time** speech to text transcription, following are the possible architectural patterns.

- <span style="color:green">Your Telephony SIPREC-->IBM Voice Gateway-->IBM Streams<-->Watson Speech To Text on IBM Public Cloud</span>
 
- <span style="color:blue">Your Telephony SIPREC-->IBM Voice Gateway-->IBM Streams<-->Watson Speech To Text on IBM Cloud Pak for Data (CP4D)</span>
 
- <span style="color:purple">Your Telephony SIPREC-->IBM Voice Gateway-->IBM Streams<-->Watson Speech To Text engine embedded inside an IBM Streams operator</span>

2. For the **batch (post call)** speech to text transcription, following are the possible architectural patterns.
 
- <span style="color:green">Speech data files in a directory-->IBM Streams<-->Watson Speech To Text on IBM Public Cloud</span>
 
- <span style="color:blue">Speech data files in a directory-->IBM Streams<-->Watson Speech To Text on IBM Cloud Pak for Data (CP4D)</span>
 
- <span style="color:purple">Speech data files in a directory-->IBM Streams<-->Watson Speech To Text engine embedded inside an IBM Streams operator</span>

## Documentation
1. The official toolkit documentation with extensive details is available at this URL: https://ibmstreams.github.io/streamsx.sttgateway/

2. A file named sttgateway-tech-brief.txt available at this tooolkit's top-level directory also provides a good amount of information about what this toolkit does, how it can be built and how it can be used in the IBM Streams applications.

3. The official documentation for the IBM Voice Gateway product is available [here](https://www.ibm.com/support/knowledgecenter/SS4U29/whatsnew.html)

4. The official documentation for the IBM Watson Speech To Text service is available [here](https://www.ibm.com/watson/services/speech-to-text)

## Requirements
There are certain important requirements that need to be satisfied in order to use the IBM Streams STT Gateway toolkit in Streams applications. Such requirements are explained below.

**Note:** This toolkit is **not** supported on Red Hat Enterprise Linux Workstation release **6.x**
    
**Note:** This toolkit requires c++11 support.

1. Network connectivity to the IBM Watson Speech To Text (STT) service running either on the public cloud or on the Cloud Pak for Data (CP4D) is needed from the IBM Streams Linux machines where this toolkit will be used. The same is true to integrate with the IBM Voice Gateway product for the use cases involving speech data ingestion for live voice calls.

2. This toolkit uses Websocket to communicate with the IBM Voice Gateway and the Watson STT service. A valid IAM access token is needed to use the Watson STT service on the public cloud and a valid access token to use the Watson STT service on the CP4D. So, users of this toolkit must provide their public cloud STT service instance's API key or the CP4D STT service instance's access token when launching the Streams application(s) that will have a dependency on this toolkit. When using the API key from the public cloud, a utility SPL composite named IAMAccessTokenGenerator available in this toolkit will be able to generate the IAM access token and then subsequently refresh that token to keep it valid. A Streams application employing this toolkit can make use of that utility composite to generate the necessary IAM access token needed in the public cloud. Please do more reading about the IAM access token from [here](https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-websockets#WSopen).

3. On the IBM Streams application development machine (where the application code is compiled to create the application bundle), it is necessary to download and install the toolkit release bundle. The toolkit release bundle contains the required external libraries: boost, websocketpp and rapidjson. Please note that this is not needed on the Streams application execution machines. For the essential steps to meet this requirement, please refer to the above-mentioned documentation URL or a file named sttgateway-tech-brief.txt available at this tooolkit's top-level directory.

4. On the IBM Streams application machines, please ensure that libcurl is installed. This is required by this toolkit to generate and refresh the IAM access token which is a must for the STT service on public cloud.

5. For the IBM Streams and the IBM Voice Gateway products to work together, certain configuration steps must be done in both the products. For more details on that, please refer to this toolkit's documentation URL or the sttgateway-tech-brief.txt available at this tooolkit's top-level directory.

## External libraries used
* boost 1.73.0
* websocketpp 0.8.2
* rapidjson 1.1.0

## Example usage of this toolkit inside a Streams application
Here is a code snippet that shows how to invoke the **WatsonSTT** operator available in this toolkit with a subset of supported features:

```
use com.ibm.streamsx.sttgateway.watson::*;

/*
Invoke one or more instances of the WatsonSTT operator.
You can send the audio data to this operator all at once or 
you can send the audio data for the live-use case as it becomes
available from your telephony infrastructure.
Avoid feeding audio data coming from more than one data source into this 
parallel region which may cause erroneous transcription results.

NOTE: The WatsonSTT operator allows fusing multiple instances of
this operator into a single PE. This will help in reducing the 
total number of CPU cores used in running the application.
*/
@parallel(width = $numberOfSTTEngines, 
partitionBy=[{port=ABC, attributes=[conversationId]}], broadcast=[AT])
(stream<STTResult_t> STTResult) as STT = WatsonSTT(AudioBlobContent as ABC; IamAccessToken, AccessTokenForCP4D as AT) {
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

A built-in example inside this toolkit can be compiled and launched with the default STT options to use the STT service on public cloud as shown below.
The sample AudioFileWatsonSTT required that the stt service connection details are provided as application configuration properties. To create the 
application configuration enter:
```
streamtool mkappconfig --description 'connection configuration for IBM Cloud Watson stt service' \
	--property 'apiKey=<your api key>' \
	--property 'iamTokenURL=https://iam.cloud.ibm.com/identity/token' \
	--property 'url=<your stt instance uri>' \
	sttConnection
```

```
cd   streamsx.sttgateway/samples/AudioFileWatsonSTT
make
st  submitjob  -d  <YOUR_STREAMS_DOMAIN>  -i  <YOUR_STREAMS_INSTANCE>  output/com.ibm.streamsx.sttgateway.sample.watsonstt.AudioFileWatsonSTT.sab
```

Following IBM Streams job sumission command shows how to override the default values with your own as needed for the various STT options:

```
cd   streamsx.sttgateway/samples/AudioRawWatsonSTT
make
st submitjob  -d  <YOUR_STREAMS_DOMAIN>  -i  <YOUR_STREAMS_INSTANCE>  output/com.ibm.streamsx.sttgateway.sample.watsonstt.AudioRawWatsonSTT.sab -P  sttApiKey=<YOUR_WATSON_STT_SERVICE_API_KEY>  -P sttBaseLanguageModel=en-US_NarrowbandModel  -P contentType="audio/wav"    -P filterProfanity=true   -P keywordsSpottingThreshold=0.294   -P keywordsToBeSpotted="['country', 'learning', 'IBM', 'model']"   -P smartFormattingNeeded=true   -P wordAlternativesThreshold=0.251   -P maxUtteranceAlternatives=5   -P audioBlobFragmentSize=32768   -P sttLiveMetricsUpdateNeeded=true  -P audioDir=<YOUR_AUDIO_FILES_DIRECTORY>   -P numberOfSTTEngines=100
```

Following is another way to run the same application to access the STT service on the IBM Cloud Pak for Data (CP4D). STT URI shown below is for an illustrative purpose and you must use a valid STT URI obtained from your CP4D cluster.

```
st  submitjob  -d  <YOUR_STREAMS_DOMAIN>  -i  <YOUR_STREAMS_INSTANCE>  output/com.ibm.streamsx.sttgateway.sample.watsonstt.AudioFileWatsonSTT.sab  -P  sttOnCP4DAccessToken=<YOUR_CP4D_STT_SERVICE_ACCESS_TOKEN>  -P  sttUri=wss://b0610b07:31843/speech-to-text/ibm-wc/instances/1567608964/api/v1/recognize 
```

If you are planning to ingest the speech data from live voice calls, then you can invoke the **IBMVoiceGatewaySource** operator as shown below.

```
(stream<BinarySpeech_t> BinarySpeechData as BSD;
 stream<EndOfCallSignal_t> EndOfCallSignal as EOCS) as VoiceGatewayInferface = 
 IBMVoiceGatewaySource() {
    logic
       state: {
          // Initialize the default TLS certificate file name if the 
          // user didn't provide his or her own.
          rstring _certificateFileName = 
             ($certificateFileName != "") ?
              $certificateFileName : getThisToolkitDir() + "/etc/ws-server.pem";
       }
				
       param
          tlsPort: $tlsPort;
          certificateFileName: _certificateFileName;
          initDelay: $initDelayBeforeSendingDataToSttEngines;
			
       // Get these values via custom output functions provided by this operator.
       output
          BSD: vgwSessionId = getIBMVoiceGatewaySessionId(),
          isCustomerSpeechData = isCustomerSpeechData(),
          vgwVoiceChannelNumber = getVoiceChannelNumber(),
          callerPhoneNumber = getCallerPhoneNumber(),
          agentPhoneNumber = getAgentPhoneNumber(),
          speechDataFragmentCnt = getTupleCnt(),
          totalSpeechDataBytesReceived = getTotalSpeechDataBytesReceived();
}

```

In addition to the code snippet shown above to invoke the IBMVoiceGatewaySource operator, one must do additional logic to allocate a dedicated WatsonSTT operator instance for each voice channel in a given call. A demo application is available for this toolkit has that logic which can be reused in any other application. That particular example can be compiled and launched to ingest speech data from the IBM Voice Gateway for seven concurrent voice calls and send it to the WatsonSTT operator running with most of the default STT options to use the STT service on public cloud as shown below.

```
cd   streamsx.sttgateway/samples/VoiceGatewayToStreamsToWatsonSTT
make
st  submitjob  -d  <YOUR_STREAMS_DOMAIN>  -i  <YOUR_STREAMS_INSTANCE>  output/com.ibm.streamsx.sttgateway.sample.watsonstt.VoiceGatewayToStreamsToWatsonSTT.sab -P tlsPort=9443  -P numberOfSTTEngines=14  -P sttApiKey=<YOUR_WATSON_STT_SERVICE_API_KEY>  -P contentType="audio/mulaw;rate=8000"
```

**Special Note**
For those customers who are using the speech to text engine embedded in the com.ibm.streams.speech2text.watson::WatsonS2T operator, the following example is available as a reference application to exploit that operator in a real-time voice call analytics scenario. It can be compiled and executed as shown below. You have to replace the hardcoded paths and IP addresses to suit your environment.

```
cd   streamsx.sttgateway/samples/VoiceGatewayToStreamsToWatsonS2T
make
st submitjob -P tlsPort=9443 -P vgwSessionLoggingNeeded=false -P numberOfS2TEngines=4 -P WatsonS2TConfigFile=/home/streamsadmin/toolkit.speech2text-v2.12.0/model/en_US.8kHz.general.diarization.low_latency.pset -P WatsonS2TModelFile=$HOME/toolkit.speech2text-v2.12.0/model/en_US.8kHz.general.pkg -P ipv6Available=false -P writeTranscriptionResultsToFiles=true -P sendTranscriptionResultsToHttpEndpoint=true -P httpEndpointForSendingTranscriptionResults=http://172.30.105.11:9080/sttresults/Receiver/ports/output/0/inject output/com.ibm.streamsx.sttgateway.sample.watsons2t.VoiceGatewayToStreamsToWatsonS2T.sab
```

## WHATS NEW

see file com.ibm.streamsx.sttgateway/CHANGELOG.md

