#--variantList='tokenDelay'
#--timeout=600

setCategory 'quick'

TT_mainComposite='WatsonSTTFileOutFunct'
TT_sabFile="output/WatsonSTTFileOutFunct.sab"

declare -A description=(
	[tokenDelay]='######################## sttResultMode 3. Expect success ###'
)

PREPS=(
	'echo "${description[$TTRO_variantCase]}"'
	'copyAndMorphSpl'
	'splCompile --c++std=c++11'
	'TT_traceLevel="trace"'
	'getapikey'
)

STEPS=(
	'mySubmitJob'
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
	local x
	for ((x=0; x<${#RequiredLines[*]};x++)) do
		linewisePatternMatchInterceptAndSuccess "$TTRO_workDirCase/data/Tuples" 'true' "${RequiredLines[$x]}"
	done
}

RequiredLines=(
'*01-call-center-10sec*fullTranscriptionText="hi I am John Smith *transcriptionCompleted=true,myseq=0*'
'*02-call-center-25sec.wav*ullTranscriptionText="I went on the*transcriptionCompleted=true,myseq=1*'
'*03-call-center-28sec.wav*fullTranscriptionText="my email is change*transcriptionCompleted=true,myseq=2*'
'*04-empty-audio.wav*sttErrorMessage="Stream was 0 bytes but needs*myseq=3*'
'*05-gettysburg-address-2min.wav*ullTranscriptionText="four score and seven years ago*transcriptionCompleted=true,myseq=4*'
'*07-ibm-earnings-2min.wav*fullTranscriptionText="also includes certain*transcriptionCompleted=true,myseq=5*'
'*08-ibm-watson-ai-3min.wav*a brand new integration*transcriptionCompleted=true,myseq=6*'
'*10-invalid-audio.wav*ttErrorMessage="unable to transcode*myseq=7*'
'*12-jfk-speech-12sec.wav*fullTranscriptionText="and so my fellow Americans*transcriptionCompleted=true,myseq=8*'
)

#'*09-ibm-watson-law-4min.wav*fullTranscriptionText="you know*transcriptionCompleted=true,myseq=4*'
#'*11-ibm-culture-2min.wav*fullTranscriptionText="we had standing together*transcriptionCompleted=true,myseq=5*'
#'*06-ibm-earnings-1min.wav*fullTranscriptionText="welcome and thank you*transcriptionCompleted=true,myseq=8'
#'*08-ibm-watson-ai-3min.wav*a brand new integration*transcriptionCompleted=true,myseq=9*'

mySubmitJob() {
	if [[ $TTRO_variantCase == 'tokenDelay' ]]; then
		submitJob -P "audioDir=$TTPR_SreamsxSttgatewaySamplesPath/audio-files" -P 'sttResultMode=3' -P "apiKey=$sttApiKey"
	else
		false
	fi
}
