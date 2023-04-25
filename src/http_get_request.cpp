#include "http_get_request.h"

int http_get_request(EthernetInterface* net, const char* address, const char* payload, const char* header, char* respBuffer, size_t respBufferSize) {
    TCPSocket socket;

    // Resolve the server's IP address and establish a connection
    SocketAddress a;
    int ret = net->gethostbyname(address, &a);
    if (ret != 0) {
        return -1; // Error resolving the address
    }

    a.set_port(80);
    ret = socket.connect(a);
    if (ret != 0) {
        return -2; // Error connecting to the server
    }

    // Prepare and send an HTTP GET request to the server
    char sbuffer[256];
    snprintf(sbuffer, sizeof(sbuffer), "GET /%s HTTP/1.1\r\nHost: %s\r\n%s\r\n\r\n", payload, address, header);

    int scount = socket.send(sbuffer, strlen(sbuffer));
    if (scount < 0) {
        return -3; // Error sending the request
    }

    // Receive the server's response and store it in respBuffer
    int rcount = socket.recv(respBuffer, respBufferSize);
    if (rcount < 0) {
        return -4; // Error receiving the response
    }

    // Close the socket and return success
    socket.close();
    return 0;
}
