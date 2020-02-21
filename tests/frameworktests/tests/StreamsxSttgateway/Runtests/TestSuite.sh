# http server install location
setVar 'TTPR_httpServerDir' "$TTRO_inputDir/../STTHTTPTestServer"

# expected http server definitions
# TTPR_httpServerHost
# if TTPR_httpServerHost is not set, the http server is started from http server install location TTPR_httpServerDir
# the following props depend usually from host
# TTPR_httpServerAddr  "${TTPR_httpServerHost}:8097"
# TTPR_httpsServerAddr "${TTPR_httpServerHost}:1443"

#Make sure instance and domain is running, ftp server running
PREPS='getstturi getapikey cleanUpInstAndDomainAtStart mkDomain startDomain mkInst startInst startHttpServer'
FINS='cleanUpInstAndDomainAtStop stopHttpServer'

# STT uri is required in TTPR_SpeechToTextUrl
# but is may be used from SPEECH_TO_TEXT_URL
getstturi() {
	if ! isExisting 'TTPR_SpeechToTextUrl'; then
		if isExisting 'SPEECH_TO_TEXT_URL'; then
			setVar 'TTPR_SpeechToTextUrl' "$SPEECH_TO_TEXT_URL"
		fi
	fi
	if ! isExistingAndTrue 'TTPR_SpeechToTextUrl'; then
		printError "Variable TTPR_SpeechToTextUrl is required with non empty value. See README.md"
		return 1
	fi
}

# STT api key is required in TTPR_SpeechToTextApikey
# but is may be used from SPEECH_TO_TEXT_APIKEY
# or it may be provided in encrypted form in file with the name provided in 
# TTPR_SpeechToTextApikeyFile
# If the api key is provided in encrypted foe the package openssl is required
# the command to generate the file is
# openssl enc -e -aes-256-cbc -in apikey -out apikey.enc -k tesframeworkpass
# the file apikey must store the apikey without any further characters
getapikey() {
	if ! isExisting 'TTPR_SpeechToTextApikey'; then
		if isExisting 'SPEECH_TO_TEXT_APIKEY'; then
			setVar 'TTPR_SpeechToTextApikey' "$SPEECH_TO_TEXT_APIKEY"
		else
			# api key is not provided try to decrypt from file
			if ! openssl version; then
				printError "Tests require package openssl"
				return 1
			fi
			local myvar;
			openssl enc -d -aes-256-cbc -in "${TTPR_SpeechToTextApikeyFile}" -out .sttenv -k tesframeworkpass
			myvar=$(<.sttenv)
			setVar 'TTPR_SpeechToTextApikey' "$myvar"
			rm .sttenv
		fi
	fi
}

startHttpServer() {
	if isNotExisting 'TTPR_httpServerHost'; then
		"$TTPR_httpServerDir/start.sh"
		setVar 'TTPR_httpServerHost' "$HOSTNAME"
		setVar 'TTRO_httpServerLocal' 'true'
	else
		printInfo "Property TTPR_httpServerHost exists -> no start of local http server"
	fi
	# http server definitions
	setVar 'TTPR_httpServerAddr'  "${TTPR_httpServerHost}:8097"
	setVar 'TTPR_httpsServerAddr' "${TTPR_httpServerHost}:1443"
}

stopHttpServer() {
	if isExistingAndTrue 'TTRO_httpServerLocal'; then
		"$TTPR_httpServerDir/stop.sh"
	else
		printInfo "no start of local http server -> no stop of local http server"
	fi
}
