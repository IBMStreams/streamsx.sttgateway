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
if [[ $TTRO_variantCase == 'STTGatewayUtils' ]]; then
	setSkip 'Dependency toolkit is compiled in TestSuite.sh since (since v.2.2.5)'
fi

if [[ $TTRO_variantCase == 'VoiceGatewayToStreamsToWatsonS2T' ]]; then
	if isExistingAndTrue TTPR_StreamsxSpeech2TextToolkit && isExistingAndTrue TTPR_StreamsxNetworkToolkit; then
		export STREAMS_S2T_TOOLKIT="$TTPR_StreamsxSpeech2TextToolkit"
		export STREAMS_NETWORK_TOOLKIT="$TTPR_StreamsxNetworkToolkit"
	else
		setSkip "sample requires TTPR_StreamsxSpeech2TextToolkit and TTPR_StreamsxNetworkToolkit"
	fi
fi
if [[ $TTRO_variantCase == 'VoiceGatewayToStreamsToWatsonSTT' || $TTRO_variantCase == 'stt_results_http_receiver' || $TTRO_variantCase == 'VoiceGatewayToStreamsToWatsonS2T' || $TTRO_variantCase == 'VgwDataRouter' || $TTRO_variantCase == 'VgwDataRouterToWatsonSTT' ]]; then
	if isExistingAndTrue TTPR_StreamsxWebsocketToolkit; then
		export STREAMS_WEBSOCKET_TOOLKIT="$TTPR_StreamsxWebsocketToolkit"
	else
		setSkip "sample requires TTPR_StreamsxWebsocketToolkit"
	fi
fi

if [[ $TTRO_variantCase == 'VgwDataRouterToWatsonS2T' ]]; then
	if isExistingAndTrue TTPR_StreamsxSpeech2TextToolkit && isExistingAndTrue TTPR_StreamsxNetworkToolkit && isExistingAndTrue TTPR_StreamsxWebsocketToolkit; then
		export STREAMS_S2T_TOOLKIT="$TTPR_StreamsxSpeech2TextToolkit"
		export STREAMS_NETWORK_TOOLKIT="$TTPR_StreamsxNetworkToolkit"
		export STREAMS_WEBSOCKET_TOOLKIT="$TTPR_StreamsxWebsocketToolkit"
	else
		setSkip "to many dependencies required"
	fi
fi

if [[ $TTRO_variantCase == 'VoiceDataSimulator' ]]; then
	setSkip "sample is not an spl sample"
fi

testStep() {
	export SPL_CMD_ARGS="-j $TTRO_treads"
	export STREAMS_STTGATEWAY_TOOLKIT="$TTPR_streamsxSttgatewayToolkit"
	export STREAMS_JSON_TOOLKIT="$TTPR_streamsxJsonToolkit"
	export STREAMS_INET_TOOLKIT="$TTPR_streamsxInetToolkit"
	local save="$PWD"
	cd "$TTPR_SreamsxSttgatewaySamplesPath/$TTRO_variantCase"
	pwd
	echoExecuteInterceptAndSuccess 'make' 'all'
	cd "$save"
	return 0
}

