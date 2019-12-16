# The common test suite for sttgateway toolkit tests
import "$TTRO_scriptDir/streamsutils.sh"

#dependency toolkits
setVar 'TTPR_streamsxJsonToolkit'       "$STREAMS_INSTALL/toolkits/com.ibm.streamsx.json"
setVar 'TTPR_streamsxInetToolkit'       "$STREAMS_INSTALL/toolkits/com.ibm.streamsx.inet"

setVar 'TT_toolkitPath' "${TTPR_streamsxSttgatewayToolkit}:${TTPR_streamsxJsonToolkit}:${TTPR_streamsxInetToolkit}"
