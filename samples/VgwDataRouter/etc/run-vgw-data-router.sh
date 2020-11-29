#!/usr/bin/bash
#
#--------------------------------------------------------------------
# First created on: Nov/26/2020
# Last modified on: Nov/28/2020
#
# This is a script that I used for testing the VgwDataRouter application
# available in the streamsx,sttgateway/samples directory. I did the test in 
# our IBM Streams lab in New York. It demonstrates routing 150 concurrent calls
# to 10 different speech processor jobs each configured with 30 speech engines.
# You can make minor changes here and use it in your test environment. 
# It allows us to run the two WebSocket server based operators invoked from 
# this application to always get placed on a particular machine in the given 
# IBM Streams cluster. In order to achieve this function, it is necessary to 
# follow the steps described below.
#
# 1) Create a host tag on one of the application machines present in the given IBM Streams instance.
# e-g: streamtool chhost -d d1 -a --tags VgwDataRouter h0201b09
# In this example command, d1 is my IBM Streams domain name and h0201b09 is 
# one of my Streams application machines.
#
# 2) Verify that the host tag is in place now on the machine you chose.
# e-g: streamtool  getinstancestate -d d1 -i i4
# In this example command, d1 and i4 are my IBM Streams domain and instance names.
#
# 3) You must ensure that you either have a self signed TLS certificate or your 
# company provided TLS certficate for use. You can copy the pem file as ws-server.pem in
# in the etc sub-directory of this application or point to your TLS certificate file via
# the -P certificateFileName submission time parameter
#
# 4) After successfully launching the VgwDataRouter application by using this script, 
# you can confirm that the two WebSocket based operators are correctly placed on the 
# machine carrying the VgwDataRouter host tag by giving this command.
# e-g: streamtool lspes -d d1 -i i4 -l | grep VgwDataRouterS
#
# 5) After this application is up and running, now you can start one or more
# jobs for the other application named VgwDataRouterToWatsonS2T or 
# VgwDataRouterToWatsonSTT by using the script available in that
# application's etc sub-directory.
#
# To learn more high-level details about this application, you can read the
# commentary section at the top of the SPL file for this application.
# streamsx.sttgateway/samples/VgwDataRouter/com.ibm.streamsx.sttgateway.sample/VgwDataRouter.spl
#
# IMPORTANT
# ---------
# Before using this script, you must first build all the examples provided in
# the samples sub-directory of this toolkit. You should first run 'make' from 
# every example directory to complete the build process and then you can use 
# this particular script by customizing it to suit your IBM Streams test environment.
#--------------------------------------------------------------------
#
echo Starting the VGW Data Router application on Streams instance i4.
# Start the VGW Data Router application on instance i4
# [Please note that this application offers many submission time parameters. We are using only a few here.]
streamtool submitjob -d d1 -i i4 ~/workspace32/VgwDataRouter/output/com.ibm.streamsx.sttgateway.sample.VgwDataRouter.sab --jobname VgwDataRouter -P tlsPortForVgwDataRx=8443 -P tlsPortForVgwDataTx=8444 -P numberOfSpeechEnginesPerSpeechProcessorJob=30 -P totalNumberOfSpeechProcessorJobs=10 -P ipv6Available=false -C fusionScheme=legacy  

# In the absence of an IBM Voice Gateway test infrastructure, you can use the following command to 
# simulate 150 concurrent calls by activating the call replay feature available in this application. 
# In the etc sub-directory of this application, a simple tool is provided to generate as many 
# test call recordings as needed. That test call data generation tool is available in the 
# etc/call-replay-test-data-generator.tar.gz file.
#streamtool submitjob -d d1 -i i4 ~/workspace32/VgwDataRouter/output/com.ibm.streamsx.sttgateway.sample.VgwDataRouter.sab --jobname VgwDataRouter -P tlsPortForVgwDataRx=8443 -P tlsPortForVgwDataTx=8444 -P numberOfSpeechEnginesPerSpeechProcessorJob=30 -P totalNumberOfSpeechProcessorJobs=10 -P ipv6Available=false -P callRecordingReadDirectory=/homes/hny5/sen/call-recording-read -P numberOfCallReplayEngines=150 -C fusionScheme=legacy  
