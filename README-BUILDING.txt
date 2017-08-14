MS3 source code building
========================

First - be sure you have read and understood LICENSE-SOURCE.txt and LICENSE.txt
within the source. As noted, this source is licensed for use on MS3 ONLY, not
other hardware or even MS2. Backporting to MS2 is therefore not permitted.

Note - MS3-1.2.x and later require the 2012 version of the build tools.

WINDOWS
=======
Cygwin is required.
Go to http://www.cygwin.com/ and download and install. Be sure to include
development packages such as 'make' and 'tar'

Note - all building of the MS3 code is from the commandline.

S12X build tools (gcc/binutils) are required.
Go to http://www.msextra.com/tools/ download and extract the Windows binaries
package. Then follow the included README.
Once extracted, the binaries will be in /usr/bin for Cygwin.

To build MS3 code, type 'make' in the ms3 source directory, this will generate
a new 's19'
To edit source files, use a text editor (not a word processor.) Don't try to
use Notepad as it lacks many features needed for code development. Perhaps try
vim, gedit, notepad+

LINUX
=====
Note - all building of the MS3 code is from the commandline.

S12X build tools (gcc/binutils) are required.
Go to http://www.msextra.com/tools/ download and extract the Linux binaries
package. Then follow the included README.
Once extracted, the binaries will be in /usr/bin

To build MS3 code, type 'make' in the ms3 source directory, this will generate
a new 's19'
To edit source files, use a text editor (not a word processor.)

MAC
===
A pre-built toolchain is not provided at this time. Try the source.

11 July 2014
