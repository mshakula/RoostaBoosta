#ifndef HTTP_GET_REQUEST_H
#define HTTP_GET_REQUEST_H

#include "EthernetInterface.h"

int http_get_request(EthernetInterface* net, char* address, char* payload, char* header, char* respBuffer, size_t respBufferSize);

#endif // HTTP_GET_REQUEST_H
