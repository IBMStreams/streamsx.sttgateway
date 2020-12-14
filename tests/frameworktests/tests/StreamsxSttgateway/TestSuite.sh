# The common test suite for sttgateway toolkit tests
import "$TTRO_scriptDir/streamsutils.sh"

#dependency toolkits
setVar 'TTPR_streamsxJsonToolkit'       "$STREAMS_INSTALL/toolkits/com.ibm.streamsx.json"
setVar 'TTPR_streamsxInetToolkit'       "$STREAMS_INSTALL/toolkits/com.ibm.streamsx.inet"

TTTT_myToolkitPath="${TTPR_streamsxSttgatewayToolkit}:${TTPR_streamsxJsonToolkit}:${TTPR_streamsxInetToolkit}"

if isExistingAndTrue TTPR_StreamsxSpeech2TextToolkit; then
	TTTT_myToolkitPath="$TTTT_myToolkitPath:$TTPR_StreamsxSpeech2TextToolkit"
fi

if isExistingAndTrue TTPR_StreamsxNetworkToolkit; then
	TTTT_myToolkitPath="$TTTT_myToolkitPath:$TTPR_StreamsxNetworkToolkit"
fi

setVar 'TT_toolkitPath' "${TTTT_myToolkitPath}"

testPreparation() {
	export SPL_CMD_ARGS="-j $TTRO_treads"
	export STREAMS_STTGATEWAY_TOOLKIT="$TTPR_streamsxSttgatewayToolkit"
	export STREAMS_JSON_TOOLKIT="$TTPR_streamsxJsonToolkit"
	export STREAMS_INET_TOOLKIT="$TTPR_streamsxInetToolkit"
	echo "Compile dependency toolkit"
	local save="$PWD"
	cd "$TTPR_SreamsxSttgatewaySamplesPath/STTGatewayUtils"
	pwd
	echoAndExecute 'make' 'all'
	cd "$save"
}

