Based on Simon Tatham's Portable Puzzle Collection - SVN r8446.

Built with Open2x toolchain (arm-open2x-linux-apps-gcc-4.1.1-glibc-2.3.6_i686_linux) and 
Open2x pre-compiled libraries (open2x-libpack-20071903-gcc-4.1.1-glibc-2.3.6).  I replaced
libSDL.a with Paeryn's hardware SDL from http://paeryn.myby.co.uk/, the 21Mb version dated 
23/11/07, but they work just fine with the standard Open2x libraries.

Makefile is the GP2X makefile.  Use it with a standard install of the Open2x toolchain 
described above and you can just type "make" to build the GP2X version.  Makefile.PC
builds Linux-based test versions (and also works under Cygwin for building under Windows)
under similar setups.  The executable built from the GP2X Makefile is statically linked 
with all the necessary libraries and is stripped and passed to UPX to compress it
automatically.

I couldn't be bothered to have the Makefile automatically generated from Recipe, sorry.

The code for the SDL version is mostly contained in sdl.c and Makefile but I've had to
tweak various things in the games.  The GP2X Makefile doesn't do the version-magic that 
the original did, ignores all icons and doesn't build some object files that I never used, 
like some of the solvers etc.

Developer documentation for the puzzle collection can be found here:

 http://www.chiark.greenend.org.uk/~sgtatham/puzzles/devel/
