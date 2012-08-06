/*
 Copyright (c) 2012 Justin Walsh, http://thejustinwalsh.com/
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once
#ifndef server_socket_h_
#define server_socket_h_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "FlashRuntimeExtensions.h"
#include "ss_socket.h"


/* socket_ctx - Every Context needs
 *
 */
typedef struct {
  // We need to hold onto the context to dispatch events from the background thread
  FREContext ctx;
  
  // Server socket, port, and binding address
  int server_socket_fd;
  short server_port;
  struct sockaddr_in server_sockaddr;
  bool is_bound;
  
  // Server thread management
  volatile bool is_listening;
  pthread_t server_thread;
  
  // All sockets this server owns
  ss_socket* sockets[SOMAXCONN];
} context_data;

context_data* context_data_alloc(void);
void context_data_free(context_data* ctxdata);
void generate_error(FREObject* object);
void* serverListeningThread(void *pArg);

/* ServerSocketExtInitializer()
 * The extension initializer is called the first time the ActionScript side of the extension
 * calls ExtensionContext.createExtensionContext() for any context.
 *
 * Please note: this should be same as the <initializer> specified in the extension.xml
 */
void ServerSocketExtInitializer(void** extDataToSet, FREContextInitializer* ctxInitializerToSet, FREContextFinalizer* ctxFinalizerToSet);

/* ServerSocketExtFinalizer()
 * The extension finalizer is called when the runtime unloads the extension. However, it may not always called.
 *
 * Please note: this should be same as the <finalizer> specified in the extension.xml
 */
void ServerSocketExtFinalizer(void* extData);

/* ContextInitializer()
 * The context initializer is called when the runtime creates the extension context instance.
 */
void ServerSocketContextInitializer(void* extData, const uint8_t* ctxType, FREContext ctx, uint32_t* numFunctionsToTest, const FRENamedFunction** functionsToSet);

/* ContextFinalizer()
 * The context finalizer is called when the extension's ActionScript code
 * calls the ExtensionContext instance's dispose() method.
 * If the AIR runtime garbage collector disposes of the ExtensionContext instance, the runtime also calls ContextFinalizer().
 */
void ServerSocketContextFinalizer(FREContext ctx);


FREObject ServerSocketBind(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]);

FREObject ServerSocketClose(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]);

FREObject ServerSocketListen(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]);

FREObject ServerSocketSend(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]);

FREObject ServerSocketRecv(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[]);

#endif

