#enhance timeout due to log running compile jobs for samples
setVar TTPR_timeout 600

#samples path
setVar 'TTPR_SreamsxSttgatewaySamplesPath' "$TTRO_inputDir/../../../samples"

#toolkit path
setVar 'TTPR_streamsxSttgatewayToolkit' "$TTRO_inputDir/../../../com.ibm.streamsx.sttgateway"

#do not set TT_toolkitPath here because it is assembled in global TestSuite.sh

# stt url without trailing v1/recognize may be provided with TTPR_SpeechToTextUrl
setVar 'TTPR_SpeechToTextUrl' "wss://api.us-south.speech-to-text.watson.cloud.ibm.com/instances/4833f0f6-d2b3-4ee0-bc35-ecaf21e76313"
# or remove the previous line and provide the environment SPEECH_TO_TEXT_URL
# or you provide the command line parameter -D TTPR_SpeechToTextUrl=<url>

# The stt api key may be provided in encrypted form in file apikey.enc in input directory
setVar 'TTPR_SpeechToTextApikeyFile' "${TTRO_inputDir}/apikey.enc"
# or you provide the environment SPEECH_TO_TEXT_APIKEY
# or you provide the command line parameter -D TTPR_SpeechToTextApikey=<apikey>