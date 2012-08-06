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

#include "ServerSocket.h"

#define READ_LENGTH 512

context_data* context_data_alloc()
{
  // Allocate our context struct
  context_data* ctxdata = malloc(sizeof(context_data));
  
  // Initialize our sockets array
  memset(&ctxdata->sockets, 0, sizeof(ctxdata->sockets));
  
  // Hand back the context data
  return ctxdata;
}

void context_data_free(context_data* ctxdata)
{
  free(ctxdata);
}

void generate_error(FREObject* object)
{
  FRENewObject((const uint8_t*)"Object", 0, NULL, object, NULL);
  
  FREObject fre_success;
  FRENewObjectFromBool(false, &fre_success);
  FRESetObjectProperty(*object, (const uint8_t*)"success", fre_success, NULL);
  
  FREObject fre_error;
  const char* error_str = strerror(errno);
  FRENewObjectFromUTF8(strlen(error_str), (const uint8_t*)error_str, &fre_error);
  FRESetObjectProperty(*object, (const uint8_t*)"error", fre_error, NULL);
}

void* serverListeningThread(void *pArg)
{
  char event_level[128];
  int i = 0, error = 0, num_sockets = 0, high_socket = 0;
  struct timeval timeout;
  context_data* ctxdata = (context_data *) pArg;
  ss_socket* s = NULL;
    
  // Select list of sockets
  fd_set socket_read_set, socket_write_set;
  
  ctxdata->is_listening = true;
  while (ctxdata->is_listening) {
    // Zero out our socket set
    FD_ZERO(&socket_read_set); FD_ZERO(&socket_write_set);
    
    // Add our primary listening socket into the set
    FD_SET(ctxdata->server_socket_fd, &socket_read_set);
    
    // Set the high_socket to at least our listener
    high_socket = ctxdata->server_socket_fd;
    
    // Add all other sockets into the set
    for (s = NULL, i = 0; i < SOMAXCONN; ++i) {
      s = ctxdata->sockets[i]; if (s == NULL) continue;
      
      // And the socket to the read set always, and the write set if we have data
      FD_SET(s->socket_desc, &socket_read_set);
      if (s->write_buffer.index > 0) {
        FD_SET(s->socket_desc, &socket_write_set);
      }
      
      // Store the high socket for select
      if (s->socket_desc > high_socket) high_socket = s->socket_desc;
    }
    
    // Set our timeout value so we don't block forever (512ms)
    timeout.tv_sec = 0;
    timeout.tv_usec = 512000;
    
    // Select on our sockets
    num_sockets = select(high_socket+1, &socket_read_set, &socket_write_set, (fd_set *)NULL, &timeout);
    if (num_sockets <= 0) continue;
    
    // Check to see if we have a pending connection
    ////
    if (FD_ISSET(ctxdata->server_socket_fd, &socket_read_set)) {
      // Clear this socket from the set
      FD_CLR(ctxdata->server_socket_fd, &socket_read_set);
      
      // Get the new connection
      int connection_fd = accept(ctxdata->server_socket_fd, NULL, NULL);
      
      // Handle an error if needed
      if (connection_fd < 0) {
        close(ctxdata->server_socket_fd);
        
        // Dispatch SocketIOError Status Event, with an error message
        #pragma mark StatusEvent -> SocketIOError
        FREDispatchStatusEventAsync(ctxdata->ctx, (const uint8_t*)"SocketIOError", (const uint8_t*)"Incoming socket rejected");
        
        continue;
      }
      
      // Find a home for this connection
      for (s = NULL, i = 0; i < SOMAXCONN; ++i) {
        s = ctxdata->sockets[i];
        if (s == NULL) { s = ctxdata->sockets[i] = ss_alloc(connection_fd); break; }
      }
      
      // Refuse the connection if we are unable to store it
      if (s == NULL) { close(connection_fd); continue; }
      
      // Set the incoming socket to non-blocking
      error = fcntl(s->socket_desc, F_SETFL, O_NONBLOCK);
      if (error < 0) {
        close(s->socket_desc);
        ss_free(s);
        
        // Dispatch SocketIOError Status Event, with an error message
        #pragma mark StatusEvent -> SocketIOError
        FREDispatchStatusEventAsync(ctxdata->ctx, (const uint8_t*)"SocketIOError", (const uint8_t*)"Incoming socket rejected, unable to set the socket to non-blocking mode");
        
        continue;
      }
      
      // Dispatch SocketOpened Status Event, with the index of the socket
      #pragma mark StatusEvent -> SocketOpened
      sprintf(event_level, "%d", i);
      FREDispatchStatusEventAsync(ctxdata->ctx, (const uint8_t*)"SocketOpened", (const uint8_t*)event_level);
    }
    
    // Check to see if we need to read or write...
    for (s = NULL, i = 0; i < SOMAXCONN; ++i) {
      s = ctxdata->sockets[i]; if (s == NULL) continue;
      
      // Read the data from the socket
      ////
      if (FD_ISSET(s->socket_desc, &socket_read_set)) {
        FD_CLR(s->socket_desc, &socket_read_set);
        
        int len = ss_recv(s->socket_desc, &s->read_buffer, READ_LENGTH);
        if (len > 0) {
          // Dispatch SocketDataReady Status Event, with the index of the socket, and the length of the data
          #pragma mark StatusEvent -> SocketDataReady
          sprintf(event_level, "%d,%d", i, len);
          FREDispatchStatusEventAsync(ctxdata->ctx, (const uint8_t*)"SocketDataReady", (const uint8_t*)event_level);
          
        }
        else if (len == 0) {
          // Connection was closed
          ss_free(s);
          s = ctxdata->sockets[i] = NULL;
          
          // Dispatch SocketClosed Status Event, with the index of the socket
          #pragma mark StatusEvent -> SocketClosed
          sprintf(event_level, "%d", i);
          FREDispatchStatusEventAsync(ctxdata->ctx, (const uint8_t*)"SocketClosed", (const uint8_t*)event_level);
          
          // Since the socket is closed skip to the next socket
          continue;
        }
        else {
          // Dispatch SocketIOError Status Event, with an error message
          #pragma mark StatusEvent -> SocketIOError
          FREDispatchStatusEventAsync(ctxdata->ctx, (const uint8_t*)"SocketIOError", (const uint8_t*)strerror(errno));
        }
      }
      
      // Write the data to the socket
      ////
      if (FD_ISSET(s->socket_desc, &socket_write_set)) {
        FD_CLR(s->socket_desc, &socket_write_set);
        
        int len = ss_send(s->socket_desc, &s->write_buffer);
        if (len < 0) {
          // Dispatch SocketIOError Status Event, with an error message
          #pragma mark StatusEvent -> SocketIOError
          FREDispatchStatusEventAsync(ctxdata->ctx, (const uint8_t*)"SocketIOError", (const uint8_t*)strerror(errno));
        }
      }
    }
  }
  
  // Disconnect everyone and free up our sockets
  for (s = NULL, i = 0; i < SOMAXCONN; ++i) {
    s = ctxdata->sockets[i]; if (s == NULL) continue;
    close(s->socket_desc);
    ss_free(s);
  }
  
  // Dispatch SocketShutdown Status Event, letting the AS layer know that the server closed and all sockets are invalid
  #pragma mark StatusEvent -> SocketShutdown
  FREDispatchStatusEventAsync(ctxdata->ctx, (const uint8_t*)"SocketShutdown", (const uint8_t*)"");
  
  return NULL;
}

/* ServerSocketExtInitializer()
 * The extension initializer is called the first time the ActionScript side of the extension
 * calls ExtensionContext.createExtensionContext() for any context.
 *
 * Please note: this should be same as the <initializer> specified in the extension.xml
 */
void ServerSocketExtInitializer(void** extDataToSet, FREContextInitializer* ctxInitializerToSet, FREContextFinalizer* ctxFinalizerToSet)
{
  *extDataToSet = NULL;
  *ctxInitializerToSet = &ServerSocketContextInitializer;
  *ctxFinalizerToSet = &ServerSocketContextFinalizer;
}

/* ServerSocketExtFinalizer()
 * The extension finalizer is called when the runtime unloads the extension. However, it may not always called.
 *
 * Please note: this should be same as the <finalizer> specified in the extension.xml
 */
void ServerSocketExtFinalizer(void* extData)
{
  
}

/* ContextInitializer()
 * The context initializer is called when the runtime creates the extension context instance.
 */
void ServerSocketContextInitializer(void* extData, const uint8_t* ctxType, FREContext ctx, uint32_t* numFunctionsToTest, const FRENamedFunction** functionsToSet)
{  
  // Initialize our context data
  context_data* ctxdata = context_data_alloc();
  ctxdata->ctx = ctx;
  FRESetContextNativeData(ctx, ctxdata);

  
  /* The following code describes the functions that are exposed by this native extension to the ActionScript code.
   * As a sample, the function isSupported is being provided.
   */
  *numFunctionsToTest = 5;
  
  FRENamedFunction* func = (FRENamedFunction*) malloc(sizeof(FRENamedFunction) * (*numFunctionsToTest));
  
  func[0].name = (const uint8_t*) "close";
  func[0].functionData = NULL;
  func[0].function = &ServerSocketClose;
  
  func[1].name = (const uint8_t*) "bind";
  func[1].functionData = NULL;
  func[1].function = &ServerSocketBind;
    
  func[2].name = (const uint8_t*) "listen";
  func[2].functionData = NULL;
  func[2].function = &ServerSocketListen;
  
  func[3].name = (const uint8_t*) "send";
  func[3].functionData = NULL;
  func[3].function = &ServerSocketSend;
  
  func[4].name = (const uint8_t*) "recv";
  func[4].functionData = NULL;
  func[4].function = &ServerSocketRecv;
  
  *functionsToSet = func;
}

/* ContextFinalizer()
 * The context finalizer is called when the extension's ActionScript code
 * calls the ExtensionContext instance's dispose() method.
 * If the AIR runtime garbage collector disposes of the ExtensionContext instance, the runtime also calls ContextFinalizer().
 */
void ServerSocketContextFinalizer(FREContext ctx)
{
  context_data* ctxdata = NULL;
  FREGetContextNativeData(ctx, (void**)&ctxdata);
  if (ctxdata == NULL) return;
  
  // Shutdown the server
  ServerSocketClose(ctx, NULL, 0, NULL);
  
  // Cleanup our context data
  context_data_free(ctxdata);
  FRESetContextNativeData(ctx, NULL);
}

FREObject ServerSocketClose(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])
{
  context_data* ctxdata = NULL;
  FREGetContextNativeData(ctx, (void**)&ctxdata);
  if (ctxdata == NULL) return NULL;
  if (ctxdata->is_listening == false) return NULL;
  
  // Shutdown the listening thread
  ctxdata->is_listening = false;
  pthread_join(ctxdata->server_thread, NULL);
  
  // Free the context data
  context_data_free(ctxdata);
  FRESetContextNativeData(ctx, NULL);
  
  return NULL;
}

/* bind(localPort:int = 0, localAddress:String = "0.0.0.0"):void
 * Bind to the local port and local address passed in from AS
 * return - result object
 */
FREObject ServerSocketBind(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])
{
  FREObject object = NULL;
  context_data* ctxdata = NULL;
  FREGetContextNativeData(ctx, (void**)&ctxdata);
  if (ctxdata == NULL) return NULL;
  if (ctxdata->is_bound == true) return NULL;
  
  int opt_val = 1; // YES
  int error = 0;
  
  // Create the socket file descriptor
  ctxdata->server_socket_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (ctxdata->server_socket_fd < 0) goto ServerSocketBindError;
  
  // Set the socket up for reuse
  setsockopt(ctxdata->server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));
  
  // Set our server socket to non-blocking
  error = fcntl(ctxdata->server_socket_fd, F_SETFL, O_NONBLOCK);
  if (error < 0) goto ServerSocketBindError;
  
  // Read the values from the AS layer
  int port = 0;
  uint32_t address_length = 0;
  const char* address = NULL;
  FREGetObjectAsInt32(argv[0], &port);
  FREGetObjectAsUTF8(argv[1], &address_length, (const uint8_t**)&address);
  
  // Setup the sockaddr_in structure with the port and address
  memset(&ctxdata->server_sockaddr, 0, sizeof(ctxdata->server_sockaddr));
  ctxdata->server_sockaddr.sin_family = AF_INET;
  ctxdata->server_sockaddr.sin_port = htons(port);
  
  // Bind to any, or bind to a specific address if provided
  if (strcmp(address, "0.0.0.0") == 0) {
    ctxdata->server_sockaddr.sin_addr.s_addr = INADDR_ANY;
  }
  else {
    inet_pton(AF_INET, address, &(ctxdata->server_sockaddr.sin_addr));
  }
  
  // Attempt to bind the socket to the port and interface
  error = bind(ctxdata->server_socket_fd, (struct sockaddr *)&ctxdata->server_sockaddr, sizeof(ctxdata->server_sockaddr));
  if (error < 0) goto ServerSocketBindError;
  
  ctxdata->is_bound = true;
  
  // Get the port that we just bound to
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if (getsockname(ctxdata->server_socket_fd, (struct sockaddr *)&sin, &len) == -1) goto ServerSocketBindError;
  port = ntohs(sin.sin_port);
  
  // Create the return object
  FRENewObject((const uint8_t*)"Object", 0, NULL, &object, NULL);
  
  // Set the success property
  FREObject fre_success;
  FRENewObjectFromBool(true, &fre_success);
  FRESetObjectProperty(object, (const uint8_t*)"success", fre_success, NULL);
  
  // Set the port property
  FREObject fre_port;
  FRENewObjectFromInt32(port, &fre_port);
  FRESetObjectProperty(object, (const uint8_t*)"localPort", fre_port, NULL);
  
  return object;
  
ServerSocketBindError:
  generate_error(&object);
  return object;
}

FREObject ServerSocketListen(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])
{
  FREObject object = NULL;
  context_data* ctxdata = NULL;
  FREGetContextNativeData(ctx, (void**)&ctxdata);
  if (ctxdata == NULL) return NULL;
  
  if (ctxdata->is_bound == false) return NULL;
  if (ctxdata->is_listening == true) return NULL;
  
  // Get the backlog from the AS layer
  int backlog = 0;
  FREGetObjectAsInt32(argv[0], &backlog);
  
  // Correct the backlog
  if (backlog > SOMAXCONN) backlog = SOMAXCONN;

  // Open the socket for listening
  int error = listen(ctxdata->server_socket_fd, backlog);
  if (error < 0) goto ServerSocketListenError;
  
  // Create the connection handler thread, this thread will mark the context as listening when it starts
  error = pthread_create(&ctxdata->server_thread, NULL, serverListeningThread, (void *)ctxdata);
  if (error < 0) goto ServerSocketListenError;
  
  // Create the return object
  FRENewObject((const uint8_t*)"Object", 0, NULL, &object, NULL);
  
  // Set the success property
  FREObject fre_success;
  FRENewObjectFromBool(true, &fre_success);
  FRESetObjectProperty(object, (const uint8_t*)"success", fre_success, NULL);
  
  return object;
  
ServerSocketListenError:
  generate_error(&object);
  return object;
}

FREObject ServerSocketSend(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])
{
  context_data* ctxdata = NULL;
  FREGetContextNativeData(ctx, (void**)&ctxdata);
  if (ctxdata == NULL) return NULL;
  
  // Read the socket_index from the AS layer
  int socket_index = 0;
  FREGetObjectAsInt32(argv[0], &socket_index);
  
  // Accquire our byte array from the AS layer
  FREByteArray byte_array;
  FREAcquireByteArray(argv[1], &byte_array);
  int length = byte_array.length;
  
  // Write the data to our sockets buffer
  ss_socket* socket = ctxdata->sockets[socket_index];
  ss_write(&socket->write_buffer, byte_array.bytes, length);
  
  // Release our byte array back to the AS layer
  FREReleaseByteArray(argv[1]);
  
  // Return the number of bytes sent
  FREObject fre_length;
  FRENewObjectFromInt32(length, &fre_length);
  return fre_length;
}

FREObject ServerSocketRecv(FREContext ctx, void* funcData, uint32_t argc, FREObject argv[])
{
  context_data* ctxdata = NULL;
  FREGetContextNativeData(ctx, (void**)&ctxdata);
  if (ctxdata == NULL) return NULL;
  
  // Read the socket_index from the AS layer
  int socket_index = 0;
  FREGetObjectAsInt32(argv[0], &socket_index);
  
  // Read the data offset from the AS layer
  int offset = 0;
  FREGetObjectAsInt32(argv[2], &offset);
  
  // Read the data length from the AS layer
  int length = 0;
  FREGetObjectAsInt32(argv[3], &length);
  
  // Accquire our byte array from the AS layer
  FREByteArray byte_array;
  FREAcquireByteArray(argv[1], &byte_array);
  
  // Read the data from our buffer
  ss_socket* socket = ctxdata->sockets[socket_index];
  int actual_length = ss_read(&socket->read_buffer, &byte_array.bytes[offset], length);
  
  // Release our byte array back to the AS layer
  FREReleaseByteArray(argv[1]);
  
  // Return the number of bytes read
  FREObject fre_length;
  FRENewObjectFromInt32(actual_length, &fre_length);
  return fre_length;
}
