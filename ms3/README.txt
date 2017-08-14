Quick guide instructions to installing MS3
==========================================

Full instructions can be found through the documentation on
http://www.msextra.com

1. If you are presently using a different code version on your MS3 you
should save you settings.
e.g. File->SaveAs in TunerStudio.

2. Ensure tuning software is closed and nothing is using your serial port.

3. Run the supplied ms3loader_win32.exe (e.g. double click on it) and follow the instructions.
NOTE!! Do not use any older MS2 type loaders - they WILL NOT WORK with MS3
ms3loader_win32.exe for Windows.
ms3loader_linux32bit for Linux.
ms3loader_macos for OSX on Intel based Mac.

Let me repeat that - YOU **MUST** USE the supplied ms3loader that matches
your operating system.
Linux and Mac loaders need to be run from the command prompt, starting in
the same directory as the firmware files. Do not try to double-click on them
from a graphical user interface as it won't work.

If you have problems with strange errors when loading the code try again with
a different computer as this has frequently been found to be the problem.

4. Run TunerStudio.
TunerStudio v2.6.x or later is required.

5. If this is your first time, you need to create a project, try to use detect
and then browse to the ini file supplied in this distribution.

6. When upgrading, just connect using your existing project. Usually you will
need to "update Project ini" and browse to the ini file provided in this zip
file.

7. Note!! When first creating a tune, you MUST be connected to the Megasquirt
'online' or open an existing MSQ file. If you start tuning offline with blank
settings you will create huge problems for yourself.

8. Be sure to also read the RELEASE-NOTES.txt

Come to  www.msextra.com  for support.

Please consider making a donation to the developers.
http://www.msextra.com/doc/donations.html
