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

package com.thejustinwalsh.net
{
	import flash.errors.IOError;
	import flash.events.Event;
	import flash.events.ServerSocketConnectEvent;
	import flash.events.StatusEvent;
	import flash.external.ExtensionContext;
	import flash.net.ServerSocket;
	import flash.utils.ByteArray;

	public class ServerSocket extends flash.net.ServerSocket
	{
		public static function get isSupported():Boolean { return true; }

		override public function get bound():Boolean { return _bound; }
		override public function get listening():Boolean { return _listening; }
		override public function get localAddress():String { return _localAddress; }
		override public function get localPort():int { return _localPort; }

		public function ServerSocket()
		{
			_extContext = ExtensionContext.createExtensionContext("com.thejustinwalsh.ane.ServerSocket", "");
			if (_extContext == null) { throw new Error("Unable to aquire native extension context `com.axonsports.ane.ServerSocket`"); }
			
			_extContext.addEventListener(StatusEvent.STATUS, onContextEvent, false, 0, true);
		}
				
		override public function close():void
		{
			// Stop listening for events, since a call to close will dispatch the SocketShutdown Event
			_extContext.removeEventListener(StatusEvent.STATUS, onContextEvent);
			
			// Close the native socket and update our state accordingly
			_extContext.call("close");
			
			// Update the socket state
			_bound = _listening = false;
			_closed = true;
			
			// Close all sockets in the wild, supress the close event if _shutdown was not triggered
			for each (var socket:Socket in _sockets) {
				socket._close(!_shutdown);
			}
		}

		override public function bind(localPort:int = 0, localAddress:String = "0.0.0.0"):void
		{
			// Verify that our localPort is within range
			if (localPort < 0 || localPort > 65535) {
				throw new RangeError("Parameter localPort must be a valid port in the range of (0, 65535)");
			}
			
			// Verify that our localAdress is valid (Currently IPV4 only)
			// IPV4, IPV6, hostname -> localAddress.match(/^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$|^(([a-zA-Z]|[a-zA-Z][a-zA-Z0-9\-]*[a-zA-Z0-9])\.)*([A-Za-z]|[A-Za-z][A-Za-z0-9\-]*[A-Za-z0-9])$|^\s*((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:)))(%.+)?\s*$/)
			if (localAddress.match(/\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\b/) == null) {
				throw new ArgumentError("localAddress must be a valid IPV4 address, fork me on github and add hostname and IPV6 support!");
			}
			
			// Verify that we are not already bound to a port
			if (bound) {
				throw new IOError("Already bound to port " + localPort);
			}
			
			// Verify that the socket is not closed
			if (_closed) {
				throw new IOError("Socket is closed");
			}
			
			// Attempt to bind to the localPort and localAddress provided
			var result:Object = _extContext.call("bind", localPort, localAddress);
			if (result.success == true)
			{
				_bound = true;
				_localPort = result.localPort;
				_localAddress = localAddress;
			}
			else
			{
				throw new IOError(result.error);
			}
		}

		override public function listen(backlog:int = 0):void
		{
			// Verify our backlog is within range
			if (backlog < 0) {
				throw new RangeError("Parameter backlog must be a number greater then or equal to 0");
			}
			
			// Verify we are bound
			if (_bound == false) {
				throw new IOError("Server is not bound, make a call to bind() before calling listen");
			}
			
			// Verify that the socket is not closed
			if (_closed) {
				throw new IOError("Socket is closed");
			}
			
			// Attempt to listen for incoming connections
			var result:Object = _extContext.call("listen", backlog);
			if (result.success == true)
			{
				_listening = true;
			}
			else
			{
				throw new IOError(result.error);
			}
		}
		
		private function onContextEvent(e:StatusEvent):void
		{
			var code:String = e.code;
			var level:String = e.level;
			var socketIndex:int = 0;
			var socket:Socket = null;
			
			switch (code)
			{
				case "SocketOpened":
					socketIndex = int(level);
					socket = new Socket();
					
					// Initialize our new socket
					socket._open(this, socketIndex);
					
					// Hold on to our socket
					_sockets[socketIndex] = socket;
					
					// Notify of the new connection
					dispatchEvent( new ServerSocketConnectEvent(ServerSocketConnectEvent.CONNECT, false, false, socket) );
					break;
				
				case "SocketClosed":
					socketIndex = int(level);
					
					// Close our socket and free it up
					_close(socketIndex);
					break;
				
				case "SocketDataReady":
					var levelData:Array = level.split(',');
					var dataLength:int = int(levelData[1]);
					socketIndex = int(levelData[0]);
					socket = _sockets[socketIndex];
					
					// Inform our socket of the data
					socket._dataReady(dataLength);
					break;
				
				case "SocketIOError":
					// TODO: Dispatch IOError
					trace(level);
					break;
				
				case "SocketShutdown":
					_shutdown = true;
					dispatchEvent( new Event(Event.CLOSE) );
					close();
					break;
				
				default:
			}
		}
		
		internal function _close(socketIndex:int):void
		{
			var socket:Socket = _sockets[socketIndex];
			socket._close();
			delete _sockets[socketIndex];
		}
		
		internal function _send(socketIndex:int, data:ByteArray):void
		{
			// Copy the data into the native network layer
			_extContext.call("send", socketIndex, data);
			
			// Clear the data from the buffer
			data.position = data.length = 0;
		}
		
		internal function _recv(socketIndex:int, data:ByteArray, dataLength:int):void
		{
			// Bail if we have no data to send
			if (dataLength == 0) return;
			
			// Ensure we have enough space for our incoming data by allocating room for it
			var offset:int = data.length;
			data.length = offset + dataLength;
			
			// Copy the data into the native network layer and adjust the buffer if needed
			var bytesRead:int = _extContext.call("recv", socketIndex, data, offset, dataLength) as int;
			if (bytesRead < data.length - offset) data.length = offset + bytesRead;
		}
		
		private var _extContext:ExtensionContext;
		private var _sockets:Object = {};
		private var _bound:Boolean = false;
		private var _listening:Boolean = false;
		private var _shutdown:Boolean = false;
		private var _closed:Boolean = false;
		private var _localAddress:String = "0.0.0.0";
		private var _localPort:int = 0;
	}
}
