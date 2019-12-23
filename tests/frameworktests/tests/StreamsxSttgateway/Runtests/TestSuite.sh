# http server install location
setVar 'TTPR_httpServerDir' "$TTRO_inputDir/../STTHTTPTestServer"

# expected http server definitions
# TTPR_httpServerHost
# if TTPR_httpServerHost is not set, the http server is started from http server install location TTPR_httpServerDir
# the following props depend usually from host
# TTPR_httpServerAddr  "${TTPR_httpServerHost}:8097"
# TTPR_httpsServerAddr "${TTPR_httpServerHost}:1443"

#Make sure instance and domain is running, ftp server running
PREPS='cleanUpInstAndDomainAtStart mkDomain startDomain mkInst startInst startHttpServer'
FINS='cleanUpInstAndDomainAtStop stopHttpServer'

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
