#!/bin/bash
# =========================================================
# First created on: Sep/15/2021
# Last modified on: May/10/2022
#
# This is a convenience script that can be used to
# start as many speech processor jobs as you need along with
# a single vgwDataRouter job.
#
# You can customize the actual submit job commands below
# by adding, removing and modifying job submission
# parameters as needed.
#
# You can adjust the two important values by modifying
# the bash variables shown below. After that, carefully
# review the submit commands to ensure that they have the 
# correct set of values for the domain name, instance name 
# and other job submission parameters. After that verification
# is completed, you may proceed to run this script.
# =========================================================
# This holds the number of speech processor jobs needed.
total_speech_processor_jobs=5
# This holds the number of speech engines per speech processor job.
total_speech_engines_per_speech_processor_job=10
#
# Port numbers used in the VGW Data Router application.
tlsPortForVgwDataRx=9443
tlsPortForVgwDataTx=9444
# ================
current_time=`date`
start_msg="Start time: "
start_msg="$start_msg$current_time"
# ================
total_speech_processor_jobs_string="$total_speech_processor_jobs"
total_speech_engines_per_speech_processor_job_string="$total_speech_engines_per_speech_processor_job"
let number_of_result_processors=$total_speech_engines_per_speech_processor_job/2
number_of_result_processors_string="$number_of_result_processors"

# Now stay in a loop and start as many speech processor jobs as requested.
for (( idx=1; idx<=$total_speech_processor_jobs; idx++ ))
do
   idx_string="$idx"
   # Read the detailed commentary for the WebSocketSink operator invocation 
   # code in the VgwDataRouter.spl file about how the parallel instances of 
   # that operator listen on different ports.
   let parallel_channel=$idx-1 
   let vgwDataRouterTxUrlPort=$tlsPortForVgwDataTx+$parallel_channel 
   echo "Starting Speech Processor job $idx_string of $total_speech_processor_jobs_string."
   streamtool submitjob -d d1 -i i1 --jobname SpeechProcessor$idx_string -P idOfThisSpeechProcessor=$idx_string -P vgwDataRouterTxUrl=wss://<YOUR_HOST>:$vgwDataRouterTxUrlPort/$idx_string -P numberOfSTTEngines=$total_speech_engines_per_speech_processor_job_string -P numberOfResultProcessors=$number_of_result_processors_string -P includeUtteranceResultReceptionTime=true -P sttUri=<YOUR_STT_URI> -P numberOfEocsNeededForVoiceCallCompletion=1 -P contentType="audio/mulaw;rate=8000" -P redactionNeeded=true -P writeTranscriptionResultsToFiles=false ../output/com.ibm.streamsx.sttgateway.sample.watsonstt.VgwDataRouterToWatsonSTT.sab -C fusionScheme=legacy
# Wait for 7 seconds before starting the next job.
echo "Sleeping for 7 seconds."
sleep 7
done
# ================
# Now start a single VgwDataRouterMini job.
echo "Starting a single VgwDataRouter job."
streamtool submitjob -d d1  -i i1  --jobname VgwDataRouter -P tlsPortForVgwDataRx=$tlsPortForVgwDataRx -P tlsPortForVgwDataTx=$tlsPortForVgwDataTx -P totalNumberOfSpeechProcessorJobs=$total_speech_processor_jobs_string -P numberOfSpeechEnginesPerSpeechProcessorJob=$total_speech_engines_per_speech_processor_job_string -P maxConcurrentCallsAllowed=6 -P numberOfEocsNeededForVoiceCallCompletion=1 -P vgwSessionLoggingNeeded=true -P ipv6Available=true ../../VgwDataRouterMini/output/com.ibm.streamsx.sttgateway.sample.VgwDataRouter.sab  -C fusionScheme=legacy
# ================
current_time=`date`
end_msg="End time: "
end_msg="$end_msg$current_time"
echo "*******************"
echo $start_msg
echo $end_msg
echo "*******************"
# ================
