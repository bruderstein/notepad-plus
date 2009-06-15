What is Notepad++ ?
===================

Notepad++ is a free (free as in "free speech", but also as in "free beer") source code editor and Notepad replacement, which supports several programming languages, running under the MS Windows environment.

Obtaining Sources
=================

Source code for Notepad++ is available using git or svn.  See the main Sourceforge project page for links.

To build Notepad++ package from source code:
============================================

For generating the executable file (notepad++.exe), you can use VC++ 7 / VC++ 8 or MinGW 3.0 / 2.X (a makefile is available, but not maintained).
A CMakeLists.txt (located in the PowerEditor\src directory) is available for generating the different VC project and MinGW makefile via cmake.

For generating the the dll file (SciLexer.dll), you have 2 choices : VC++ 6 (from v2.5) or MinGW 3.0 / 2.X 

All the binaries will be built in the directory notepad++\PowerEditor\bin


Go to Notepad++ official site for more information :
http://notepad-plus.sourceforge.net/

Don HO
don.h@free.fr
