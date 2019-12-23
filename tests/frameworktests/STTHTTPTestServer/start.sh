#!/bin/bash

IFS=$' \t\n'
set -o posix;
set -o errexit; set -o errtrace; set -o pipefail

declare -r command="${0##*/}"
declare -r commandPath="${0%/*}"

usage() {
	cat <<-EOF

	usage: ${command} [option]

	Start or stop the inet toolkit http test server.

	OPTIONS:
	-h|--help                : display this help
	--noprompt               : No interactive user interaction
	-f                       : start one instance in foreground - ports: 8097 1443
	EOF
}

userPrompt() {
	local pr="Continue or not? y/n "
	local inputWasY=''
	while read -p "$pr"; do
		if [[ $REPLY == y* || $REPLY == Y* || $REPLY == c* || $REPLY == C* ]]; then
			inputWasY='true'
			break

		elif [[ $REPLY == e* || $REPLY == E* || $REPLY == n* || $REPLY == N* ]]; then
			inputWasY=''
			break
		fi
	done
	if [[ -n $inputWasY ]]; then
		return 0
	else
		return 1
	fi
}

declare noprompt=''
declare foreground=''

if [[ $# -gt 0 ]]; then
	case "$1" in
	-h|--help)
		usage
		exit 0;;
	--noprompt)
		noprompt='true';;
	-f)
		foreground='true';;
	*)
		echo "Wrong command line argument $1 exit" >&2
		exit 1;;
	esac
fi

if [[ -e $commandPath/.pid ]]; then
	"$commandPath/stop.sh"
fi

declare javacmd='java'

cd "$commandPath"
ant
rm -f nohup.out
if [[ -z $foreground ]]; then
	echo "Starting  http test server"
	nohup "$javacmd" "-cp" "bin:opt/jetty-all-9.4.12.v20180830-uber.jar" "com.ibm.streamsx.sttgateway.test.httptestserver.STTHTTPTestServer" 8097 1443 &> nohup1.out &
	echo -n "$!" > .pid1
else
	echo "Starting  http test server 8097 1443"
	eval "$javacmd" "-cp" "bin:opt/jetty-all-9.4.12.v20180830-uber.jar" "com.ibm.streamsx.sttgateway.test.httptestserver.STTHTTPTestServer" 8097 1443
fi
exit 0;
