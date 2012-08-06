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

#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <sys/socket.h>
#include "ss_socket.h"

/* ss_alloc - Allocate memory for our buffers and initialize the struct with a file descriptor
 */
ss_socket* ss_alloc(int socket_fd)
{
  // Allocate memory for this socket
  ss_socket* socket = malloc(sizeof(ss_socket));
  assert(socket != NULL);
  
  // Set our socket descriptor
  socket->socket_desc = socket_fd;
  
  // Initialize our read buffer
  socket->read_buffer.index = 0;
  socket->read_buffer.size = SS_BUFFER_SIZE;
  pthread_mutex_init(&socket->read_buffer.lock, NULL);
  socket->read_buffer.buffer = malloc(SS_BUFFER_SIZE);
  assert(socket->read_buffer.buffer != NULL);
  
  // Initialie our write buffer
  socket->write_buffer.index = 0;
  socket->write_buffer.size = SS_BUFFER_SIZE;
  pthread_mutex_init(&socket->write_buffer.lock, NULL);
  socket->write_buffer.buffer = malloc(SS_BUFFER_SIZE);
  assert(socket->write_buffer.buffer != NULL);
  
  return socket;
}

/* ss_free - Free the memory and uninitialize the ss_socket struct
 */
void ss_free(ss_socket *socket)
{
  // Invalidate our socket descriptor
  socket->socket_desc = -1;
  
  // Free the read buffer
  free(socket->read_buffer.buffer);
  socket->read_buffer.buffer = NULL;
  socket->read_buffer.index = socket->read_buffer.size = 0;
  pthread_mutex_destroy(&socket->read_buffer.lock);
  
  // Free the write buffer
  free(socket->write_buffer.buffer);
  socket->write_buffer.buffer = NULL;
  socket->write_buffer.index = socket->write_buffer.size = 0;
  pthread_mutex_destroy(&socket->write_buffer.lock);
  
  // Free the memory for this socket
  free(socket);
}

/* ss_write - Write data into the target buffer, grow the buffer as needed to fit the data
 * @param buffer - The target buffer to write to
 * @param data - The data to write into the target buffer
 * @param size - The size of the data
 * @return - The size of the data written to the buffer
 */
int ss_write(ss_buffer *buffer, const unsigned char *data, unsigned int size)
{
  int grow_size = 0;
  unsigned char *new_buffer = NULL;
  
  // Lock our buffer
  pthread_mutex_lock(&buffer->lock);
  
  // Grow our buffer to fit the data if needed
  if (size > (buffer->size - buffer->index)) {
    // Allocate a new buffer on SS_BUFFER_SIZE boundaries
    grow_size = ((size / SS_BUFFER_SIZE) + 1) * SS_BUFFER_SIZE;
    new_buffer = malloc(grow_size);
    assert(new_buffer != NULL);
    
    // Copy the buffer and replace it with our new buffer
    memcpy(new_buffer, buffer->buffer, buffer->size);
    free(buffer->buffer);
    buffer->buffer = new_buffer;
    buffer->size = grow_size;
  }
  
  // Write the data into our buffer
  memcpy(&buffer->buffer[buffer->index], data, size);
  buffer->index = buffer->index + size;
  assert(buffer->index < buffer->size);
  
  // Unlock our buffer
  pthread_mutex_unlock(&buffer->lock);
  
  return size;
}

/* ss_read - Read data out of the target buffer into the data array passed in
 * @param buffer - The target buffer to read from
 * @param data - The data array to write the target buffer data into
 * @param size - The size of the data array
 * @return - The size of the data read from the buffer
 */
int ss_read(ss_buffer *buffer, unsigned char *data, unsigned int size)
{
  int read_length = size;
  
  // Lock our buffer
  pthread_mutex_lock(&buffer->lock);
  
  // If we are attempting to read more data then we have read as much as we have
  if (read_length > (buffer->size - buffer->index)) {
    read_length = buffer->size - buffer->index;
  }
  
  // Copy the data out of the buffer, and adjust the index
  memcpy(data, buffer->buffer, read_length);
  buffer->index = buffer->index - read_length;
  assert(buffer->index >= 0);
  
  // Unlock our buffer
  pthread_mutex_unlock(&buffer->lock);
  
  return read_length;
}

int ss_recv(int socket_fd, ss_buffer *buffer, unsigned int size)
{
  int grow_size = 0;
  unsigned char *new_buffer = NULL;
  
  // Lock our buffer
  pthread_mutex_lock(&buffer->lock);
  
  // Grow our buffer to fit the data if needed
  if (size > (buffer->size - buffer->index)) {
    // Allocate a new buffer on SS_BUFFER_SIZE boundaries
    grow_size = ((size / SS_BUFFER_SIZE) + 1) * SS_BUFFER_SIZE;
    new_buffer = malloc(grow_size);
    assert(new_buffer != NULL);
    
    // Copy the buffer and replace it with our new buffer
    memcpy(new_buffer, buffer->buffer, buffer->size);
    free(buffer->buffer);
    buffer->buffer = new_buffer;
    buffer->size = grow_size;
  }
  
  // Read the data into our buffer (if we actually get data adjust the buffer
  int len = (int)recv(socket_fd, &buffer->buffer[buffer->index], size, 0);
  if (len > 0) {
    buffer->index = buffer->index + len;
    assert(buffer->index < buffer->size);
  }
  
  // Unlock our buffer
  pthread_mutex_unlock(&buffer->lock);
  
  // Return the bytes read
  return len;
}

int ss_send(int socket_fd, ss_buffer *buffer)
{
  // Lock our buffer
  pthread_mutex_lock(&buffer->lock);
  
  // Attempt to send all the bytes in our buffer
  int len = send(socket_fd, buffer->buffer, (ssize_t)buffer->index, 0);
  if (len > 0) {
    // If we have any remaining bytes, then move the memory back to the beginning of the buffer and set the index
    int bytes_left = buffer->index - len;
    if (bytes_left > 0) memmove(buffer->buffer, &buffer->buffer[len], bytes_left);
    buffer->index = bytes_left;
    assert(buffer->index >= 0);
  }
  
  // Unlock our buffer
  pthread_mutex_unlock(&buffer->lock);
  
  return len;
}
