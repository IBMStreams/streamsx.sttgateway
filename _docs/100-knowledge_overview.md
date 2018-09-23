---
title: "Toolkit Overview [Technical]"
permalink: /docs/knowledge/overview/
excerpt: "Basic knowledge of the toolkit's technical domain."
last_modified_at: 2018-09-23T12:37:48+01:00
redirect_from:
   - /theme-setup/
sidebar:
   nav: "knowledgedocs"
---
{% include toc %}
{% include editme %}

## Purpose of this toolkit

The streamsx.sttgateway toolkit provides an operator that helps you to integrate IBM Streams applications with the [IBM Watson Speech To Text (STT) cloud service](https://www.ibm.com/watson/services/speech-to-text/). This toolkit lets you send audio data to the Watson STT service and get back the transcription results. This toolkit is designed to work with the IBM Watson STT service available on IBM public cloud and on IBM Cloud Private (ICP). It uses the Websocket communication API interface provided by that Watson STT service. It can take audio data either from the real-time speech conversations or from the prerecorded speech conversations that are stored in files.

## Technical positioning of this toolkit
At a very high level, this toolkit shares the same design goal as the other IBM Streams toolkit named com.ibm.streams.speech2text to convert speech data into text. But, they both work very differently to realize that design goal. IBM Streams users can select either of these two toolkits depending on their application and hardware infrastructure needs. So, it is important to know the following major comparisons between these two toolkits before choosing the suitable one for a given situation.

| No. | Comparison topic | com.ibm.streams.speech2text | com.ibm.streamsx.sttgateway |
| --- | --- | --- | --- |
| 1 | Namespace | `com.ibm.streams.speech2text.watson` (Wraps Rapid libraries) | `com.ibm.streamsx.sttgateway.watson` (Invokes the IBM Watson STT cloud service) |
| 2 | Operator name | `WatsonS2T` is the operator name. | `WatsonSTT` is the operator name. |
| 3 | Self contained versus external service invocation | It is self contained. It includes the speech to text core libraries and a small number of base language model files that are necessary to do the speech to text conversion. | It relies on an external STT service. It contains the necessary code to communicate with the external STT service to get the speech to text conversion done. |
| 4 | Speech data staying within your application versus going outside | Audio content will not have to go outside of this toolkit. | Audio content will go outside of this toolkit via the network to the external service that can run either on the IBM public cloud or inside your own private network in an ICP container. |
| 5 | Ease of customizing the LM and AM models | Model customization (for both LM and AM) requires the user to do the customization either on the IBM public cloud or on-prem and then copy the generated custom model patch files into the Streams machine so that they are locally accessible by the Streams application. | LM and AM model customization is done using the IBM cloud tools/infrastructure. Such customized LM and AM models can be used via the WatsonSTT operator parameters without a need to obtain the copies of the generated custom model patch files into the Streams application machine. |
| 6 | Ease of switching between different language models and minimizing toolkit upgrade cycles | It is necessary to package relevant language base models (English, Spanish, French etc.) as part of this toolkit. This also implies that a toolkit upgrade will be needed for using newer versions of the models as well as the newer versions of the speech engine libraries. | There is no packaging of the language model files needed inside the toolkit. Switching between different language models is simply done through an operator parameter. There is no need to upgrade this toolkit for using the newer versions of the models as well as the newer versions of the speech engine libraries as they become available in the external Watson STT service. |
| 7 | Ease of using different audio formats | it may be possible to use only WAV formatted audio data. | Any supported audio format by the STT service can be used (mp3, WAV, FLAC etc.) |
| 8 | Speech to text conversion performance | In the lab tests, both toolkits seem to provide the same level of performance in terms of the total time taken to complete the transcription of the given audio content. | However, it is important to note that under certain batch workloads, this toolkit  seems to complete the transcription faster than the `com.ibm.streams.speech2text`. |
| 9 | RPM dependency | `atlas` and `atlas-devel` RPMs for Linux must be installed in all the Streams machines where the `WatsonS2T` operator is running. | There is no such RPM dependency for the `WatsonSTT` operator. |
| 10 | CPU core usage | Every instance of the `WatsonS2T` operator must run on its own PE (a.k.a Linux process). Multiple instances of that operator can't be fused to pack them into a single Linux process. This will result in using many CPU cores when there is a need to have a bank of `WatsonS2T` operators to achieve data parallelism. | There is no such restriction to fuse multiple `WatsonSTT` operators into a single PE (Linux process). This will result in the efficient use of the CPU cores by requiring fewer CPU cores to have a bank of `WatsonSTT` operators to achieve data parallelism. |
| 11 | Transcription result options | The `WatsonS2T` operator can only return one full utterance at a time. | The `WatsonSTT` operator can be configured at runtime to return one of these three possibilities as the transcription result: <span style="color:red">_**1. Partial utterances as the transcription is in progress**_</span>, <span style="color:green">_**2. Only finalized a.k.a. completed utterances**_</span>, <span style="color:blue">_**3. (default) Full text containing all the finalized utterances after transcribing the entire audio.**_</span> |
| 12 | Value added features | The `WatsonS2T` operator can only provide the core speech to text capabilities. | The `WatsonSTT` operator can optionally provide a few value added features in addition to the core speech to text capabilities. Such features include `smart formatting`, `profanity filtering`, `keywords spotting` etc. |

## Requirements for this toolkit
There are certain important requirements that need to be satisfied in order to use the IBM Streams STT Gateway toolkit in Streams applications. Such requirements are explained below.

1. Network connectivity to the Watson Speech To Text (STT) service running either on the public or the private cloud is needed from the IBM Streams Linux machines where this toolkit will be used.
   
2. A valid authentication token is needed to use the Watson STT service. This toolkit uses Websocket to communicate with the Watson STT cloud service. For that Websocket interface, one must use the auth tokens and not the usual cloud service credentials. So, users of this toolkit must generate their own authentication token and provide it when launching the Streams application(s) that will have a dependency on this toolkit. To generate your own auth token, please do more reading from [here](https://console.bluemix.net/docs/services/speech-to-text/input.html#tokens).

3. On the IBM Streams application development machine (where the application code is compiled to create the application bundle), it is necessary to download and install the boost_1_67_0 or a higher version as well as the websocketpp version 0.8.1. Please note that this is not needed on the Streams application execution machines. For the necessary steps to meet this requirement, please refer to the section titled "Toolkit Usage Overview".
