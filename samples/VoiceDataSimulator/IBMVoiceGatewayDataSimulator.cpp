/*
==============================================
# Licensed Materials - Property of IBM
# Copyright IBM Corp. 2018, 2021
==============================================
*/

/*
==============================================
First created on: Oct/03/2019
Last modified on: Aug/23/2021

This C++ example below can be used to generate voice traffic to test the
other streamsx.sttgateway toolkit example named VoiceGatewayToStreamsToWatsonSTT.
This C++ application is useful to test the IBM Streams application mentioned above in the
absence of a full fledged IBM Voice Gateway test infrastructure. This C++ application
can become handy to test the streamsx.sttgatway toolkit's IBMVoiceGatewaySource operator.
So, this application will mimic the way a real IBM Voice Gateway product will
interact with the IBM Streams application by performing similar message exchanges that
are done to get a live voice call transcribed into text.

When this application is run, it will read fragments of speech data from a 
given WAV file and send it over two channels (two websocket connections) established
with the IBM Streams application mentioned above.


This example code can be built and run from a Linux terminal window by 
using the steps shown below.

1) Ensure that you have installed boost_1_73_0 or higher as explained in the streamsx.sttgateway documentation. (https://ibmstreams.github.io/streamsx.sttgateway/docs/user/overview/)

2) Ensure that you have installed websocket++ v0.8.2 or higher as explained in the streamsx.sttgateway documentation.

3) Ensure that you have the OpenSSL installed on your Linux machine since we will need the OpenSSL libraries in the compiler command used below.

4) Set LD_LIBRARY_PATH=<YOUR_BOOST_INSTALL_DIR>/boost_1_69_0/lib in order to link to the Boost ASIO.
   (A quick way is to point this to your fully built streamsx.sttgateway/com.ibm.streamsx.sttgateway/lib directory.)

5a) Compile this test application as shown below:
g++ IBMVoiceGatewayDataSimulator.cpp -o c.out -std=c++11 -I <YOUR_WEBSOCKETPP_INSTALL_DIR>/websocketpp-0.8.2 -lboost_system -lboost_random -lboost_chrono -lpthread -lssl -lcrypto

5b) If you don't want to give the compiler command yourself, you can try to use the
    Makefile provided in this directory. You have to edit the Makefile to point to the correct
    Boost and websoketpp install directories and then simply run "make clean" and "make".

6) Once it is compiled, you can run it to send speech messages read from specific WAV files to the VoiceGatewayToStreamsToWatsonSTT application running in IBM Streams. You can use the WAV files available from the audio-files directory in the samples directory of the streamsx.gateway toolkit. You can open multiple Linux terminal windows (e-g: 10 of them) and then run this application in quick succession from those terminal windows to send concurrent requests and test how the Streams application handles concurrent requests to perform Speech To Text. If it all works reliably, then the IBM Streams application can be connected to the real IBM Voice Gateway traffic to perform the STT operation. Other possibility is to write a bash shell script and invoke this application 10 times from there to run in the background mode.

This application takes the following command line arguments:
WebSocket URL
Wav file name
Unique Voice Gateway Session Id  (Any random string)
Wav content block size in bytes to send at a time
Delay in milliseconds needed in between sending Wav content blocks

Example invocation of this application is shown below:
./c.out wss://b0513:443 /tmp/test1.wav vgw-session-1 16536 100
==============================================
*/

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <iostream>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef std::shared_ptr<boost::asio::ssl::context> context_ptr;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <sstream>
#include <unistd.h>

// We need this to enable SSL on the websocket client side i.e. this application.
context_ptr on_tls_init() {
	   // This method establishes the SSL negotiation and the connection.
	   // TIP: If we want to know which TLS version gets negotiated between the
	   // client and the server, we can run this command from a client machine:
	   //
	   // openssl s_client -connect TLSHost:port
	   // openssl s_client -connect b0513:8443
	   //
	   // You can read more about that command in this URL:
	   // https://security.stackexchange.com/questions/100029/how-do-we-determine-the-ssl-tls-version-of-an-http-request
	   //
	   // As a client, we can request the server to support only the tlsv12 protocol.
	   // We can disable the other SSL, TLS protocol versions in order to
	   // strengthen the security.
	   // You can read more details about this from here.
	   // https://stackoverflow.com/questions/47096415/how-to-make-boostasio-ssl-server-accept-both-tls-1-1-and-tls-1-2/47097088
	   // https://www.boost.org/doc/libs/1_58_0/doc/html/boost_asio/reference/ssl__context.html
	   // https://www.boost.org/doc/libs/1_58_0/doc/html/boost_asio/reference/ssl__context/method.html
	   //
	   // We can initialize the asio client context with sslv3. Then, we can apply the
	   // no_tlsxxx flags to disable a particular tls version as needed.
    context_ptr ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

	   // We will support only tlsv1.2. Let us disable all the other older
	   // tls versions including the very old ssl v2 and v3 protocols.
    try {
       ctx->set_options(boost::asio::ssl::context::default_workarounds |
                   boost::asio::ssl::context::no_sslv2 |
                   boost::asio::ssl::context::no_sslv3 |
					  boost::asio::ssl::context::no_tlsv1 |
                   boost::asio::ssl::context::single_dh_use);
    } catch (std::exception &e) {
       std::cout << "Error in on_tls_init: " << e.what() << std::endl;
    }

    return ctx;
}

class connection_metadata {
public:
    typedef websocketpp::lib::shared_ptr<connection_metadata> ptr;

    connection_metadata(int id, websocketpp::connection_hdl hdl, std::string uri)
      : m_id(id)
      , m_hdl(hdl)
      , m_status("Connecting")
      , m_uri(uri)
      , m_server("N/A")
    {}

    void on_open(client * c, websocketpp::connection_hdl hdl) {
        m_status = "Open";

        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
    }

    void on_fail(client * c, websocketpp::connection_hdl hdl) {
        m_status = "Failed";

        client::connection_ptr con = c->get_con_from_hdl(hdl);
        m_server = con->get_response_header("Server");
        m_error_reason = con->get_ec().message();
    }
    
    void on_close(client * c, websocketpp::connection_hdl hdl) {
        m_status = "Closed";
        client::connection_ptr con = c->get_con_from_hdl(hdl);
        std::stringstream s;
        s << "close code: " << con->get_remote_close_code() << " (" 
          << websocketpp::close::status::get_string(con->get_remote_close_code()) 
          << "), close reason: " << con->get_remote_close_reason();
        m_error_reason = s.str();
    }

    void on_message(websocketpp::connection_hdl, client::message_ptr msg) {
        if (msg->get_opcode() == websocketpp::frame::opcode::text) {
            m_messages.push_back("<< " + msg->get_payload());
            m_recent_message_received = msg->get_payload();
            // Comment or uncomment the following line for your debug needs.
            // std::cout << "Message Received = " << m_recent_message_received << std::endl;
        } else {
            m_messages.push_back("<< " + websocketpp::utility::to_hex(msg->get_payload()));
        }
    }

    websocketpp::connection_hdl get_hdl() const {
        return m_hdl;
    }
    
    int get_id() const {
        return m_id;
    }
    
    std::string get_status() const {
        return m_status;
    }

    void record_sent_message(std::string message) {
        m_messages.push_back(">> " + message);
    }

    std::string get_recent_message_received() {
       return m_recent_message_received;
    }

    friend std::ostream & operator<< (std::ostream & out, connection_metadata const & data);
private:
    int m_id;
    websocketpp::connection_hdl m_hdl;
    std::string m_status;
    std::string m_uri;
    std::string m_server;
    std::string m_error_reason;
    std::vector<std::string> m_messages;
    std::string m_recent_message_received;
};

std::ostream & operator<< (std::ostream & out, connection_metadata const & data) {
    out << "> URI: " << data.m_uri << "\n"
        << "> Status: " << data.m_status << "\n"
        << "> Remote Server: " << (data.m_server.empty() ? "None Specified" : data.m_server) << "\n"
        << "> Error/close reason: " << (data.m_error_reason.empty() ? "N/A" : data.m_error_reason) << "\n";
    out << "> Messages Processed: (" << data.m_messages.size() << ") \n";

    std::vector<std::string>::const_iterator it;
    for (it = data.m_messages.begin(); it != data.m_messages.end(); ++it) {
        out << *it << "\n";
    }

    return out;
}

class websocket_endpoint {
public:
    websocket_endpoint () : m_next_id(0) {
        m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
        m_endpoint.clear_error_channels(websocketpp::log::elevel::all);

        m_endpoint.init_asio();
        // Sep/11/2019 Senthil added the following code block.
        m_endpoint.set_tls_init_handler(bind(&on_tls_init));
        m_endpoint.start_perpetual();

        m_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(&client::run, &m_endpoint);
    }

    ~websocket_endpoint() {
        m_endpoint.stop_perpetual();
        
        for (con_list::const_iterator it = m_connection_list.begin(); it != m_connection_list.end(); ++it) {
            if (it->second->get_status() != "Open") {
                // Only close open connections
                continue;
            }
            
            std::cout << "> Closing connection " << it->second->get_id() << std::endl;
            
            websocketpp::lib::error_code ec;
            m_endpoint.close(it->second->get_hdl(), websocketpp::close::status::going_away, "", ec);
            if (ec) {
                std::cout << "> Error closing connection " << it->second->get_id() << ": "  
                          << ec.message() << std::endl;
            }
        }
        
        m_thread->join();
    }

    int connect(std::string const & uri) {
        websocketpp::lib::error_code ec;

        client::connection_ptr con = m_endpoint.get_connection(uri, ec);

        if (ec) {
            std::cout << "> Connect initialization error: " << ec.message() << std::endl;
            return -1;
        }

        int new_id = m_next_id++;
        connection_metadata::ptr metadata_ptr = websocketpp::lib::make_shared<connection_metadata>(new_id, con->get_handle(), uri);
        m_connection_list[new_id] = metadata_ptr;

        con->set_open_handler(websocketpp::lib::bind(
            &connection_metadata::on_open,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));
        con->set_fail_handler(websocketpp::lib::bind(
            &connection_metadata::on_fail,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));
        con->set_close_handler(websocketpp::lib::bind(
            &connection_metadata::on_close,
            metadata_ptr,
            &m_endpoint,
            websocketpp::lib::placeholders::_1
        ));
        con->set_message_handler(websocketpp::lib::bind(
            &connection_metadata::on_message,
            metadata_ptr,
            websocketpp::lib::placeholders::_1,
            websocketpp::lib::placeholders::_2
        ));

        m_endpoint.connect(con);

        return new_id;
    }

    void close(int id, websocketpp::close::status::value code, std::string reason) {
        websocketpp::lib::error_code ec;
        
        con_list::iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            std::cout << "> No connection found with id " << id << std::endl;
            return;
        }
        
        m_endpoint.close(metadata_it->second->get_hdl(), code, reason, ec);
        if (ec) {
            std::cout << "> Error initiating close: " << ec.message() << std::endl;
        }
    }

    void send(int id, std::string message) {
        websocketpp::lib::error_code ec;
        
        con_list::iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            std::cout << "> No connection found with id " << id << std::endl;
            return;
        }
        
        m_endpoint.send(metadata_it->second->get_hdl(), message, websocketpp::frame::opcode::text, ec);
        if (ec) {
            std::cout << "> Error sending message: " << ec.message() << std::endl;
            return;
        }
        
        metadata_it->second->record_sent_message(message);
    }

    void sendBinary(int id, std::string message) {
        websocketpp::lib::error_code ec;
        
        con_list::iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            std::cout << "> No connection found with id " << id << std::endl;
            return;
        }
        
        m_endpoint.send(metadata_it->second->get_hdl(), message, websocketpp::frame::opcode::binary, ec);
        if (ec) {
            std::cout << "> Error sending message: " << ec.message() << std::endl;
            return;
        }
        
    }

    connection_metadata::ptr get_metadata(int id) const {
        con_list::const_iterator metadata_it = m_connection_list.find(id);
        if (metadata_it == m_connection_list.end()) {
            return connection_metadata::ptr();
        } else {
            return metadata_it->second;
        }
    }

    std::string get_recent_message_received(int id) {
        con_list::const_iterator metadata_it = m_connection_list.find(id);

        if (metadata_it == m_connection_list.end()) {
            return std::string("");
        } else {
            return (metadata_it->second->get_recent_message_received());
        }  
    }    


private:
    typedef std::map<int,connection_metadata::ptr> con_list;

    client m_endpoint;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

    con_list m_connection_list;
    int m_next_id;
};

int main(int argc, char *argv[]) {
    // We are going to make two WebSocket connections to the IBM Streams application.
    // One to mimic the customer channel and the other one for the agent channel.
    if (argc < 6) {
       // User didn't give the required arguments.
       std::cout << "Usage: ./c.out  <WebSocket Server URL> <WAV file name> <VGW Session Id> <Wav content block size in bytes to send at a time> <Delay in milliseconds between sending Wav content blocks>" << std::endl;
       std::cout << "Example: ./c.out wss://b0513:443 /tmp/test1.wav vgw-session-1 16536 100" << std::endl;
       return(0);
    }

    std::string wsUrl = std::string(argv[1]);
    std::string wavFileName = std::string(argv[2]);
    std::string vgwSessionId = std::string(argv[3]);
    int32_t wavBlockSize = atoi(argv[4]);
    int32_t delayBetweenSendingWavContentBlocks = atoi(argv[5]);
    unsigned int microseconds = 1 * 1000 * 1000;

    // Check for the file existence before attempting to read the audio data.
    struct stat fileStat;
    int32_t fileStatReturnCode = stat(wavFileName.c_str(), &fileStat);
				
    if (fileStatReturnCode != 0) {
       // File not found. Exit now.
       std::cout << wavFileName << " is not there." << std::endl;
       return(0);
    }

    // Open the first WebSocket connection to the given URL.
    websocket_endpoint endpoint1;

    int con_id1 = endpoint1.connect(wsUrl);

    if (con_id1 != -1) {
       std::cout << "> Created connection for channel 1 with id " << con_id1 << std::endl;
    } else {
      // Unable to create a connection.
      return(0);
    }

    // Wait for three seconds and check the status of the connection.
    microseconds = 3 * 1000 * 1000;
    usleep(microseconds);

    // Validate the connection opened with the remote WebSocket server to see if it is really there.
    connection_metadata::ptr metadata = endpoint1.get_metadata(con_id1);

    if (metadata) {
       std::stringstream ss;
       ss << *metadata;
       std::string myStr = ss.str();
       std::size_t found = myStr.find("Status: Open");
       // Comment or uncomment the following line for your debug needs.
       // std::cout << "myStr=" << myStr << std::endl;

       if (found == std::string::npos) {
          std::cout << "Connection id " << con_id1 <<
             " for channel 1 is not valid. It failed to open." << std::endl;
          return(0);       
       }
    } else {
       std::cout << "> Unknown connection id " << con_id1 << std::endl;
       return(0);
    }

    // Open the second WebSocket connection to the given URL.
    websocket_endpoint endpoint2;

    int con_id2 = endpoint2.connect(wsUrl);

    if (con_id2 != -1) {
       std::cout << "> Created connection for channel 2 with id " << con_id2 << std::endl;
    } else {
      // Unable to create a connection.
      return(0);
    }

    // Wait for three seconds and check the status of the connection.
    microseconds = 3 * 1000 * 1000;
    usleep(microseconds);

    // Validate the connection opened with the remote WebSocket server to see if it is really there.
    connection_metadata::ptr metadata2 = endpoint2.get_metadata(con_id2);

    if (metadata2) {
       std::stringstream ss;
       ss << *metadata2;
       std::string myStr = ss.str();
       std::size_t found = myStr.find("Status: Open");
       // Comment or uncomment the following line for your debug needs.
       // std::cout << "myStr=" << myStr << std::endl;

       if (found == std::string::npos) {
          std::cout << "Connection id " << con_id2 <<
             " for channel 2 is not valid. It failed to open." << std::endl;
          return(0);       
       }
    } else {
       std::cout << "> Unknown connection id " << con_id2 << std::endl;
       return(0);
    }


    // Let us first start an STT session on connection 1 as a customer channel.
    // In order to do that. we have to send the following JSON message to the WebSocket server.
    std::string sttStartSessionMessage = std::string("{\"action\":\"start\",") +
       std::string("\"siprecMetadata\":{") +
       std::string("\"vgwSessionID\":\"") + vgwSessionId + std::string("\",") +
       std::string("\"vgwSIPCallID\":\"sipCallID123\",") +
       std::string("\"vgwSiprecSIPCallID\":\"siprecCallID123\",") +
       std::string("\"vgwParticipantURI\":\"sip:+19149453000@4.55.11.163:5060\",") +
       std::string("\"vgwIsCaller\":true,") +
       std::string("\"vgwTenantID\":\"vgwTenantID123\",") +
       std::string("\"vgwSIPToURI\":\"vgwSIPToURI123\",") +
       std::string("\"vgwSIPCustomInviteHeaders\": {") +
       std::string("\"Cisco-Guid\": ") +
       std::string("\"3502874379-4113371627-2730858090-1212179876\"}}}");

    // Send the Start STT session message on connection 1.
    endpoint1.send(con_id1, sttStartSessionMessage);

    // Comment or uncomment the following line for your debug needs.
    // std::cout << sttStartSessionMessage << std::endl;

    // Let us next start an STT session on connection 2 as an agent channel.
    // In order to do that. we have to send the following JSON message to the WebSocket server.
    std::string sttStartSessionMessage2 = std::string("{\"action\":\"start\",") +
       std::string("\"siprecMetadata\":{") +
       std::string("\"vgwSessionID\":\"") + vgwSessionId + std::string("\",") +
       std::string("\"vgwSIPCallID\":\"sipCallID123\",") +
       std::string("\"vgwSiprecSIPCallID\":\"siprecCallID123\",") +
       std::string("\"vgwParticipantURI\":\"sip:+15712487798@169.61.56.229\",") +
       std::string("\"vgwIsCaller\":false,") +
       std::string("\"vgwTenantID\":\"vgwTenantID123\",") +
       std::string("\"vgwSIPToURI\":\"vgwSIPToURI123\",") +
       std::string("\"vgwSIPCustomInviteHeaders\": {") +
       std::string("\"Cisco-Guid\": ") +
       std::string("\"3502874379-4113371627-2730858090-1212179876\"}}}");
    
    // Send the Start STT session message on connection 2.
    endpoint2.send(con_id2, sttStartSessionMessage2);

    // Comment or uncomment the following line for your debug needs.
    // std::cout << sttStartSessionMessage2 << std::endl;

    // Wait for five seconds and then check the response received from the WebSocket server.
    microseconds = 5 * 1000 * 1000;
    usleep(microseconds);

    // Check if we got a response from the WebSocket server on connection 1.
    std::string response = endpoint1.get_recent_message_received(con_id1);
    // Comment or uncomment the following line for your debug needs.
    // std::cout << "Response 1 = " << response << std::endl;
    std::size_t found = response.find("listening");

    if (found == std::string::npos) {
       // We didn't get the expected 'state':'listening' response.
       std::cout << "Didn't receive the 'listening' response for channel 1." << std::endl;
       return(0);
    }

    // Check if we got a response from the WebSocket server on connection 2.
    response = endpoint2.get_recent_message_received(con_id2);
    // Comment or uncomment the following line for your debug needs.
    // std::cout << "Response 2 = " << response << std::endl;
    found = response.find("listening");

    if (found == std::string::npos) {
       // We didn't get the expected 'state':'listening' response.
       std::cout << "Didn't receive the 'listening' response for channel 2." << std::endl;
       return(0);
    }

    std::cout << "Start STT session established on two channels." << std::endl;
  
    // We can now read the audio file and send blocks (chunks) of speech data 
    // File is present. Read the binary data now.
    std::ifstream input(wavFileName.c_str(), std::ios::binary);
    std::vector<char> buffer((std::istreambuf_iterator<char>(input)), 
       (std::istreambuf_iterator<char>()));

    // We will send user specified bytes of chunks alternatively to both the channels.
    // Get a plain C buffer of the binary content now.
    char *p = buffer.data();
    int32_t bufSize = buffer.size();
    int32_t chunkStart = 0;
    int32_t chunkSize = 0;
    bool sendItToFirstChannel = true;
    std::cout << "Wav content is being sent now. (" << bufSize << " bytes)" << std::endl;

    // Stay in a loop and send the data now.
    while(bufSize > 0) {
       chunkSize = (bufSize >= wavBlockSize) ? wavBlockSize : bufSize;
       std::string msg = std::string(&p[chunkStart], chunkSize);
          
       // Send it now by alternating between the channels.
       // This allows us to test the logic in the IBM Streams operator to handle two channels.
       if (sendItToFirstChannel == true) {
          // Next chance is for the second channel.
          sendItToFirstChannel = false;
          // Comment or uncomment the following line for your debug needs.
          /*
          std::cout << "Sending to channel 1: BufSize=" << bufSize << 
             ", ChunkSize=" << chunkSize << std::endl; 
          */
          endpoint1.sendBinary(con_id1, msg);
          // Wait for 200 milliseconds to somewhat represent the speed of human speech
          microseconds = 200 * 1000;
          usleep(microseconds);
       } else {
          // Next chance is for the first channel.
          sendItToFirstChannel = true;
          // Comment or uncomment the following line for your debug needs.
          /*
          std::cout << "Sending to channel 2: BufSize=" << bufSize << 
             ", ChunkSize=" << chunkSize << std::endl; 
          */
          endpoint2.sendBinary(con_id2, msg);
          // Wait for 200 milliseconds to somewhat represent the speed of human speech
          microseconds = delayBetweenSendingWavContentBlocks * 1000;
          usleep(microseconds);
       }
          
       // Readjust the buffer start position.
       chunkStart += chunkSize;
       // Deduct the bytes already sent from the bufSize.
       bufSize -= chunkSize;
    } // End of while loop
    
    std::cout << "Wav content is fully sent." << std::endl;
    // We are done sending the data.
    // Wait for 3  seconds before sending the stop STT session message.
    microseconds = 3 * 1000 * 1000;
    usleep(microseconds);

    // At this time, we can end the STT session on both the connections.
    std::string sttStopSessionMessage = std::string("{\"action\":\"stop\"}");   
    // Send the Stop STT session message on channel 1.
    endpoint1.send(con_id1, sttStopSessionMessage);
    std::string sttStopSessionMessage2 = std::string("{\"action\":\"stop\"}");
    // Send the Stop STT session message on channel 2.
    endpoint2.send(con_id2, sttStopSessionMessage2);
    std::cout << "STT session is stopped on two channels." << std::endl;

    // Wait for 3  seconds before closing the WebSocket connection.
    microseconds = 3 * 1000 * 1000;
    usleep(microseconds);

    // At this point, close both the connections that we opened.
    int close_code = websocketpp::close::status::normal;
    std::string reason;
    std::stringstream ss;
            
    ss >> con_id1 >> close_code;
    std::getline(ss,reason);        
    endpoint1.close(con_id1, close_code, reason);

    std::stringstream ss2;
    ss2 >> con_id2 >> close_code;
    reason = "";
    std::getline(ss2,reason); 
    endpoint2.close(con_id2, close_code, reason);
    std::cout << "WebSocket connection is now closed on two channels." << std::endl;

    // Wait for 3  seconds before exiting the application..
    microseconds = 3 * 1000 * 1000;
    usleep(microseconds);
    // We can exit now.
    return(0);
}
