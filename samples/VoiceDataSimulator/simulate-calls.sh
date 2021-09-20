#!/bin/bash
# =========================================================
# This is a convenience script that can be used to
# simulate hundreds of concurrent voice calls very easily.
# This script takes three arguments as shown below.
# -s tells which call number you want to start from.
# i.e. start from 1st call or 10th call or 100th call etc.
# -t tells how many total calls you want to simulate.
# i.e. simulate 256 calls or 300 calls or 350 calls.
# -v tells any unique vgw session id suffix for every simulation run.
# -u tells the WebSocket URL of IBM Voice Gateway source operator
#  running in a Streams application.
# -a tells the full path of the Mulaw formatted audio file.
#
# ./simulate-calls.sh -s 1 -t 560 -v xyz -u wss://MyHost:9443 -a ./test-call.mulaw
#
# =========================================================
# This holds the total number of replayable calls to be generated.
starting_call_number=0
total_calls_needed=0
vgw_session_id_suffix=""
url=""
audio_file=""

# Number of command line options to shift out
s=0 

while getopts "s:t:v:u:a:h" options; do
    case $options in
    s) starting_call_number=$OPTARG
       let s=s+2
       ;;
    t) total_calls_needed=$OPTARG
       let s=s+2
       ;;
    v) vgw_session_id_suffix=$OPTARG
       let s=s+2
       ;;
    u) url=$OPTARG
       let s=s+2
       ;;
    a) audio_file=$OPTARG
       let s=s+2
       ;;
    h | * ) echo "
Command line arguments
  -s INTEGER      startingCallNumber  (1)
  -t INTEGER      totalCallsNeeded    (1 to 1100)
  -v STRING       vgwSessionIdSuffix  (abc)
  -u STRING       url                 (wss://MyHost1:9443)
  -a STRING       audioFileName       (test-call.mulaw)

  e-g:
  -s 1 -t 100 -v abc -u wss://MyHost1:9443 -a test-call.mulaw
"
        exit 1
        ;;
    esac
done
shift $s

#Validate the starting call number value as entered by the user.
if [ $starting_call_number -le 0 ];
then
   echo "Missing or wrong value for starting call number via the -s option."
   echo "Please specify a number greater than 0."
   echo ""
   echo "Get help using -h option."
   exit 1
fi

# Validate the total calls needed value as entered by the user.
if [ $total_calls_needed -le 0 ] ||  [ $total_calls_needed -gt "1100" ];
then
   echo "Missing or wrong value for total calls needed via the -t option."
   echo "Please specify a number from 1 to 1100."
   echo ""
   echo "Get help using -h option."
   exit 1
fi

# Validate the vgw session id suffix as entered by the user.
if [ "$vgw_session_id_suffix" == "" ];
then
   echo "Missing or wrong vgw session id suffix via the -v option."
   echo "Your vgw session id suffix must be specified."
   echo ""
   echo "Get help using -h option."
   exit 1
fi

# Validate the url as entered by the user.
if [ "$url" == "" ];
then
   echo "Missing or wrong url via the -u option."
   echo "Your url must be specified."
   echo ""
   echo "Get help using -h option."
   exit 1
fi

# Validate the audio file as entered by the user.
if [ "$audio_file" == "" ];
then
   echo "Missing or wrong audio file name via the -a option."
   echo "Your audio file name must be specified."
   echo ""
   echo "Get help using -h option."
   exit 1
fi

# ================
# echo $total_calls_needed
# echo $$vgw_session_id_suffix
# echo $url
# echo $audio_file
# exit 1
# ================
current_time=`date`
start_msg="Start time: "
start_msg="$start_msg$current_time"
echo $start_msg
# ================

# Now stay in a loop and simulate  as many calls as the user asked for.
for (( callIdx=0; callIdx<$total_calls_needed; callIdx++ ))
do
   let a=$starting_call_number+$callIdx
   # We will have four leading zeroes as needed to have a 
   # maximum of 9999 total calls.
   b=$(printf '%04d' $a)
   vgw_session_id="$b-$vgw_session_id_suffix"
   # You can customize it to suit your URL, your MuLaw audio file etc.
   # This will run this voice call simulation application in the background.
   ./c.out $url $audio_file $vgw_session_id 16536 100 &
done
