#--variantList='complete final final_tim final_tim_wrd final_alt final_alt_cfn final_alt_spk'
#--timeout=1200

setCategory 'quick'

TT_mainComposite='WatsonSTT0OutputFunctions'
TT_sabFile="output/WatsonSTT0OutputFunctions.sab"

declare -A description=(
	['complete']='######################## sttResultMode complete; Expect success ###'
	['final']='######################## sttResultMode final; minimal output; Expect success ###'
	['final_tim']='######################## sttResultMode final; plus utterance start and end time ; Expect success ###'
	['final_tim_wrd']='######################## sttResultMode final; plus utterance words ; Expect success ###'
	['final_alt']='######################## sttResultMode final; plus utterance alternatives ; Expect success ###'
	['final_alt_cfn']='######################## sttResultMode final; plus utterance alternatives and confusion network; Expect success ###'
	['final_alt_spk']='######################## sttResultMode final; plus utterance alternatives and speaker labels; Expect success ###'
)

PREPS=(
	'echo "${description[$TTRO_variantCase]}"'
	'copyAndMorphSpl'
	'splCompile --c++std=c++11'
	'TT_traceLevel="debug"'
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
	if [[ $TTRO_variantCase == complete ]]; then
		linewisePatternMatchInterceptAndSuccess "$TTRO_workDirCase/data/Tuples" 'true' \
			'*01-call-center-10sec*utteranceText="hi I am John Smith *sttErrorMessage=""*' \
			'*03-call-center-28sec.wav*utteranceText="my email is change*sttErrorMessage=""*' \
			'*05-gettysburg-address-2min.wav*utteranceText="four score and seven years ago*sttErrorMessage=""*' \
			'*10-invalid-audio.wav*ttErrorMessage="unable to transcode*' \
			'*12-jfk-speech-12sec.wav*utteranceText="and so my fellow Americans*sttErrorMessage=""*'
	elif [[ $TTRO_variantCase == final* ]]; then
		linewisePatternMatchInterceptAndSuccess "$TTRO_workDirCase/data/Tuples" 'true' \
			'*01-call-center-10sec*finalizedUtterance=true*utteranceText="hi I am John Smith *sttErrorMessage=""*' \
			'*01-call-center-10sec*transcriptionCompleted=true*sttErrorMessage=""*' \
			'*03-call-center-28sec.wav*finalizedUtterance=true*utteranceText="my email is change*sttErrorMessage=""*' \
			'*03-call-center-28sec.wav*transcriptionCompleted=true*sttErrorMessage=""*' \
			'*05-gettysburg-address-2min.wav*finalizedUtterance=true*utteranceText="four score and seven years ago*sttErrorMessage=""*' \
			'*05-gettysburg-address-2min.wav*transcriptionCompleted=true*sttErrorMessage=""*' \
			'*10-invalid-audio.wav*ttErrorMessage="unable to transcode*' \
			'*12-jfk-speech-12sec.wav*finalizedUtterance=true*utteranceText="and so my fellow Americans*sttErrorMessage=""*' \
			'*12-jfk-speech-12sec.wav*transcriptionCompleted=true*sttErrorMessage=""*'
	else
		false
	fi
}
