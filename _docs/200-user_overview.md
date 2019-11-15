---
title: "Toolkit Usage Overview"
permalink: /docs/user/overview/
excerpt: "How to use this toolkit."
last_modified_at: 2019-11-14T08:28:48+01:00
redirect_from:
   - /theme-setup/
sidebar:
   nav: "userdocs"
---
{% include toc %}
{%include editme %}

## Satisfying the toolkit requirements
As explained in the "Toolkit Overview [Technical]" section, this toolkit requires network connectivity to the IBM Voice Gateway and the Watson STT service running either on the IBM public cloud or on the Cloud Pak for Data (CP4D).  In addition, a user specific IAM access token to invoke the Watson STT service on public cloud. In order to generate and refresh the IAM access token needed in the public cloud, this toolkit uses the Linux curl command. So, it is necessary to have the curl command working on all the IBM Streams application machines. If you are using the Watson STT on CP4D, then you will need the regular access token obtained from the CP4D web console instead of the IAM access token. In addition, it also requires you to download and install the boost_1_69_0 as well as the websocketpp version 0.8.1 on the IBM Streams application development machine where the application code is compiled to create the application bundle. These two C++ libraries form the major external dependency for this toolkit. 

Bulk of the Websocket logic in this toolkit's operator relies on the following open source C++ Websocket header only library.
[websocket++](https://github.com/zaphoyd/websocketpp)

This toolkit requires the following two open source packages that are not shipped with this toolkit due to the open source code distribution policies. Users of this toolkit must first understand the usage clauses stipulated by these two packages and then bring these open source packages on their own inside of this toolkit as explained below. This needs to be done only on the Linux machine(s) where the Streams application development i.e. coding, compiling and packaging is done. Only after doing that, users can use this toolkit in their Streams applications.

1. boost_1_69_0
   - Obtain the official boost version boost_1_69_0 from here:
           https://www.boost.org/users/history/version_1_69_0.html
   
   - A few .so files from the boost_1_69_0/lib directory are copied into the `impl/lib` directory of this toolkit.
       - (It is needed for the dynamic loading of these .so files when the Streams application using this toolkit is launched.)
       
   - The entire boost_1_69_0/include directory is copied into the `impl/include` directory of this toolkit. [Around 200 MB in size]
       - (It is needed for a successful compilation of the Streams application that uses this toolkit. Please note that these include files will not bloat the size of that application's SAB file since the `impl/include` directory will not be part of the SAB file.)
       
2. websocketpp v0.8.1
   - The entire websocketpp directory is copied into the `impl/include` directory of this toolkit. [Around 1.5 MB in size]
       - (It is needed for a successful compilation of the Streams application that uses this toolkit. Please note that these include files will not bloat the size of that application's SAB file  since the `impl/include` directory will not be part of the SAB file.)

## Downloading and building boost_1_69_0 or a higher version
i. Download and build boost 1_69_0 or a higher version in the user's home directory by using the --prefix option as shown below:

   - Download boost_1_69_0 in your home directory: 
      - `mkdir <YOUR_HOME_DIRECTORY>/boost-install-files`
      - `cd <YOUR_HOME_DIRECTORY>/boost-install-files`
      - `wget https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.gz` [Approximately 1 minute]

   - Extract boost_1_69_0 in `<YOUR_HOME_DIRECTORY>/boost-install-files`:
      - `cd <YOUR_HOME_DIRECTORY>/boost-install-files`
      - `tar -xvzf <YOUR_HOME_DIRECTORY>/boost-install-files/boost_1_69_0.tar.gz` [Approximately 5 minutes]

   - Bootstrap boost_1_69_0 and install it in your home directory using the --prefix option:
      - `cd <YOUR_HOME_DIRECTORY>/boost-install-files/boost_1_69_0`
      - `./bootstrap.sh --prefix=<YOUR_HOME_DIRECTORY>/boost_1_69_0` [Approximately 1 minute]
      - `./b2 install --prefix=<YOUR_HOME_DIRECTORY>/boost_1_69_0 --with=all` [Approximately 25 minutes]
      - `cd <YOUR_HOME_DIRECTORY>`
      - `rm -rf <YOUR_HOME_DIRECTORY>/boost-install-files` [Approximately 2 minutes]

   - Instructions shown above are from this URL:
      - [C++ boost install instructions](https://gist.github.com/1duo/2d1d851f76f8297be264b52c1f31a2ab)

ii. After that, copy a few .so files from the `<YOUR_HOME_DIRECTORY>/boost_1_69_0/lib` directory into the `impl/lib` directory of this toolkit.
   - (libboost_chrono.so.1.69.0, libboost_random.so.1.69.0, libboost_system.so.1.69.0, libboost_thread.so.1.69.0)
    
   - For all those .so files you copied, you must also create a symbolic link within the `impl/lib` directory of this toolkit.
      - e-g: ln   -s    libboost_chrono.so.1.69.0    libboost_chrono.so

iii. Move the entire `<YOUR_HOME_DIRECTORY>/boost_1_69_0/include/boost` directory into the `impl/include` directory of this toolkit.
   
iv. At this time, you may delete the `<YOUR_HOME_DIRECTORY>/boost_1_69_0` directory.

## Downloading websocketpp 0.8.1
i. Download websocketpp v0.8.1 from [here](https://github.com/zaphoyd/websocketpp/releases) and extract it in your home directory first. Then move the `~/websocket-0.8.1/websocketpp` directory into the `impl/include` directory of this toolkit.
   - (websocket++ is a header only C++ library which has no .so files of its own. In that way, it is very convenient.)

ii. At this time, you may delete the `~/websocket-0.8.1` directory.

## A must do in the Streams applications that will use this toolkit
i. You must add this toolkit as a dependency in your application.
   - In Streams Studio, you can add this toolkit location in the Streams Explorer view and then add this toolkit as a dependency inside your application project's Dependencies section.
       
   - In a command line compile mode, simply add the -t option to point to this toolkit's top-level or its parent directory.

   - This toolkit provides secure access to the STT service by generating an IAM access token via a utility SPL composite that can be invoked within a Streams application. That composite IAMAccessTokenGenerator has a dependency on the streamsx.json toolkit. So, it is necessary to have the streamsx.json (v1.4.6 or higher) toolkit on your development machine where you will build your application. In a command line compile mode, you have to add the -t option to point to your streamsx.json toolkit directory.
       
ii. In Streams studio, you must double click on the BuildConfig of your application's main composite and then select "Other" in the dialog that is opened. In the "C++ compiler options", you must add the following.
   - `-I <Full path to your com.ibm.streamsx.sttgateway toolkit>/impl/include`
      - (e-g): `-I /home/xyz/streamsx.sttgateway/com.ibm.streamsx.sttgateway/impl/include`
       
   - In Streams studio, you must double click on the BuildConfig of your application's main composite and then select "Other" in the dialog that is opened. In the "Additional SPL compiler options", you must add the following.
      - --c++std=c++11
       
   - If you are building your application from the command line, please refer to the Makefile provided in the AudioFileWatsonSTT example shipped with this toolkit. Before using that Makefile, you must set the STREAMS_STTGATEWAY_TOOLKIT environment variable to point to the full path of your streamsx.sttgateway/com.ibm.streamsx.sttgateway directory. Similarly, you must set the STREAMS_JSON_TOOLKIT environment variable to point to the full path of your streamsx.json (v1.4.6 or higher) toolkit directory. To build your own applications, you can do the same as done in that Makefile.

   - Please note that your IBM public cloud STT instance's API key will have to be provided via a submission time parameter into your application so that your API key can be used to generate a new IAM access token within the utility composite IAMAccessTokenGenerator mentioned above. Since the IAM access tokens will expire after a certain time period, it is necessary to keep refreshing it periodically. That utility composite does that as well. This API Key is needed only when using the STT service on public cloud. If the STT service on the IBM Cloud Pak for Data (CP4D) is used, then the API key is not needed. Instead, a never expiring access token obtained from the CP4D STT service instance's web console should be used. 

## Example usage of this toolkit inside a Streams application:
Here is a code snippet that shows how to invoke the **WatsonSTT** operator available in this toolkit for the basic features. For using the advanced features of the Watson STT service, please refer to another example code snippet shown in the Operator Usage Patterns section.

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

A built-in example inside this toolkit can be compiled and launched with the default STT options to use the STT service on public cloud as shown below:

```
cd   streamsx.sttgateway/samples/AudioFileWatsonSTT
make
st  submitjob  -d  <YOUR_STREAMS_DOMAIN>  -i  <YOUR_STREAMS_INSTANCE>  output/com.ibm.streamsx.sttgateway.sample.watsonstt.AudioFileWatsonSTT.sab  -P  sttApiKey=<YOUR_WATSON_STT_SERVICE_API_KEY>
```

Following IBM Streams job sumission command shows how to override the default values with your own as needed for the various the STT options:

```
cd   streamsx.sttgateway/samples/AudioRawWatsonSTT
make
st submitjob  -d  <YOUR_STREAMS_DOMAIN>  -i  <YOUR_STREAMS_INSTANCE>  output/com.ibm.streamsx.sttgateway.sample.watsonstt.AudioRawWatsonSTT.sab -P  sttApiKey=<YOUR_WATSON_STT_SERVICE_API_KEY>  -P sttResultMode=2   -P sttBaseLanguageModel=en-US_NarrowbandModel  -P contentType="audio/wav"    -P filterProfanity=true   -P keywordsSpottingThreshold=0.294   -P keywordsToBeSpotted="['country', 'learning', 'IBM', 'model']"   -P smartFormattingNeeded=true   -P identifySpeakers=true   -P wordTimestampNeeded=true   -P wordConfidenceNeeded=true   -P wordAlternativesThreshold=0.251   -P maxUtteranceAlternatives=5   -P audioBlobFragmentSize=32768   -P audioDir=<YOUR_AUDIO_FILES_DIRECTORY>   -P numberOfSTTEngines=100
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
			
       // Get these values via custom output functions	provided by this operator.
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

In addition to the code snippet shown above to invoke the IBMVoiceGatewaySource operator, one must do additional logic to allocate a dedicated instance of a WatsonSTT operator for a given voice call. A built-in example inside this toolkit has that logic which can be reused in any other application. That particular example can be compiled and launched to ingest speech data from the IBM Voice Gateway for seven concurrent voice calls and send it to the WatsonSTT operator running with most of the default STT options to use the STT service on public cloud as shown below:

```
cd   streamsx.sttgateway/samples/VoiceGatewayToStreamsToWatsonSTT
make
st  submitjob  -d  <YOUR_STREAMS_DOMAIN>  -i  <YOUR_STREAMS_INSTANCE>  output/com.ibm.streamsx.sttgateway.sample.watsonstt.VoiceGatewayToStreamsToWatsonSTT.sab -P tlsPort=9443  -P numberOfSTTEngines=14  -P sttApiKey=<YOUR_WATSON_STT_SERVICE_API_KEY>  -P sttResultMode=2   -P contentType="audio/mulaw;rate=8000"
```

**Special Note**

For those customers who are using the speech to text engine embedded in the com.ibm.streams.speech2text.watson::WatsonS2T operator, the following example is available as a reference application to exploit that operator in a real-time voice call analytics scenario. It can be compiled and executed as shown below. You have to replace the hardcoded paths and IP addresses to suit your environment.

```
cd   streamsx.sttgateway/samples/VoiceGatewayToStreamsToWatsonS2T
make
st submitjob -P tlsPort=9443 -P vgwSessionLoggingNeeded=false -P numberOfS2TEngines=4 -P WatsonS2TConfigFile=/home/streamsadmin/toolkit.speech2text-v2.12.0/model/en_US.8kHz.general.diarization.low_latency.pset -P WatsonS2TModelFile=/home/streamsadmin/toolkit.speech2text-v2.12.0/model/en_US.8kHz.general.pkg -P ipv6Available=false -P writeTranscriptionResultsToFiles=true -P sendTranscriptionResultsToHttpEndpoint=true -P httpEndpointForSendingTranscriptionResults=http://172.30.105.11:9080/sttresults/Receiver/ports/output/0/inject output/com.ibm.streamsx.sttgateway.sample.watsons2t.VoiceGatewayToStreamsToWatsonS2T.sab
```

### Working examples shipped with this toolkit
As explained in the previous section, there are four examples available within this toolkit directory that can be compiled and tested by using a valid IAM access token (generated via your STT service instance's API key) required to connect to the Watson STT service on the IBM public cloud or by using the regular access token to connect to the Watson STT service on your CP4D cluster. The streamsx.sttgateway toolkit provides a utility composite operator named IAMAccessTokenGenerator that can be used within any Streams application to generate and refresh an IAM access token that stays current and valid at all times. Without a valid IAM access token, this toolkit will not be able to function correctly. So, it is necessary to understand the importance of the IAM access token. The four examples provided in this toolkit will show you how to use the utility composite operator to generate/refresh the IAM access token and send it to the WatsonSTT operator. For using the Watson STT service on your CP4D cluster, you will not require the IAM access token and you will need the regular access token displayed in the CP4D web console. The first two examples show different variations of reading and processing the batch workload from a collection of prerecorded audio files. The third example shows how to get the real-time speech data of  the live voice calls from the IBM Voice Gateway product into IBM Streams and then invoke the Watson Speech To Text service either on the public cloud or on a private cloud running CP4D. The fourth example shows how to do real-time speech to text transcription using the embedded STT engine available inside a Streams operator named com.ibm.streams.speech2text.watson::WatsonS2T. Within the same streamsx.sttgateway/samples directory where these four examples are present, there is a directory named audio_files that contains a few test audio files useful for testing the batch speech to text examples. There is also a helper application named stt_results_http_receiver which can be used to see how the real-time STT results can be streamed to an external HTTP server.

* [AudioFileWatsonSTT](https://github.com/IBMStreams/streamsx.sttgateway/tree/master/samples/AudioFileWatsonSTT)
* [AudioRawWatsonSTT](https://github.com/IBMStreams/streamsx.sttgateway/tree/master/samples/AudioRawWatsonSTT)
* [VoiceGatewayToStreamsToWatsonSTT](https://github.com/IBMStreams/streamsx.sttgateway/tree/master/samples/VoiceGatewayToStreamsToWatsonSTT)
* [VoiceGatewayToStreamsToWatsonS2T](https://github.com/IBMStreams/streamsx.sttgateway/tree/master/samples/VoiceGatewayToStreamsToWatsonS2T)
* [stt_results_http_receiver](https://github.com/IBMStreams/streamsx.sttgateway/tree/master/samples/stt_results_http_receiver)
* [audio_files](https://github.com/IBMStreams/streamsx.sttgateway/tree/master/samples/audio-files)

## Common log messages generated by the Websocket library
There will be many log messages from the underlying C++ Websocket library routinely getting written in the Streams WatsonSTT operator log files. Such log messages are about making a connection to the Watson STT service, disconnecting from the Watson STT service, receiving Websocket control frames during the ongoing transcription etc. For example, in the absence of any audio data available for transcription for a prolonged period (30 seconds or more), the Watson STT service will terminate the Websocket connection to better utilize its back-end resources. In that case, a set of messages as shown below will be logged. 

```
[2018-08-11 10:07:35] [control] Control frame received with opcode 8
[2018-08-11 10:07:35] [application] Websocket connection closed with the Watson STT service.
[2018-08-11 10:07:35] [disconnect] Disconnect close local:[1011,see the previous message for the error details.] remote:[1011,see the previous message for the error details.]
```

These messages are normal and they don't reflect any error in the Streams WatsonSTT operator. When the audio data becomes available later, the WatsonSTT operator will reestablish the Websocket connection and continue the transcription task. Only when you see any abnormal runtime exception errors, you should take actions to diagnose and fix the problem.
