# Copyright (C)2018, 2020 International Business Machines Corporation and  
# others. All Rights Reserved.                        
.PHONY: build all executable clean

# Ensure that you have the OpenSSL installed on your Linux machine since 
# we will need the OpenSSL crypto libraries in the compiler command used below.

# Please point this to your correct C++ boost lib directory.
# A quick way is to point this to your fully built streamsx.sttgateway/com.ibm.streamsx.sttgateway/lib directory.
LD_LIBRARY_PATH=$(HOME)/boost_1_73_0/lib
# Please point this to your websocketpp install direcctory
WEBSOCKETPP_INSTALL_DIR=$(HOME)/websocketpp-0.8.2

CPP_FLAGS = -std=c++11

build: executable

all: clean build

executable:
	g++ IBMVoiceGatewayDataSimulator.cpp -o c.out $(CPP_FLAGS) -I $(WEBSOCKETPP_INSTALL_DIR) -lboost_system -lboost_random -lboost_chrono -lpthread -lssl -lcrypto

clean:
	rm -f c.out
