#enhance timeout due to log running compile jobs for samples
setVar TTPR_timeout 300

#samples path
setVar 'TTPR_SreamsxSttgatewaySamplesPath' "$TTRO_inputDir/../../../samples"

#toolkit path
setVar 'TTPR_streamsxSttgatewayToolkit' "$TTRO_inputDir/../../../com.ibm.streamsx.sttgateway"

#do not set TT_toolkitPath here because it is assembled in global TestSuite.sh
