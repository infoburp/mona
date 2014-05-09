not reddit mona lisa meme thing in c.

This fork (c) Kyle Mclamb, MIT License.
originally written by nick welch <nick@incise.org>.

COMPILE
=======
You need everything listed here:

	apt-get install build-essential pkg-config libcairo2-dev libsdl2-dev

If you want to use nvideo try:

	apt-get install nvidia-cuda-toolkit

That's just nvcc and the runtime, I think.

As far as I know POSIX is required but not anything linux-specific. I have not,
however, tried actually compiling with FreeBSD/Mac OSX/MinGW. Have fun.

To actually compile:

	make

That's it. you can use CUDA=yes to cuda, NOSDL=yes to only require cairo.

USE
===
	./mona -h

will tell you about the current array of command line flags available to you. 
Don't ask about the abbreviations, please.

CONTRIBUTING
============
submit pull requests, or just fork and tell me about it. The code is a bit
hairy and slow, be warned.
