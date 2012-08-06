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
	import flash.events.Event;
	import flash.events.OutputProgressEvent;
	import flash.events.ProgressEvent;
	import flash.net.Socket;
	import flash.utils.ByteArray;

	public class Socket extends flash.net.Socket
	{
		override public function get bytesAvailable():uint { return _readBuffer.bytesAvailable; }
		override public function get bytesPending():uint { return _writeBuffer.position; }
		override public function get connected():Boolean { return _socketIndex >= 0; }
		override public function get endian():String { return _readBuffer.endian; }
		override public function set endian(value:String):void { _readBuffer.endian = _writeBuffer.endian = value; }
		override public function get objectEncoding():uint { return _readBuffer.objectEncoding; }
		override public function set objectEncoding(value:uint):void { _readBuffer.objectEncoding = _writeBuffer.objectEncoding = value; }
		override public function get timeout():uint { return 0; }
		override public function set timeout(time:uint):void { }
		
		public function Socket(host:String = null, port:int = 0)
		{
			super(null, 0);
			_readBuffer = new ByteArray();
			_writeBuffer = new ByteArray();
		}
		
		override public function connect(host:String, port:int):void { }
		
		override public function close():void
		{
			if (connected == false) return;
			_parent._close(_socketIndex);
		}
		
		override public function flush():void
		{
			if (connected == false) return;
			
			var bytesSent:int = bytesPending;
			_parent._send(_socketIndex, _writeBuffer);
			
			if (bytesSent > 0) {
				dispatchEvent( new OutputProgressEvent(OutputProgressEvent.OUTPUT_PROGRESS) );
			}
		}

		// Read interface
		override public function readBoolean():Boolean { return _readBuffer.readBoolean(); }
		override public function readByte():int { return _readBuffer.readByte(); }
		override public function readBytes(bytes:ByteArray, offset:uint=0, length:uint=0):void { return _readBuffer.readBytes(bytes, offset, length); }
		override public function readDouble():Number { return _readBuffer.readDouble(); }
		override public function readFloat():Number { return _readBuffer.readFloat(); }
		override public function readInt():int { return _readBuffer.readInt(); }
		override public function readMultiByte(length:uint, charSet:String):String { return _readBuffer.readMultiByte(length, charSet); }
		override public function readObject():* { return _readBuffer.readObject(); }
		override public function readShort():int { return _readBuffer.readShort(); }
		override public function readUnsignedByte():uint { return _readBuffer.readUnsignedByte(); }
		override public function readUnsignedInt():uint { return _readBuffer.readUnsignedInt(); }
		override public function readUnsignedShort():uint { return _readBuffer.readUnsignedShort(); }
		override public function readUTF():String { return _readBuffer.readUTF(); }
		override public function readUTFBytes(length:uint):String { return _readBuffer.readUTFBytes(length); }
		
		// Write interface
		override public function writeBoolean(value:Boolean):void { _writeBuffer.writeBoolean(value); checkFlush(); }
		override public function writeByte(value:int):void { _writeBuffer.writeByte(value); checkFlush(); }
		override public function writeBytes(bytes:ByteArray, offset:uint=0, length:uint=0):void { _writeBuffer.writeBytes(bytes, offset, length); checkFlush(); }
		override public function writeDouble(value:Number):void { _writeBuffer.writeDouble(value); checkFlush(); }
		override public function writeFloat(value:Number):void { _writeBuffer.writeFloat(value); checkFlush(); }
		override public function writeInt(value:int):void { _writeBuffer.writeInt(value); checkFlush(); }
		override public function writeMultiByte(value:String, charSet:String):void { _writeBuffer.writeMultiByte(value, charSet); checkFlush(); }
		override public function writeObject(object:*):void { _writeBuffer.writeObject(object); checkFlush(); }
		override public function writeShort(value:int):void { _writeBuffer.writeShort(value); checkFlush(); }
		override public function writeUnsignedInt(value:uint):void { _writeBuffer.writeUnsignedInt(value); checkFlush(); }
		override public function writeUTF(value:String):void { _writeBuffer.writeUTF(value); checkFlush(); }
		override public function writeUTFBytes(value:String):void { _writeBuffer.writeUTFBytes(value); checkFlush(); }
		
		private function checkFlush():void
		{
			if (_writeBuffer.position > _writeTrigger) flush();
		}
		
		internal function _open(parent:ServerSocket, index:int):void
		{
			_parent = parent;
			_socketIndex = index;
		}
		
		internal function _close(silent:Boolean = false):void
		{
			_parent = null;
			_socketIndex = -1;
			
			// If we are not being silenced dispatch the close event
			if (!silent) dispatchEvent( new Event(Event.CLOSE) );
		}
		
		internal function _dataReady(dataLength:int):void
		{
			// Read the data from the native layer
			_parent._recv(_socketIndex, _readBuffer, dataLength);
			
			// Dispatch the progress event with the bytes available
			dispatchEvent( new ProgressEvent(ProgressEvent.SOCKET_DATA, false, false, this.bytesAvailable, 0) );
		}

		// These values are our contract with the ServerSocket for sending and recieving data
		private var _parent:ServerSocket;
		private var _socketIndex:int = -1;
		
		// We need two internal buffers for reading and writing too
		private var _readBuffer:ByteArray;
		private var _writeBuffer:ByteArray;
		
		// This is the trigger that automatically sends the data, be sure to call flush when your done building your packet
		private const _writeTrigger:int = 512;
	}
}
