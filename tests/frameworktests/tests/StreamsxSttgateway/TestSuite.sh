# The common test suite for sttgateway toolkit tests
import "$TTRO_scriptDir/streamsutils.sh"

#dependency toolkits
setVar 'TTPR_streamsxJsonToolkit'       "$STREAMS_INSTALL/toolkits/com.ibm.streamsx.json"
setVar 'TTPR_streamsxInetToolkit'       "$STREAMS_INSTALL/toolkits/com.ibm.streamsx.inet"

setVar 'TT_toolkitPath' "${TTPR_streamsxSttgatewayToolkit}:${TTPR_streamsxJsonToolkit}:${TTPR_streamsxInetToolkit}"

getapikey() {
  # the command to generate
  # openssl enc -e -aes-256-cbc -in apikey -out apikey.enc -k tesframeworkpass
  # the file apikey must have the form
  # sttApiKey=<your api key>nl
  local myvar;
  myvar=$(openssl enc -d -aes-256-cbc -in ${TTRO_inputDir}/apikey.enc -k tesframeworkpass)
  eval "$myvar"
}
export -f getapikey
