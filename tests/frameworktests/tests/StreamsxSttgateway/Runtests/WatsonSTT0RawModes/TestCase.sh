#--variantList='fusedMode1 fusedMode2 fusedMode3 fusedTokenDelay \
#--             fusedQueueMode1 fusedQueueMode2 fusedQueueMode3 fusedQueueTokenDelay \
#--             unFusedMode1 unFusedMode2 unFusedMode3 unFusedTokenDelay'
##--variantList='fusedMode1'
#--timeout=1200

TT_mainComposite='WatsonSTT0RawModes'
TT_sabFile="output/WatsonSTT0RawModes.sab"

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
	'submitJob -P "audioDir=$TTPR_SreamsxSttgatewaySamplesPath/audio-files" -P "apiKey=$TTPR_SpeechToTextApikey" -P "uri=$TTPR_SpeechToTextUrl/v1/recognize"'
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
		TTTT_patternList=(
			'*01-call-center-10sec*utteranceText="hi I am John Smith *'
			'*02-call-center-25sec*utteranceText="I went on the*'
			'*03-call-center-28sec*utteranceText="my email is change*'
			'*04-empty-audio.wav*sttErrorMessage="Stream was 0 bytes but needs*'
			'*05-gettysburg-address-2min.wav*utteranceText="four score and seven years ago*'
			'*07-ibm-earnings-2min.wav*utteranceText="also includes certain*'
			'*08-ibm-watson-ai-3min.wav*a brand new integration*'
			'*10-invalid-audio.wav*ttErrorMessage="unable to transcode*'
			'*12-jfk-speech-12sec.wav*utteranceText="and so my fellow Americans*'
		)
	else
		TTTT_patternList=(
			'*01-call-center-10sec*="hi I am John Smith *'
			'*01-call-center-10sec*transcriptionCompleted=true*'
			'*02-call-center-25sec.wav*="I went on the*'
			'*02-call-center-25sec*transcriptionCompleted=true*'
			'*03-call-center-28sec.wav*="my email is change*'
			'*03-call-center-28sec*transcriptionCompleted=true*'
			'*04-empty-audio.wav*sttErrorMessage="Stream was 0 bytes but needs*'
			'*04-empty-audio.wav*transcriptionCompleted=true*'
			'*05-gettysburg-address-2min.wav*="four score and seven years ago*'
			'*05-gettysburg-address-2min.wav*transcriptionCompleted=true*'
			'*07-ibm-earnings-2min.wav*="also includes certain*'
			'*07-ibm-earnings-2min.wav*transcriptionCompleted=true*'
			'*08-ibm-watson-ai-3min.wav*a brand new integration*'
			'*08-ibm-watson-ai-3min.wav*transcriptionCompleted=true*'
			'*10-invalid-audio.wav*ttErrorMessage="unable to transcode*'
			'*10-invalid-audio.wav*transcriptionCompleted=true*'
			'*12-jfk-speech-12sec.wav*="and so my fellow Americans*'
			'*12-jfk-speech-12sec.wav*transcriptionCompleted=true*'
		)
	fi
	if ! linewisePatternMatchArray "$TTRO_workDirCase/data/Tuples" 'true'; then
		setError "Not enough pattern matches found"
	fi
}
