#!/bin/bash
# =========================================================
# First created on: Sep/15/2021
# Last modified on: Sep/20/2021
#
# This is a convenience script that can be used to
# calcel all the speech processor jobs and a single
# VgwDataRouter job. 

# This script takes four arguments as shown below.
# -d tells your Streams domain name.
# -i tells your Streams instance name.
# -b tells the beginning job id that you want to cancel.
# -e tells the ending job id that you want to cancel.
#
# ./stop-stt-mini.sh -d d1 -i i1 -b 45 -e 68
#
# =========================================================
domain_name=""
instance_name=""
beginning_job_id=0
ending_job_id=0

# Number of command line options to shift out
s=0 

while getopts "d:i:b:e:h" options; do
    case $options in
    d) domain_name=$OPTARG
       let s=s+2
       ;;
    i) instance_name=$OPTARG
       let s=s+2
       ;;
    b) beginning_job_id=$OPTARG
       let s=s+2
       ;;
    e) ending_job_id=$OPTARG
       let s=s+2
       ;;
    h | * ) echo "
Command line arguments
  -d STRING      domainName      (d1)
  -i STRING      instanceName    (i1)
  -b INTEGER     beginningJobId  (45)
  -e INTEGER     endingJobId     (68)

  e-g: 
  -d d1 -i i1 -b 45 -e 68
"
        exit 1
        ;;
    esac
done
shift $s

# Validate the Streams domain name as entered by the user.
if [ "$domain_name" == "" ];
then
   echo "Missing or wrong Streams domain name via the -d option."
   echo "Your Streams domain name must be specified."
   echo ""
   echo "Get help using -h option."
   exit 1
fi

# Validate the Streams instance name as entered by the user.
if [ "$instance_name" == "" ];
then
   echo "Missing or wrong Streams instance name via the -i option."
   echo "Your Streams instanxe name must be specified."
   echo ""
   echo "Get help using -h option."
   exit 1
fi

# Validate the beginning job id value as entered by the user.
if [ $beginning_job_id -le 0 ];
then
   echo "Missing or wrong value for the beginning job id via the -b option."
   echo "Please specify a beginning job id greater than 0."
   echo ""
   echo "Get help using -h option."
   exit 1
fi

# Validate the ending job id value as entered by the user.
if [ $ending_job_id -le 0 ];
then
   echo "Missing or wrong value for the ending job id via the -e option."
   echo "Please specify an ending job id greater than 0."
   echo ""
   echo "Get help using -h option."
   exit 1
fi

# ================
# echo $domain_name
# echo $instance_name
# echo $beginning_job_id
# echo $ending_job_id
# exit 1
# ================
current_time=`date`
start_msg="Start time: "
start_msg="$start_msg$current_time"
# ================

# Now stay in a loop and cancel the jobs starting from 
# beginning id to ending id as provided by the user.
for (( idx=$beginning_job_id; idx<=$ending_job_id; idx++ ))
do
   streamtool canceljob -d $domain_name -i $instance_name -j $idx
done

# ================
current_time=`date`
end_msg="End time: "
end_msg="$end_msg$current_time"
echo "*******************"
echo $start_msg
echo $end_msg
echo "*******************"
# ================

# Alternatively, you can do it in a single line from the Linux command line.
# for i in {48..73}; do streamtool canceljob -d d1 -i i1 -j $i; done
