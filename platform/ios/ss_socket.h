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
#ifndef ss_socket_h_
#define ss_socket_h_

#include <pthread.h>

#define SS_BUFFER_SIZE 1024

typedef struct {
  int index;
  int size;
  pthread_mutex_t lock;
  unsigned char *buffer;
} ss_buffer;

typedef struct {
  int socket_desc;
  ss_buffer read_buffer;
  ss_buffer write_buffer;
} ss_socket;

ss_socket* ss_alloc(int socket_fd);
void ss_free(ss_socket *socket);

int ss_write(ss_buffer *buffer, const unsigned char *data, unsigned int size);
int ss_read(ss_buffer *buffer, unsigned char *data, unsigned int size);

int ss_send(int socket_fd, ss_buffer *buffer);
int ss_recv(int socket_fd, ss_buffer *buffer, unsigned int size);

#endif
