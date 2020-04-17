#--timeout=1200
#--exclusive=true

# to activate the samples VoiceGatewayToStreamsToWatsonS2T and stt_results_http_receiver
# you may use a separate properties file like :
#setVar 'TTPR_streamsxInetserverToolkit' "$HOME/toolkits/install/4.3.1.0-i20200220/toolkits/com.ibm.streamsx.inetserver"
#setVar 'TTPR_StreamsxNetworkToolkit' "$HOME/toolkits2/streamsx.network-3.4.0/com.ibm.streamsx.network"
#setVar 'TTPR_StreamsxSpeech2TextToolkit' "$HOME/toolkits/install/4.3.1.0-3.6.0-engine4.8.1/toolkits/com.ibm.streams.speech2text"
#setVar 'TTPR_WatsonModelDir' "$HOME/toolkits/install/4.3.1.0-3.6.0-engine4.8.1/model"
#
# an use the coomand line options :
# --properties tests/TestProperties.sh --properties myProperties.sh

if isExistingAndTrue TTPR_StreamsxSpeech2TextToolkit && isExistingAndTrue TTPR_StreamsxNetworkToolkit; then
	:
else
	setSkip "sample requires TTPR_StreamsxSpeech2TextToolkit and TTPR_StreamsxNetworkToolkit"
fi

TT_mainComposite='WatsonS2TParallelFile'
TT_sabFile="output/WatsonS2TParallelFile.sab"

description='######################## Parallel execution; Partition colocation; Expect success ###'

PREPS=(
	'echo "${description}"'
	'copyOnly'
	'setVar TTPR_STTWidth 4'
	'splCompile width=$TTPR_STTWidth'
	'TT_traceLevel="info"'
)

STEPS=(
	'submitJob -P "audioDir=$TTPR_SreamsxSttgatewaySamplesPath/audio-files" -P "watsonModelDir=$TTPR_WatsonModelDir"'
	'checkJobNo'
	'waitForJobHealth'
	'waitForFinAndCheckHealth'
	'cancelJobAndLog'
	'myEvaluate'
)

FINS=(
	'cancelJobAndLog'
)

myEvaluate() {
	if linewisePatternMatchArray "$TTRO_workDirCase/data/Tuples" 'true'; then
		echo "All checks success"
	else
		setFailure "Some results are missing"
	fi
}

TTTT_patternList=(
'*01-call-center-10sec*utteranceText="hi I am John Smith *'
'*01-call-center-10sec*transcriptionCompleted=true*'
'*02-call-center-25sec.wav*utteranceText="I went on the*'
'*02-call-center-25sec.wav*transcriptionCompleted=true*'
'*03-call-center-28sec.wav*utteranceText="my email is change*'
'*03-call-center-28sec.wav*transcriptionCompleted=true*'
'*05-gettysburg-address-2min.wav*utteranceText="four score and seven years ago*'
'*05-gettysburg-address-2min.wav*transcriptionCompleted=true*'
'*06-ibm-earnings-1min.wav*utteranceText="welcome and thank you for*'
'*06-ibm-earnings-1min.wav*transcriptionCompleted=true*'
'*07-ibm-earnings-2min.wav*utteranceText="also includes certain non gap financial*'
'*07-ibm-earnings-2min.wav*transcriptionCompleted=true*'
'*08-ibm-watson-ai-3min.wav*utteranceText="so * we just*'
'*08-ibm-watson-ai-3min.wav*transcriptionCompleted=true*'
'*09-ibm-watson-law-4min.wav*utteranceText="you know I*'
'*09-ibm-watson-law-4min.wav*transcriptionCompleted=true*'
'*11-ibm-culture-2min.wav*utteranceText="we had standing together*'
'*11-ibm-culture-2min.wav*transcriptionCompleted=true*'
'*12-jfk-speech-12sec.wav*utteranceText="and so my fellow Americans*'
'*12-jfk-speech-12sec.wav*transcriptionCompleted=true*'
)
