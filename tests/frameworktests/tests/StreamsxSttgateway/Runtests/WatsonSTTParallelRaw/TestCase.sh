#--variantList='fused unFused'
#--timeout=1200
#--exclusive=true

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
'*myseq=0*01-call-center-10sec*finalizedUtterance=true*utteranceText="hi I am John Smith *'
'*myseq=0*01-call-center-10sec*transcriptionCompleted=true*'
'*myseq=1*02-call-center-25sec.wav*finalizedUtterance=true*utteranceText="I went on the*'
'*myseq=1*02-call-center-25sec.wav*transcriptionCompleted=true*'
'*myseq=2*03-call-center-28sec.wav*finalizedUtterance=true*utteranceText="my email is change*'
'*myseq=2*03-call-center-28sec.wav*transcriptionCompleted=true*'
'*myseq=3*04-empty-audio.wav*sttErrorMessage="Stream was 0 bytes but needs*'
'*myseq=4*05-gettysburg-address-2min.wav*finalizedUtterance=true*utteranceText="four score and seven years ago*'
'*myseq=4*05-gettysburg-address-2min.wav*transcriptionCompleted=true*'
'*myseq=5*06-ibm-earnings-1min.wav*finalizedUtterance=true*utteranceText="welcome and thank you for*'
'*myseq=5*06-ibm-earnings-1min.wav*transcriptionCompleted=true*'
'*myseq=6*07-ibm-earnings-2min.wav*utteranceText="also includes certain non gap financial*'
'*myseq=6*07-ibm-earnings-2min.wav*transcriptionCompleted=true*'
'*myseq=7*08-ibm-watson-ai-3min.wav*finalizedUtterance=true*utteranceText="so * we just*'
'*myseq=7*08-ibm-watson-ai-3min.wav*transcriptionCompleted=true*'
'*myseq=8*09-ibm-watson-law-4min.wav*finalizedUtterance=true*utteranceText="you know I*'
'*myseq=8*09-ibm-watson-law-4min.wav*transcriptionCompleted=true*'
'*myseq=9*10-invalid-audio.wav*sttErrorMessage="unable to transcode*'
'*myseq=10*11-ibm-culture-2min.wav*finalizedUtterance=true*utteranceText="we had standing together*'
'*myseq=10*11-ibm-culture-2min.wav*transcriptionCompleted=true*'
'*myseq=11*12-jfk-speech-12sec.wav*finalizedUtterance=true*utteranceText="and so my fellow Americans*'
'*myseq=11*12-jfk-speech-12sec.wav*transcriptionCompleted=true*'
)
