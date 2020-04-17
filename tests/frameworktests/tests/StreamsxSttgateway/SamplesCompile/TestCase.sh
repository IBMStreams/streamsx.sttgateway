# The case to compile all available samples

#--variantList=$(\
#--for x in $TTPR_SreamsxSttgatewaySamplesPath/*; do \
#--	if [[ -f $x/Makefile ]]; then \
#--		echo -n "${x#$TTPR_SreamsxSttgatewaySamplesPath/} "; \
#--	fi; \
#--done\
#--)

# to activate the samples VoiceGatewayToStreamsToWatsonS2T and stt_results_http_receiver
# you may use a separate properties file like :
#setVar 'TTPR_streamsxInetserverToolkit' "$HOME/toolkits/install/4.3.1.0-i20200220/toolkits/com.ibm.streamsx.inetserver"
#setVar 'TTPR_StreamsxNetworkToolkit' "$HOME/toolkits2/streamsx.network-3.4.0/com.ibm.streamsx.network"
#setVar 'TTPR_StreamsxSpeech2TextToolkit' "$HOME/toolkits/install/4.3.1.0-3.6.0-engine4.8.1/toolkits/com.ibm.streams.speech2text"
#setVar 'TTPR_WatsonModelDir' "$HOME/toolkits/install/4.3.1.0-3.6.0-engine4.8.1/model"
#
# an use the coomand line options :
# --properties tests/TestProperties.sh --properties myProperties.sh

setCategory 'quick'

#skip the special samples unless some environments are set
if [[ $TTRO_variantCase == 'VoiceGatewayToStreamsToWatsonS2T' ]]; then
	if isExistingAndTrue TTPR_StreamsxSpeech2TextToolkit && isExistingAndTrue TTPR_StreamsxNetworkToolkit; then
		export STREAMS_S2T_TOOLKIT="$TTPR_StreamsxSpeech2TextToolkit"
		export STREAMS_NETWORK_TOOLKIT="$TTPR_StreamsxNetworkToolkit"
	else
		setSkip "sample requires TTPR_StreamsxSpeech2TextToolkit and TTPR_StreamsxNetworkToolkit"
	fi
fi
if [[ $TTRO_variantCase == 'stt_results_http_receiver' ]]; then
	if ! isExistingAndTrue TTPR_streamsxInetserverToolkit; then
		setSkip "sample requires TTPR_streamsxInetserverToolkit"
	else
		export STREAMS_INET_SERVER_TOOLKIT="$TTPR_streamsxInetserverToolkit"
	fi
fi

if [[ $TTRO_variantCase == 'VoiceDataSimulator' ]]; then
	setSkip "sample is not an spl sample"
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