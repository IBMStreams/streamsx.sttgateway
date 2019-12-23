#--timeout=600

setCategory 'quick'

TT_mainComposite=AccessTokenGenerator
TT_sabFile="output/AccessTokenGenerator.sab"

PREPS=(
	'copyOnly'
	'splCompile'
	'TT_traceLevel="warn"'
)

# use TTRO_inputDirCase as model dir has hidden files and hidden files are not copied from copyOnly
STEPS=(
	'submitJob'
	'checkJobNo'
	'waitForJobHealth'
	'TT_waitForFileName="$TT_dataDir/WindowMarker"'
	'waitForFinAndCheckHealth'
	'sleep 10'
	'cancelJobAndLog'
	'myEvaluate'
)

FINS='cancelJobAndLog'

myEvaluate() {
	local linecount=$(grep "access_token=\"2YotnFZFEjr1zCsicMWpAA\"" data/Tuples | wc -l)
	if [[ $linecount -lt 4 ]]; then
		setFailure "to few access token received: linecount is $linecount"
	fi
}