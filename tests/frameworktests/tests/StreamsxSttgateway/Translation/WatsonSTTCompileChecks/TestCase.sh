#--variantCount=3

setCategory quick

TT_mainComposite='WatsonSTTCompileChecks'
TT_sabFile="output/WatsonSTTCompileChecks.sab"

declare -a description=(
	'#### variant 0 - good case ####################################'
	'#### variant 1 - missing input attribute ####################################'
	'#### variant 2 - input attribute wrong type ####################################'
)

PREPS=(
	'echo "${description[$TTRO_variantCase]}"'
	'copyAndMorphSpl'
)

STEPS=(
	'myCompile'
	'myEval'
)

myCompile() {
	if [[ $TTRO_variantCase -eq 0 ]]; then
		splCompileInterceptAndSuccess '--c++std=c++11'
	else
		splCompileInterceptAndError	'--c++std=c++11'
	fi
}

myEval() {
	if [[ $TTRO_variantCase -eq 0 ]]; then
		return 0
	fi
	local variantCase=$((TTRO_variantCase - 1 ))
	case "$TTRO_variantSuite" in
	de_DE)
		linewisePatternMatchInterceptAndSuccess "$TT_evaluationFile" "" "${errorCodes_de_DE[$variantCase]}";;
	fr_FR)
		linewisePatternMatchInterceptAndSuccess "$TT_evaluationFile" "" "${errorCodes_fr_FR[$variantCase]}";;
	it_IT)
		linewisePatternMatchInterceptAndSuccess "$TT_evaluationFile" "" "${errorCodes_it_IT[$variantCase]}";;
	es_ES)
		linewisePatternMatchInterceptAndSuccess "$TT_evaluationFile" "" "${errorCodes_es_ES[$variantCase]}";;
	pt_BR)
		linewisePatternMatchInterceptAndSuccess "$TT_evaluationFile" "" "${errorCodes_pt_BR[$variantCase]}";;
	ja_JP)
		linewisePatternMatchInterceptAndSuccess "$TT_evaluationFile" "" "${errorCodes_ja_JP[$variantCase]}";;
	zh_CN)
		linewisePatternMatchInterceptAndSuccess "$TT_evaluationFile" "" "${errorCodes_zh_CN[$variantCase]}";;
	zh_TW)
		linewisePatternMatchInterceptAndSuccess "$TT_evaluationFile" "" "${errorCodes_zh_TW[$variantCase]}";;
	en_US)
		linewisePatternMatchInterceptAndSuccess "$TT_evaluationFile" "" "${errorCodes[$variantCase]}";;
	esac;
}

errorCodes=(
	"*CDIST3801E: Operator WatsonSTT: The required input tuple attribute 'speech' is missing in*"
	"*CDIST3802E: Operator WatsonSTT: The required input tuple attribute 'speech' is not of type*"
)

errorCodes_de_DE=(
	"*CDIST3801E: Operator WatsonSTT: Das erforderliche Eingabetupelattribut 'speech' fehlt im ersten*"
	"*CDIST3802E: Operator WatsonSTT: Das erforderliche Eingabetupelattribut 'speech' hat nicht den Typ*"
)

errorCodes_fr_FR=(
	"*CDISP9164E ERREUR: CDIST3801E: Opérateur WatsonSTT : l'attribut de bloc de données d'entrée requis*"
	"*CDIST3802E: Opérateur WatsonSTT : l'attribut de bloc de données d'entrée requis 'speech' n'est pas de type*"
)

errorCodes_it_IT=(
	"*CDIST3801E: Operatore WatsonSTT: l'attributo di tupla di input richiesto 'speech' nella prima porta*"
	"*CDIST3802E: Operatore WatsonSTT: l'attributo di tupla di input richiesto 'speech' non è di tipo*"
)

errorCodes_es_ES=(
	"*CDIST3801E: Operador WatsonSTT: Falta el atributo de tupla de entrada necesario 'speech' en el primer*"
	"*CDIST3802E: Operador WatsonSTT: El atributo de tupla de entrada necesario 'speech' no es del tipo*"
)

errorCodes_pt_BR=(
	"*CDIST3801E: Operador WatsonSTT: o atributo de tupla de entrada necessário 'speech' está ausente na primeira*"
	"*CDIST3802E: Operador WatsonSTT: o atributo de tupla de entrada necessário 'speech' não é do tipo*"
)

errorCodes_ja_JP=(
	"*CDIST3801E: オペレーター WatsonSTT: 必須の入力タプル属性 'speech' が最初の入力ポー*"
	"*CDIST3802E: オペレーター WatsonSTT: 必須の入力タプル属性 'speech' が、最初の入力ポートでタイプ*"
)

errorCodes_zh_CN=(
	"*CDIST3801E: 操作程序 WatsonSTT：第一个输入端口中缺少必需输入元组属性*"
	"*CDIST3802E: 操作程序 WatsonSTT：第一个输入端口中的必需输入元组属性*"
)

errorCodes_zh_TW=(
	"*CDIST3801E: 運算子 WatsonSTT：第一個輸入埠中遺漏必要的輸入值組屬性*"
	"*CDIST3802E: 運算子 WatsonSTT：第一個輸入埠中的必要輸入值組屬性*"
)
