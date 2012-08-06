# ServerSocket Native Extension
Adobe Air Native Extension (ANE) for ServerSocket on mobile platforms

## About
The ServerSocket class is not available on mobile platforms directly from adobe in the Air SDK.  This extension implements the API on the iOS platform.  With your help, it could easily support other mobile platforms as well.

## Building
The native extension is using ruby's rake for it's build system and therefore requires ruby 1.9.3, this is installed by default on OS X Lion.  Additionally, you must have Xcode and the iOS SDK's installed on your system.

To build simply type `rake build` into your terminal from within the root directory of the native extension.  If this is your first time building the build system will generate the file `config/build.yml` and prompt you to edit the file for your system.  The relevant changes are the path to your Adobe Air SDK and what version of the iOS SDK you are using.

After editing your `config/build.yml` file, simply type `rake build` again.  If all goes well you will see the `ServerSocket.ane` file sitting in your `bin` directory. 

## Usage
The package path `com.thejustinwalsh.net` is a direct analog to `flash.net` and the extension implements a working default package as well.  So everywhere you would use `flash.net.ServerSocket` use `com.thejustinwalsh.net.ServerSocket` instead.

## Considerations
Always call the `flush()` function after you are finished writing data to the socket in a given frame of execution.  The current implementation will trigger and automatically send data every 512 bytes, so if you don't call flush and you are not sending exactly 512 bytes, data may get stuck in the buffer.

## License
ServerSocket is license under a permissive MIT source license. Fork well my friends.

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
