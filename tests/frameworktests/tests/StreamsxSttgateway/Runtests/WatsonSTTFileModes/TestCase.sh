#--variantList='fusedMode1 fusedMode2 fusedMode3 fusedTokenDelay \
#--             fusedQueueMode1 fusedQueueMode2 fusedQueueMode3 fusedQueueTokenDelay \
#--             unFusedMode1 unFusedMode2 unFusedMode3 unFusedTokenDelay'
#--timeout=1200

setCategory 'quick'

TT_mainComposite='WatsonSTTFileModes'
TT_sabFile="output/WatsonSTTFileModes.sab"

declare -A description=(
	[fusedMode1]='######################## sttResultMode 1 Partition colocation; Expect success ###'
	[fusedMode2]='######################## sttResultMode 2 Partition colocation; Expect success ###'
	[fusedMode3]='######################## sttResultMode 3 Partition colocation; Expect success ###'
	[fusedTokenDelay]='######################## sttResultMode 3 Partition colocation; Threaded Port; Delayed access; token Expect success ###'
	[fusedQueueMode1]='######################## sttResultMode 1 Partition colocation; Threaded Port; Expect success ###'
	[fusedQueueMode2]='######################## sttResultMode 2 Partition colocation; Threaded Port; Expect success ###'
	[fusedQueueMode3]='######################## sttResultMode 3 Partition colocation; Threaded Port; Expect success ###'
	[fusedQueueTokenDelay]='######################## sttResultMode 3 Partition colocation; Delayed access; token Expect success ###'
	[unFusedMode1]='######################## sttResultMode 1 Partition isolation; Expect success ###'
	[unFusedMode2]='######################## sttResultMode 2 Partition isolation; Expect success ###'
	[unFusedMode3]='######################## sttResultMode 3 Partition isolation; Expect success ###'
	[unFusedTokenDelay]='######################## sttResultMode 3 Partition isolation; Delayed access; token Expect success ###'
)

PREPS=(
	'echo "${description[$TTRO_variantCase]}"'
	'copyAndMorphSpl'
	'splCompile --c++std=c++11'
	'TT_traceLevel="trace"'
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
	if [[ ( $TTRO_variantCase == *Mode3 ) || ( $TTRO_variantCase == *TokenDelay ) ]]; then
		local x
		for ((x=0; x<${#RequiredLinesFull[*]};x++)) do
			linewisePatternMatchInterceptAndSuccess "$TTRO_workDirCase/data/Tuples" 'true' "${RequiredLinesFull[$x]}"
		done
	else
		local x
		for ((x=0; x<${#RequiredLines[*]};x++)) do
			linewisePatternMatchInterceptAndSuccess "$TTRO_workDirCase/data/Tuples" 'true' "${RequiredLines[$x]}"
		done
	fi
}

RequiredLinesFull=(
'*01-call-center-10sec*utteranceText="hi I am John Smith *myseq=0*'
'*02-call-center-25sec.wav*utteranceText="I went on the*myseq=1*'
'*03-call-center-28sec.wav*utteranceText="my email is change*myseq=2*'
'*04-empty-audio.wav*sttErrorMessage="Stream was 0 bytes but needs*myseq=3*'
'*05-gettysburg-address-2min.wav*utteranceText="four score and seven years ago*myseq=4*'
'*07-ibm-earnings-2min.wav*utteranceText="also includes certain*myseq=5*'
'*08-ibm-watson-ai-3min.wav*a brand new integration*myseq=6*'
'*10-invalid-audio.wav*ttErrorMessage="unable to transcode*myseq=7*'
'*12-jfk-speech-12sec.wav*utteranceText="and so my fellow Americans*myseq=8*'
)
RequiredLines=(
'*01-call-center-10sec*="hi I am John Smith *myseq=0*'
'*02-call-center-25sec.wav*="I went on the*myseq=1*'
'*03-call-center-28sec.wav*="my email is change*myseq=2*'
'*04-empty-audio.wav*sttErrorMessage="Stream was 0 bytes but needs*myseq=3*'
'*05-gettysburg-address-2min.wav*="four score and seven years ago*myseq=4*'
'*07-ibm-earnings-2min.wav*="also includes certain*myseq=5*'
'*08-ibm-watson-ai-3min.wav*a brand new integration*myseq=6*'
'*10-invalid-audio.wav*ttErrorMessage="unable to transcode*myseq=7*'
'*12-jfk-speech-12sec.wav*="and so my fellow Americans*myseq=8*'
)

#'*09-ibm-watson-law-4min.wav*fullTranscriptionText="you know*transcriptionCompleted=true,myseq=4*'
#'*11-ibm-culture-2min.wav*fullTranscriptionText="we had standing together*transcriptionCompleted=true,myseq=5*'
#'*06-ibm-earnings-1min.wav*fullTranscriptionText="welcome and thank you*transcriptionCompleted=true,myseq=8'
#'*08-ibm-watson-ai-3min.wav*a brand new integration*transcriptionCompleted=true,myseq=9*'
