/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) STM32 Port - TCP implementation
 */

#include "eventloop_stm32.h"

#if defined(UA_ARCHITECTURE_STM32)

/*********************/
/* TCP Connection    */
/*********************/

typedef struct {
    UA_ConnectionManager cm;
    UA_EventLoopSTM32 *el;
    size_t sendBufferSize;
    size_t recvBufferSize;
    UA_UInt32 connectTimeout;
} UA_TCPConnectionManager;

static UA_StatusCode
TCP_registerListenSocket(UA_TCPConnectionManager *tcm, UA_RegisteredFD *rfd) {
    /* Register the listen socket with the event loop */
    return UA_EventLoopSTM32_registerFD(tcm->el, rfd);
}

static UA_StatusCode
TCP_registerConnectedSocket(UA_TCPConnectionManager *tcm, UA_RegisteredFD *rfd) {
    /* Register the connected socket with the event loop */
    return UA_EventLoopSTM32_registerFD(tcm->el, rfd);
}

static UA_StatusCode
TCP_connectionSocketCallback(UA_ConnectionManager *cm, UA_RegisteredFD *rfd,
                            short event) {
    UA_TCPConnectionManager *tcm = (UA_TCPConnectionManager*)cm;
    
    if(event & UA_FDEVENT_ERR) {
        UA_LOG_WARNING(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                      "TCP: Error event on socket %u", (unsigned)rfd->fd);
        /* Handle error */
        return UA_STATUSCODE_BADCONNECTIONCLOSED;
    }
    
    if(event & UA_FDEVENT_IN) {
        /* Data available for reading */
        char buffer[tcm->recvBufferSize];
        ssize_t received = recv(rfd->fd, buffer, sizeof(buffer), 0);
        
        if(received > 0) {
            UA_ByteString message = {
                .length = (size_t)received,
                .data = (UA_Byte*)buffer
            };
            
            /* Process the received message */
            if(rfd->application.connection) {
                rfd->application.connection->receiveCallback(
                    rfd->application.connection, &message, rfd->application.context);
            }
        } else if(received == 0) {
            /* Connection closed by peer */
            UA_LOG_INFO(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                       "TCP: Connection closed by peer on socket %u", (unsigned)rfd->fd);
            return UA_STATUSCODE_BADCONNECTIONCLOSED;
        } else {
            /* Error occurred */
            if(errno != EAGAIN && errno != EWOULDBLOCK) {
                UA_LOG_WARNING(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                              "TCP: Recv error on socket %u: %s", (unsigned)rfd->fd, strerror(errno));
                return UA_STATUSCODE_BADCONNECTIONCLOSED;
            }
        }
    }
    
    if(event & UA_FDEVENT_OUT) {
        /* Socket ready for writing */
        /* Handle outgoing data if needed */
    }
    
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
TCP_listenSocketCallback(UA_ConnectionManager *cm, UA_RegisteredFD *rfd,
                        short event) {
    UA_TCPConnectionManager *tcm = (UA_TCPConnectionManager*)cm;
    
    if(event & UA_FDEVENT_IN) {
        /* Accept new connection */
        struct sockaddr_storage client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(rfd->fd, (struct sockaddr*)&client_addr, &client_len);
        if(client_socket == -1) {
            UA_LOG_WARNING(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                          "TCP: Accept failed: %s", strerror(errno));
            return UA_STATUSCODE_GOOD; /* Continue listening */
        }
        
        UA_LOG_INFO(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                   "TCP: New connection accepted on socket %d", client_socket);
        
        /* Create a new registered FD for the client connection */
        UA_RegisteredFD *clientRfd = (UA_RegisteredFD*)UA_calloc(1, sizeof(UA_RegisteredFD));
        if(!clientRfd) {
            close(client_socket);
            return UA_STATUSCODE_BADOUTOFMEMORY;
        }
        
        clientRfd->fd = client_socket;
        clientRfd->listenEvents = UA_FDEVENT_IN;
        clientRfd->callback = TCP_connectionSocketCallback;
        clientRfd->application = rfd->application;
        
        /* Register the client socket */
        UA_StatusCode res = TCP_registerConnectedSocket(tcm, clientRfd);
        if(res != UA_STATUSCODE_GOOD) {
            close(client_socket);
            UA_free(clientRfd);
            return res;
        }
    }
    
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
TCP_openListenConnection(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
                        void *application, void *context,
                        UA_ConnectionManager_connectionCallback connectionCallback) {
    UA_TCPConnectionManager *tcm = (UA_TCPConnectionManager*)cm;
    
    /* Parse parameters */
    const UA_UInt16 *port = (const UA_UInt16*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "port"), &UA_TYPES[UA_TYPES_UINT16]);
    
    if(!port) {
        UA_LOG_ERROR(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                    "TCP: No port specified for listen connection");
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    
    /* Create socket */
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(listenSocket == -1) {
        UA_LOG_ERROR(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                    "TCP: Failed to create listen socket: %s", strerror(errno));
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    /* Set socket options */
    int optval = 1;
    if(setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        UA_LOG_WARNING(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                      "TCP: Failed to set SO_REUSEADDR: %s", strerror(errno));
    }
    
    /* Bind socket */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(*port);
    
    if(bind(listenSocket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        UA_LOG_ERROR(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                    "TCP: Failed to bind to port %u: %s", *port, strerror(errno));
        close(listenSocket);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    /* Start listening */
    if(listen(listenSocket, 5) == -1) {
        UA_LOG_ERROR(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
                    "TCP: Failed to listen on socket: %s", strerror(errno));
        close(listenSocket);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    
    /* Create registered FD */
    UA_RegisteredFD *rfd = (UA_RegisteredFD*)UA_calloc(1, sizeof(UA_RegisteredFD));
    if(!rfd) {
        close(listenSocket);
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    
    rfd->fd = listenSocket;
    rfd->listenEvents = UA_FDEVENT_IN;
    rfd->callback = TCP_listenSocketCallback;
    rfd->application.application = application;
    rfd->application.context = context;
    
    /* Register the listen socket */
    UA_StatusCode res = TCP_registerListenSocket(tcm, rfd);
    if(res != UA_STATUSCODE_GOOD) {
        close(listenSocket);
        UA_free(rfd);
        return res;
    }
    
    UA_LOG_INFO(cm->eventSource.eventLoop->logger, UA_LOGCATEGORY_NETWORK,
               "TCP: Listening on port %u", *port);
    
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
TCP_openConnection(UA_ConnectionManager *cm, const UA_KeyValueMap *params,
                  void *application, void *context,
                  UA_ConnectionManager_connectionCallback connectionCallback) {
    /* Parse the address parameter to determine if this is a listen or connect */
    const UA_String *address = (const UA_String*)
        UA_KeyValueMap_getScalar(params, UA_QUALIFIEDNAME(0, "address"), &UA_TYPES[UA_TYPES_STRING]);
    
    if(!address || UA_String_equal(address, &UA_STRING("opc.tcp://0.0.0.0"))) {
        /* This is a listen connection */
        return TCP_openListenConnection(cm, params, application, context, connectionCallback);
    }
    
    /* This would be a client connection - implement if needed */
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

static void
TCP_free(UA_ConnectionManager *cm) {
    UA_TCPConnectionManager *tcm = (UA_TCPConnectionManager*)cm;
    
    /* Close all registered file descriptors */
    UA_RegisteredFD *rfd;
    ZIP_ITER(UA_FDTree, &tcm->el->el.fds, rfd) {
        if(rfd->fd != UA_INVALID_FD) {
            close(rfd->fd);
        }
    }
    
    UA_free(tcm);
}

/* Public function to create TCP connection manager */
UA_ConnectionManager *
UA_ConnectionManager_new_TCP_STM32(UA_EventLoopSTM32 *el,
                                  size_t sendBufferSize,
                                  size_t recvBufferSize) {
    UA_TCPConnectionManager *tcm = (UA_TCPConnectionManager*)
        UA_calloc(1, sizeof(UA_TCPConnectionManager));
    if(!tcm)
        return NULL;
    
    tcm->cm.eventSource.eventLoop = &el->el.eventLoop;
    tcm->cm.eventSource.eventSourceType = UA_EVENTSOURCETYPE_CONNECTIONMANAGER;
    tcm->cm.protocol = UA_STRING("tcp");
    tcm->cm.openConnection = TCP_openConnection;
    tcm->cm.free = TCP_free;
    
    tcm->el = el;
    tcm->sendBufferSize = sendBufferSize ? sendBufferSize : 8192;
    tcm->recvBufferSize = recvBufferSize ? recvBufferSize : 8192;
    tcm->connectTimeout = 10000; /* 10 seconds */
    
    return &tcm->cm;
}

#endif /* UA_ARCHITECTURE_STM32 */
