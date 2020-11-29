# Copyright (C)2018, 2020 International Business Machines Corporation and  
# others. All Rights Reserved.                        
.PHONY: build all distributed clean

# You must ensure that the following line points to your 
# correct STT Gateway toolkit directory that is of v2.2.3 or above.
STREAMS_STTGATEWAY_TOOLKIT ?= $(PWD)/../../com.ibm.streamsx.sttgateway
# You must ensure that the following line points to your 
# correct websocket toolkit directory that is of v1.0.9 or above.
STREAMS_WEBSOCKET_TOOLKIT ?= $(HOME)/com.ibm.streamsx.websocket
# You must ensure that the following line points to your
# correct JSON toolkit directory that is of v1.4.6 or above.
STREAMS_JSON_TOOLKIT ?= $(STREAMS_INSTALL)/toolkits/com.ibm.streamsx.json
# You must ensure that the following line points to your
# correct INET toolkit directory that is of v2.3.6 or above.
STREAMS_INET_TOOLKIT ?= $(STREAMS_INSTALL)/toolkits/com.ibm.streamsx.inet
# You must ensure that the following line points to your
# correct STTGatewayUtils toolkit directory that is of v1.0.0 or above.
STT_GATEWAY_UTILS ?= $(PWD)/../STTGatewayUtils

ifeq ($(STREAMS_STUDIO_BUILDING), 1)
    $(info Building from Streams Studio, use env vars set by studio)
    SPLC = $(STREAMS_STUDIO_SC_PATH)
    DATA_DIR = $(STREAMS_STUDIO_DATA_DIRECTORY)
    OUTPUT_DIR = $(STREAMS_STUDIO_OUTPUT_DIRECTORY)
    TOOLKIT_PATH = $(STREAMS_STUDIO_SPL_PATH)
else
    $(info build use env settings)
    ifndef STREAMS_INSTALL
        $(error require streams environment STREAMS_INSTALL)
    endif
    SPLC = $(STREAMS_INSTALL)/bin/sc
    DATA_DIR = data
    OUTPUT_DIR = output
    TOOLKIT_PATH = $(STREAMS_JSON_TOOLKIT):$(STREAMS_INET_TOOLKIT):$(STREAMS_S2T_TOOLKIT):$(STREAMS_WEBSOCKET_TOOLKIT):$(STREAMS_STTGATEWAY_TOOLKIT):$(STT_GATEWAY_UTILS)
endif

SPL_MAIN_COMPOSITE = com.ibm.streamsx.sttgateway.sample.watsonstt::VgwDataRouterToWatsonSTT
SPLC_FLAGS = -a --c++std=c++11 -x '-I $(STREAMS_STTGATEWAY_TOOLKIT)/impl/include'
SPL_CMD_ARGS ?=

build: distributed

all: clean build

distributed:
	$(SPLC) $(SPLC_FLAGS) -M $(SPL_MAIN_COMPOSITE) -t ${TOOLKIT_PATH} --data-dir $(DATA_DIR) --output-dir $(OUTPUT_DIR) $(SPL_CMD_ARGS)

clean:
	$(SPLC) $(SPLC_FLAGS) -M $(SPL_MAIN_COMPOSITE) -t ${TOOLKIT_PATH} --data-dir $(DATA_DIR) --output-dir $(OUTPUT_DIR) -C $(SPL_CMD_ARGS)
	rm -rf output
