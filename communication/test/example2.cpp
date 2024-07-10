#include <ccrtp/rtp.h>
#include <iostream>
#include <cstring>

using namespace ost;
using namespace std;

int main()
{
	// Define network parameters
	InetHostAddress localAddr("0.0.0.0");  // Bind to all available network interfaces
	tpport_t localPort = 5000;             // Local port to bind to
	tpport_t remotePort = 5000;            // Remote port to send packets to
	InetHostAddress remoteAddr("255.255.255.255"); // Broadcast address

	// Create RTP session
	RTPSession rtpSession(localAddr, localPort);

	// Set the destination address and port for the RTP session
	rtpSession.addDestination(remoteAddr, remotePort);

	// Set up RTP session parameters
	rtpSession.setPayloadFormat(StaticPayloadFormat(sptPCMU));
	rtpSession.startRunning();

	// Create a message to send
	const char *message = "Hello, everyone!";
	size_t messageLength = strlen(message);

	// Send the message
	rtpSession.putData(0, (const uint8_t *)message, messageLength);

	cout << "Message sent to everyone: " << message << endl;

	return 0;
}