# The case to compile all available samples

#--variantList=$(\
#--for x in $TTPR_SreamsxSttgatewaySamplesPath/*; do \
#--	if [[ -f $x/Makefile ]]; then \
#--		echo -n "${x#$TTPR_SreamsxSttgatewaySamplesPath/} "; \
#--	fi; \
#--done\
#--)

setCategory 'quick'

#skip the special samples unless some environments are set
if [[ $TTRO_variantCase == 'VoiceGatewayToStreamsToWatsonS2T' ]]; then
#	if ! isExistingAndTrue TTPR_StreamsxSpeech2TextToolkit; then
		setSkip "sample requires TTPR_StreamsxSpeech2TextToolkit"
#	else
#		export STREAMS_S2T_TOOLKIT="$TTPR_StreamsxSpeech2TextToolkit"
#	fi
fi
if [[ $TTRO_variantCase == 'stt_results_http_receiver' ]]; then
	if ! isExistingAndTrue TTPR_streamsxInetserverToolkit; then
		setSkip "sample requires TTPR_streamsxInetserverToolkit"
	else
		export STREAMS_INET_SERVER_TOOLKIT="$TTPR_streamsxInetserverToolkit"
	fi
fi

function testStep {
	local save="$PWD"
	cd "$TTPR_SreamsxSttgatewaySamplesPath/$TTRO_variantCase"
	pwd
	export SPL_CMD_ARGS="-j $TTRO_treads"
	export STREAMS_STTGATEWAY_TOOLKIT="$TTPR_streamsxSttgatewayToolkit"
	export STREAMS_JSON_TOOLKIT="$TTPR_streamsxJsonToolkit"
	export STREAMS_INET_TOOLKIT="$TTPR_streamsxInetToolkit"
	echoExecuteInterceptAndSuccess 'make' 'all'
	cd "$save"
	return 0
}