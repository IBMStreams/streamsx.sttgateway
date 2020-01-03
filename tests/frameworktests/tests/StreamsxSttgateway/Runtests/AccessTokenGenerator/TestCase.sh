#--variantList='params appConfDefault appConfSpecial paramsSecure paramsInvalid'
#--timeout=600
#--exclusive=true

setCategory 'quick'

TT_mainComposite=AccessTokenGenerator
TT_sabFile="output/AccessTokenGenerator.sab"

declare -A description=(
	[params]='######################## Get the connection information from operator parameters. Expect success ###'
	[appConfDefault]='######################## Get the connection information from default application configutation. Expect success ###'
	[appConfSpecial]='######################## Get the connection information from special compiled in application configutation. Expect success ###'
	[paramsSecure]='######################## Get the connection information from operator parameters. Secure connection. Expect success ###'
	[paramsInvalid]='######################## Get the connection information from operator parameters. Expect no output tuples ###'
)

PREPS=(
	'echo "${description[$TTRO_variantCase]}"'
	'copyAndMorphSpl'
	'myCompile'
	'cleanAppConf'
	'setAppConf'
	'TT_traceLevel="warn"'
)

STEPS=(
	'mySubmitJob'
	'checkJobNo'
	'waitForJobHealth'
	'TT_waitForFileName="$TT_dataDir/WindowMarker"'
	'myWait'
	'cancelJobAndLog'
	'myEvaluate'
	'if [[ $TTRO_variantCase != 'paramsInvalid' ]]; then checkLogsNoError; fi'
)

FINS=(
	'cancelJobAndLog'
	'cleanAppConf'
)

myEvaluate() {
	if [[ $TTRO_variantCase == 'paramsInvalid' ]]; then
		if [[ -e data/Tuples ]]; then
			setFailure "File data/Tuples exists but is not expected"
		fi
	else
		local i
		for i in 0 1 2 3; do
			local linecount=$(grep "access_token=\"2YotnFZFEjr1zCsicMWpAA_access_$i\"" data/Tuples | wc -l)
			if [[ $linecount -lt 1 ]]; then
				setFailure "to few access token received: linecount is $linecount i is $i"
			fi
		done
	fi
}

myCompile() {
	if [[ $TTRO_variantCase == 'appConfSpecial' ]]; then
		splCompile 'applicationConfiguration=specialSttConnection'
	else
		splCompile
	fi
}

mySubmitJob() {
	if [[ $TTRO_variantCase == 'paramsSecure' ]]; then
		submitJob -P 'iamTokenURL=https://localhost:1443/access'
	elif [[ $TTRO_variantCase == 'paramsInvalid' ]]; then
		submitJob -P 'apiKey=invalid'
	else
		submitJob
	fi	
}

myWait() {
	if [[ $TTRO_variantCase == 'paramsInvalid' ]]; then
		sleep 60
		if ! jobHealthy; then
			setFailure "Job is not healty"
		fi
	else
		'waitForFinAndCheckHealth'
	fi
}

setAppConf() {
	if [[ $TTRO_variantCase == appConf* ]]; then
		local appconfname='sttConnection'
		if [[ $TTRO_variantCase == 'appConfSpecial' ]]; then
			appconfname='specialSttConnection'
		fi
		streamtool mkappconfig --description 'A test propperty' \
			--property "apiKey=valid" \
			--property "iamTokenURL=http://localhost:8097/access" \
			--property "failureRetryDelay=10.0" \
			--property "guardTime=5" \
			"$appconfname"
	fi
}

cleanAppConf() {
	# clean all app confs and ignore errors
	streamtool rmappconfig sttConnection --noprompt || :
	streamtool rmappconfig specialSttConnection --noprompt || :
}
