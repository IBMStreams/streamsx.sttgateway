---
title: "Toolkit Overview [Technical]"
permalink: /docs/knowledge/overview/
excerpt: "Basic knowledge of the toolkit's technical domain."
last_modified_at: 2019-11-14T08:15:48+01:00
redirect_from:
   - /theme-setup/
sidebar:
   nav: "knowledgedocs"
---
{% include toc %}
{% include editme %}

## Purpose of this toolkit

The streamsx.sttgateway toolkit provides the following two operators that can help you to integrate an IBM Streams applicaion with the [IBM Voice Gateway product v1.0.3.0 or higher](https://www.ibm.com/support/knowledgecenter/SS4U29/whatsnew.html) at the point of data ingestion and with the [IBM Watson Speech To Text (STT) cloud service](https://www.ibm.com/watson/services/speech-to-text/) at the analytic stage(s) of that application pipeline.

1. IBMVoiceGatewaySource
2. WatsonSTT

**IBMVoiceGatewaySource** is a source operator that can be used to ingest speech data from multiple live telephone conversations happening between different pairs of speakers e-g: customers and call center agents.

**WatsonSTT** is an analytic operator that can be used to transcribe speech data into text either in real-time or in batch mode.

In a Streams application, these two operators can either be used together or independent of each other. In a real-time speech analytics scenario for transcribing live voice calls into text, both of these operators can be used as part of a Streams application graph. In a batch mode operation to convert a set of prerecorded audio files stored in a file system directory into text, WatsonSTT operator can be used on its own in a Streams application graph. So, your application requirements will determine whether you will need only one or both of those operators.

This toolkit excels at ingesting speech data from an enterprise telephony infrastructure and from audio files (wav, mp3 etc.) as well as in sending the speech data to the Watson STT service and get back the transcription results. Based on the user preference, this toolkit is designed to work with the IBM Watson STT service available on the IBM public cloud or on the hybrid cloud ready IBM Cloud Pak for Data (CP4D). It uses the Websocket communication API interface to interact with both the IBM Voice Gateway and the Watson STT service. So, processing of the audio data either from the real-time speech conversations or from the prerecorded speech conversations that are stored in files can be accomplished by this toolkit.

## Architectural patterns enabled by this toolkit
1. For the **real-time** speech to text transcription, following are the possible architectural patterns.
- <span style="color:green">Your Telephony SIPREC-->IBM Voice Gateway-->IBM Streams<-->Watson Speech To Text on IBM Public Cloud</span>
- <span style="color:blue">Your Telephony SIPREC-->IBM Voice Gateway-->IBM Streams<-->Watson Speech To Text on IBM Cloud Pak for Data (CP4D)</span>
- <span style="color:purple">Your Telephony SIPREC-->IBM Voice Gateway-->IBM Streams<-->Watson Speech To Text engine embedded inside an IBM Streams operator</span>
2. For the **batch (post call)** speech to text transcription, following are the possible architectural patterns.
- <span style="color:green">Speech data files in a directory-->IBM Streams<-->Watson Speech To Text on IBM Public Cloud</span>
- <span style="color:blue">Speech data files in a directory-->IBM Streams<-->Watson Speech To Text on IBM Cloud Pak for Data (CP4D)</span>
- <span style="color:purple">Speech data files in a directory-->IBM Streams<-->Watson Speech To Text engine embedded inside an IBM Streams operator</span>

## Technical positioning of this toolkit
At a very high level, this toolkit shares the same design goal as the other IBM Streams toolkit named com.ibm.streams.speech2text to convert speech data into text. But, they both work very differently to realize that design goal. IBM Streams users can select either of these two toolkits depending on their application and hardware infrastructure needs. So, it is important to know the following major differences between these two toolkits before choosing the suitable one for a given situation.

| No. | Comparison topic | com.ibm.streams.speech2text | com.ibm.streamsx.sttgateway |
| --- | --- | --- | --- |
| 1 | Namespace | `com.ibm.streams.speech2text.watson` (Wraps Rapid libraries) | `com.ibm.streamsx.sttgateway.watson` (Invokes the IBM Watson STT cloud service) |
| 2 | Scope | Can only do speech to text. | Supports both live speech data ingestion and speech to text. |
| 3 | Operator name | `WatsonS2T` is the operator name. | `IBMVoiceGatewaySource` and `WatsonSTT` are the operator names. |
| 4 | Self contained versus external service invocation | It is self contained. It includes the speech to text core libraries and a small number of base language model files that are necessary to do the speech to text conversion. | It relies on an external STT service. It contains the necessary code to communicate with the external STT service to get the speech to text conversion done. |
| 5 | Speech data staying within your application versus going outside | Audio content will not have to go outside of this toolkit. | Audio content will go outside of this toolkit via the network to the external service that can run either on the IBM public cloud or inside your own private network in a Cloud Pak for Data CP4D cluster. |
| 6 | Ease of customizing the LM and AM models | Model customization (for both LM and AM) requires the user to do the customization either on the IBM public cloud and then copy the generated custom model patch files into the Streams machine so that they are locally accessible by the Streams application. | LM and AM model customization is done using the IBM cloud tools/infrastructure. Such customized LM and AM models can be used via the WatsonSTT operator parameters without a need to obtain the copies of the generated custom model patch files into the Streams application machine. |
| 7 | Ease of switching between different language models and minimizing toolkit upgrade cycles | It is necessary to package relevant language base models (English, Spanish, French etc.) as part of this toolkit. This also implies that a toolkit upgrade will be needed for using newer versions of the models as well as the newer versions of the speech engine libraries. | There is no packaging of the language model files needed inside the toolkit. Switching between different language models is simply done through an operator parameter. There is no need to upgrade this toolkit for using the newer versions of the models as well as the newer versions of the speech engine libraries as they become available in the external Watson STT service. |
| 8 | Ease of using different audio formats | it may be possible to use only WAV formatted audio data. | Any supported audio format by the STT service can be used (mp3, WAV, FLAC, mulaw etc.) |
| 9 | Speech to text conversion performance | In the lab tests, both toolkits seem to provide the same level of performance in terms of the total time taken to complete the transcription of the given audio content. | However, it is important to note that under certain batch workloads, this toolkit  seems to complete the transcription faster than the `com.ibm.streams.speech2text`. |
| 10 | RPM dependency | `atlas` and `atlas-devel` RPMs for Linux must be installed in all the Streams machines where the `WatsonS2T` operator is running. | There is no such RPM dependency for the `WatsonSTT` operator. |
| 11 | CPU core usage | Every instance of the `WatsonS2T` operator must run on its own PE (a.k.a Linux process). Multiple instances of that operator can't be fused to pack them into a single Linux process. This will result in using many CPU cores when there is a need to have a bank of `WatsonS2T` operators to achieve data parallelism. | There is no such restriction to fuse multiple `WatsonSTT` operators into a single PE (Linux process). This will result in the efficient use of the CPU cores by requiring fewer CPU cores to have a bank of `WatsonSTT` operators to achieve data parallelism. |
| 12 | Transcription result options | The `WatsonS2T` operator can only return one full utterance at a time. | The `WatsonSTT` operator can be configured at runtime to return one of these three possibilities as the transcription result: <span style="color:red">_**1. Partial utterances as the transcription is in progress**_</span>, <span style="color:green">_**2. Only finalized a.k.a. completed utterances**_</span>, <span style="color:blue">_**3. (default) Full text containing all the finalized utterances after transcribing the entire audio.**_</span> |
| 13 | Value added features | The `WatsonS2T` operator can only provide the core speech to text capabilities. | The `WatsonSTT` operator can optionally provide a few value added features in addition to the core speech to text capabilities. Such features include `smart formatting`, `profanity filtering`, `keywords spotting` etc. |
| 14 | Availability |  Can be downloaded only by the customers who possess active Streams licenses. |  Can be downloaded by anyone for free from the IBMStreams GitHub web site. |

## Requirements for this toolkit
There are certain important requirements that need to be satisfied in order to use the IBM Streams STT Gateway toolkit in Streams applications. Such requirements are explained below.

1. Network connectivity to the IBM Watson Speech To Text (STT) service running either on the public cloud or on the Cloud Pak for Data (CP4D) is needed from the IBM Streams Linux machines where this toolkit will be used. The same is true to integrate with the IBM Voice Gateway product for the use cases involving speech data ingestion for live voice calls.

2. This toolkit uses Websocket to communicate with the IBM Voice Gateway and the Watson STT service. A valid IAM access token is needed to use the Watson STT service on the public cloud and a valid access token to use the Watson STT service on the CP4D. So, users of this toolkit must provide their public cloud STT service instance's API key or the CP4D STT service instance's access token when launching the Streams application(s) that will have a dependency on this toolkit. When using the API key from the public cloud, a utility SPL composite named IAMAccessTokenGenerator available in this toolkit will be able to generate the IAM access token and then subsequently refresh that token to keep it valid. A Streams application employing this toolkit can make use of that utility composite to generate the necessary IAM access token needed in the public cloud. Please do more reading about the 
IAM access token from [here](https://cloud.ibm.com/docs/services/speech-to-text?topic=speech-to-text-websockets#WSopen).

3. On the IBM Streams application development machine (where the application code is compiled to create the application bundle), it is necessary to download and install the boost_1_69_0 as well as the websocketpp version 0.8.1. Please note that this is not needed on the Streams application execution machines. For the necessary steps to meet this requirement, please refer to the section titled "Toolkit Usage Overview".

4. On the IBM Streams application machines, please ensure that you can run the Linux curl command. The utility composite mentioned in a previous paragraph will use the Linux curl command to generate and refresh the IAM access token for using the STT service on public cloud. So, curl command should work on all the Streams application machines.

5. For the IBM Streams and the IBM Voice Gateway products to work together, certain configuration steps must be done as explained here.

- a) In order to make the IBM Voice Gateway send the speech data from the live voice calls, it is necessary to set the following environment variable in the deployment configuration of the IBM Voice Gateway's SIP Orchestrator at the time of deploying it.
```
{
   "name": "SEND_SIPREC_METADATA_TO_STT",
   "value": "true"
}
```

- b) It is also necessary to set the following environment variable in the IBM Voice Gateway's Media Relay deployment configuration to point to the URL at which the IBMVoiceGatewaySource operator is running.
```
{
   "name": "WATSON_STT_URL",
   "value": "https://IP-Address-Or-The-Streams-Machine-Name-Where-IBMVoiceGatewaySource-Is-Running:Port"
}
```

- c) It is necessary to create a self signed or Root CA signed TLS/SSL certificate in PEM format and point to that certificate file at the time of starting the IBM Streams application that invokes the IBMVoiceGatewaySource operator present in this toolkit. If you don't want to keep pointing to your TLS/SSL certificate file everytime you start the IBM Streams application, you can also copy the full certificate file to your Streams application's etc directory as ws-server.pem and compile your application which will then be used by default.

- d) You should also extract just the certificate portion without your private key from your full TLS/SSL certificate PEM file that you created in the previous step and then create a security credential in the IBM Voice Gateway container as explained in that product's documentation. After that, you should point to that security credential in the deployment configuration of the IBM Voice Gateway's Media Relay.

- e) If you are comfortable with using a self signed TLS/SSL certificate file in your environment, you can follow the instructions given in the following file that is shipped with this toolkit to create your own self signed SSL certificate.

```
<YOUR_STTGATEWAY_TOOLKIT_HOME>/samples/VoiceGatewayToStreamsToWatsonSTT/etc/creating-a-self-signed-certificate.txt
```
