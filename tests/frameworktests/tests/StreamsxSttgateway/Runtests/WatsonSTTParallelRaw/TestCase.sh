#--variantList='fused unFused'
#--timeout=1200
#--exclusive=true

setCategory 'quick'

TT_mainComposite='WatsonSTTParallelRaw'
TT_sabFile="output/WatsonSTTParallelRaw.sab"

declare -A description=(
	[fused]='######################## Parallel execution; Partition colocation; Expect success ###'
	[unFused]='######################## Parallel execution; Partition isolation; Expect success ###'
)

PREPS=(
	'echo "${description[$TTRO_variantCase]}"'
	'copyAndMorphSpl'
	'splCompile --c++std=c++11'
	'TT_traceLevel="info"'
)

STEPS=(
	'submitJob -P "audioDir=$TTPR_SreamsxSttgatewaySamplesPath/audio-files" -P "apiKey=$SPEECH_TO_TEXT_APIKEY" -P "uri=wss://$SPEECH_TO_TEXT_URI/v1/recognize"'
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
'*01-call-center-10sec*finalizedUtterance=true*utteranceText="hi I am John Smith *'
'*01-call-center-10sec*transcriptionCompleted=true*'
'*02-call-center-25sec.wav*finalizedUtterance=true*utteranceText="I went on the*'
'*02-call-center-25sec.wav*transcriptionCompleted=true*'
'*03-call-center-28sec.wav*finalizedUtterance=true*utteranceText="my email is change*'
'*03-call-center-28sec.wav*transcriptionCompleted=true*'
'*04-empty-audio.wav*sttErrorMessage="Stream was 0 bytes but needs*'
'*05-gettysburg-address-2min.wav*finalizedUtterance=true*utteranceText="four score and seven years ago*'
'*05-gettysburg-address-2min.wav*transcriptionCompleted=true*'
'*06-ibm-earnings-1min.wav*finalizedUtterance=true*utteranceText="welcome and thank you for*'
'*06-ibm-earnings-1min.wav*transcriptionCompleted=true*'
'*07-ibm-earnings-2min.wav*utteranceText="also includes certain non gap financial*'
'*07-ibm-earnings-2min.wav*transcriptionCompleted=true*'
'*08-ibm-watson-ai-3min.wav*finalizedUtterance=true*utteranceText="so * we just*'
'*08-ibm-watson-ai-3min.wav*transcriptionCompleted=true*'
'*09-ibm-watson-law-4min.wav*finalizedUtterance=true*utteranceText="you know I*'
'*09-ibm-watson-law-4min.wav*transcriptionCompleted=true*'
'*10-invalid-audio.wav*sttErrorMessage="unable to transcode*'
'*11-ibm-culture-2min.wav*finalizedUtterance=true*utteranceText="we had standing together*'
'*11-ibm-culture-2min.wav*transcriptionCompleted=true*'
'*12-jfk-speech-12sec.wav*finalizedUtterance=true*utteranceText="and so my fellow Americans*'
'*12-jfk-speech-12sec.wav*transcriptionCompleted=true*'
)
