#!/usr/bin/bash
#
#--------------------------------------------------------------------
# First created on: Nov/27/2020
# Last modified on: Nov/28/2020
#
# This is a script that I used for testing the VgwDataRouterToWatsonSTT in
# our IBM Streams lab in New York. This script is set up to start
# 10 speech processor jobs each configured to run with 4 speech engines.
# They will provide a combined total of 40 speech engines to support
# 20 concurrent voice calls. You can make minor changes here and
# use it in your test environment. Before running this script, you must
# ensure that you started the VgwDataRouter application using a script
# available in that project's etc sub-directory.
#
# After launching this appliction by using this script, it will take a 
# few minutes for the multiple speech processor jobs and the speech engines
# invoked by them to come up clean. So, you must ensure that all the PEs (Processing Elements)
# are cleanly running before the voice calls' speech data can be sent through these
# jobs for processing. You can run the following command and ensure that it is not
# showing any PE in "Healthy-->no" state.
#
# st lspes -d d1 -i i4 | grep -i no
# 
# To learn more high-level details about this application, you can read the
# commentary section at the top of the SPL file for this application.
# streamsx.sttgateway/samples/VgwDataRouterToWatsonS2T/com.ibm.streamsx.sttgateway.sample.watsonstt/VgwDataRouterToWatsonSTT.spl
#
# IMPORTANT
# ---------
# Before using this script, you must first build all the examples provided in
# the samples sub-directory of this toolkit. You should first run 'make' from 
# every example directory to complete the build process and then you can use 
# this particular script by customizing it to suit your IBM Streams test environment.
#--------------------------------------------------------------------
#
echo Starting 10 speech processor jobs on Streams instance i4.
# Start 10 speech processor jobs on Streams instance i4
# [Please note that this application offers many submission time parameters. We are using only a few here.]
# Before running this script, you must substitute the correct value below for <YOUR_WATSON_STT_API_KEY>.
#
streamtool submitjob -d d1 -i i4 ~/workspace32/VgwDataRouterToWatsonSTT/output/com.ibm.streamsx.sttgateway.sample.watsonstt.VgwDataRouterToWatsonSTT.sab --jobname VgwDataRouterToWatsonSTT1 -P idOfThisSpeechProcessor=1 -P vgwDataRouterTxUrl=wss://h0201b09:8444/1 -P numberOfSTTEngines=4 -P sttApiKey=<YOUR_WATSON_STT_API_KEY> -P contentType="audio/mulaw;rate=8000" -P writeTranscriptionResultsToFiles=true -P sendTranscriptionResultsToHttpEndpoint=true -P httpEndpointForSendingTranscriptionResults=https://b0517:9443 -P tlsAcceptAllCertificates=true -P writeResultsToVoiceChannelFiles=true -C fusionScheme=legacy 
streamtool submitjob -d d1 -i i4 ~/workspace32/VgwDataRouterToWatsonSTT/output/com.ibm.streamsx.sttgateway.sample.watsonstt.VgwDataRouterToWatsonSTT.sab --jobname VgwDataRouterToWatsonSTT2 -P idOfThisSpeechProcessor=2 -P vgwDataRouterTxUrl=wss://h0201b09:8444/2 -P numberOfSTTEngines=4 -P sttApiKey=<YOUR_WATSON_STT_API_KEY> -P contentType="audio/mulaw;rate=8000" -P writeTranscriptionResultsToFiles=true -P sendTranscriptionResultsToHttpEndpoint=true -P httpEndpointForSendingTranscriptionResults=https://b0517:9443 -P tlsAcceptAllCertificates=true -P writeResultsToVoiceChannelFiles=true -C fusionScheme=legacy 
streamtool submitjob -d d1 -i i4 ~/workspace32/VgwDataRouterToWatsonSTT/output/com.ibm.streamsx.sttgateway.sample.watsonstt.VgwDataRouterToWatsonSTT.sab --jobname VgwDataRouterToWatsonSTT3 -P idOfThisSpeechProcessor=3 -P vgwDataRouterTxUrl=wss://h0201b09:8444/3 -P numberOfSTTEngines=4 -P sttApiKey=<YOUR_WATSON_STT_API_KEY> -P contentType="audio/mulaw;rate=8000" -P writeTranscriptionResultsToFiles=true -P sendTranscriptionResultsToHttpEndpoint=true -P httpEndpointForSendingTranscriptionResults=https://b0517:9443 -P tlsAcceptAllCertificates=true -P writeResultsToVoiceChannelFiles=true -C fusionScheme=legacy 
streamtool submitjob -d d1 -i i4 ~/workspace32/VgwDataRouterToWatsonSTT/output/com.ibm.streamsx.sttgateway.sample.watsonstt.VgwDataRouterToWatsonSTT.sab --jobname VgwDataRouterToWatsonSTT4 -P idOfThisSpeechProcessor=4 -P vgwDataRouterTxUrl=wss://h0201b09:8444/4 -P numberOfSTTEngines=4 -P sttApiKey=<YOUR_WATSON_STT_API_KEY> -P contentType="audio/mulaw;rate=8000" -P writeTranscriptionResultsToFiles=true -P sendTranscriptionResultsToHttpEndpoint=true -P httpEndpointForSendingTranscriptionResults=https://b0517:9443 -P tlsAcceptAllCertificates=true -P writeResultsToVoiceChannelFiles=true -C fusionScheme=legacy 
streamtool submitjob -d d1 -i i4 ~/workspace32/VgwDataRouterToWatsonSTT/output/com.ibm.streamsx.sttgateway.sample.watsonstt.VgwDataRouterToWatsonSTT.sab --jobname VgwDataRouterToWatsonSTT5 -P idOfThisSpeechProcessor=5 -P vgwDataRouterTxUrl=wss://h0201b09:8444/5 -P numberOfSTTEngines=4 -P sttApiKey=<YOUR_WATSON_STT_API_KEY> -P contentType="audio/mulaw;rate=8000" -P writeTranscriptionResultsToFiles=true -P sendTranscriptionResultsToHttpEndpoint=true -P httpEndpointForSendingTranscriptionResults=https://b0517:9443 -P tlsAcceptAllCertificates=true -P writeResultsToVoiceChannelFiles=true -C fusionScheme=legacy 
streamtool submitjob -d d1 -i i4 ~/workspace32/VgwDataRouterToWatsonSTT/output/com.ibm.streamsx.sttgateway.sample.watsonstt.VgwDataRouterToWatsonSTT.sab --jobname VgwDataRouterToWatsonSTT6 -P idOfThisSpeechProcessor=6 -P vgwDataRouterTxUrl=wss://h0201b09:8444/6 -P numberOfSTTEngines=4 -P sttApiKey=<YOUR_WATSON_STT_API_KEY> -P contentType="audio/mulaw;rate=8000" -P writeTranscriptionResultsToFiles=true -P sendTranscriptionResultsToHttpEndpoint=true -P httpEndpointForSendingTranscriptionResults=https://b0517:9443 -P tlsAcceptAllCertificates=true -P writeResultsToVoiceChannelFiles=true -C fusionScheme=legacy 
streamtool submitjob -d d1 -i i4 ~/workspace32/VgwDataRouterToWatsonSTT/output/com.ibm.streamsx.sttgateway.sample.watsonstt.VgwDataRouterToWatsonSTT.sab --jobname VgwDataRouterToWatsonSTT7 -P idOfThisSpeechProcessor=7 -P vgwDataRouterTxUrl=wss://h0201b09:8444/7 -P numberOfSTTEngines=4 -P sttApiKey=<YOUR_WATSON_STT_API_KEY> -P contentType="audio/mulaw;rate=8000" -P writeTranscriptionResultsToFiles=true -P sendTranscriptionResultsToHttpEndpoint=true -P httpEndpointForSendingTranscriptionResults=https://b0517:9443 -P tlsAcceptAllCertificates=true -P writeResultsToVoiceChannelFiles=true -C fusionScheme=legacy 
streamtool submitjob -d d1 -i i4 ~/workspace32/VgwDataRouterToWatsonSTT/output/com.ibm.streamsx.sttgateway.sample.watsonstt.VgwDataRouterToWatsonSTT.sab --jobname VgwDataRouterToWatsonSTT8 -P idOfThisSpeechProcessor=8 -P vgwDataRouterTxUrl=wss://h0201b09:8444/8 -P numberOfSTTEngines=4 -P sttApiKey=<YOUR_WATSON_STT_API_KEY> -P contentType="audio/mulaw;rate=8000" -P writeTranscriptionResultsToFiles=true -P sendTranscriptionResultsToHttpEndpoint=true -P httpEndpointForSendingTranscriptionResults=https://b0517:9443 -P tlsAcceptAllCertificates=true -P writeResultsToVoiceChannelFiles=true -C fusionScheme=legacy 
streamtool submitjob -d d1 -i i4 ~/workspace32/VgwDataRouterToWatsonSTT/output/com.ibm.streamsx.sttgateway.sample.watsonstt.VgwDataRouterToWatsonSTT.sab --jobname VgwDataRouterToWatsonSTT9 -P idOfThisSpeechProcessor=9 -P vgwDataRouterTxUrl=wss://h0201b09:8444/9 -P numberOfSTTEngines=4 -P sttApiKey=<YOUR_WATSON_STT_API_KEY> -P contentType="audio/mulaw;rate=8000" -P writeTranscriptionResultsToFiles=true -P sendTranscriptionResultsToHttpEndpoint=true -P httpEndpointForSendingTranscriptionResults=https://b0517:9443 -P tlsAcceptAllCertificates=true -P writeResultsToVoiceChannelFiles=true -C fusionScheme=legacy 
streamtool submitjob -d d1 -i i4 ~/workspace32/VgwDataRouterToWatsonSTT/output/com.ibm.streamsx.sttgateway.sample.watsonstt.VgwDataRouterToWatsonSTT.sab --jobname VgwDataRouterToWatsonSTT10 -P idOfThisSpeechProcessor=10 -P vgwDataRouterTxUrl=wss://h0201b09:8444/10 -P numberOfSTTEngines=4 -P sttApiKey=<YOUR_WATSON_STT_API_KEY> -P contentType="audio/mulaw;rate=8000" -P writeTranscriptionResultsToFiles=true -P sendTranscriptionResultsToHttpEndpoint=true -P httpEndpointForSendingTranscriptionResults=https://b0517:9443 -P tlsAcceptAllCertificates=true -P writeResultsToVoiceChannelFiles=true -C fusionScheme=legacy 
