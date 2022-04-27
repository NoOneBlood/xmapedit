 The XMAPEDIT for Windows by nuke.YKT and NoOne
 =================

 * Website:  http://cruo.bloodgame.ru
 * E-Mail:   baitd@yandex.ru
 
This is an extended version of original Blood map editor with
fixed bugs, lots of new features and extended limits.
 
It's compatible with vanilla Blood, supports widescreen resolutions,
includes modern object types for level designers and most of the
features of the DOS XMAPEDIT version.
 
The Windows version uses the following source code parts:
 
Jonathon Fowler BUILD Engine port:
 * Website:       http://www.jonof.id.au
 * Source code:   https://github.com/jonof/jfbuild
 
nuke.YKT NBlood and SAMapedit port
 * Source code:   https://github.com/nukeykt/NBlood
 * Source code:   https://github.com/nukeykt/samapedit
 
Ken Silverman BUILD Engine:
 * Website: http://www.advsys.net/ken

Minimum system requirements
---------------------------
* 32 or 64-bit CPU. These have been tried first-hand:
  * Intel x86, x86_64
* A modern operating system:
  * Windows Vista, 7, 8/10+
* Blood (with Plasma Pack 1.10 or higher recommended)

Compilation
-----------
XMAPEDIT for x86 platforms can be compiled under Windows using
Microsoft Visual C++ 2019 and NMAKE. Note that other compilers
untested, but there is always high chance you got it since
original Jonathon Fowler's code can, so feel free to test other
compilers by yourself if required.

1. Clone this repository or unpack the source archive (you may need to do the same for jfaudiolib).
2. Open the command-line build prompt. e.g. x86 Native Tools Command Prompt for VS 2019.
3. Change into the xmapedit source code folder, then compile with: `nmake /f Makefile.msvc`
4. Once you got it compiled, you may find the xmapedit binary in the `mapedit-data` directory.


Compilation options
-------------------
Some engine features may be enabled or disabled at compile time. These can be passed
to the MAKE tool, or written to a Makefile.user (Makefile.msvcuser for MSVC) file in
the source directory.

These options are available:

 * `RELEASE=1` – build with optimisations for release.
 * `RELEASE=0` – build for debugging.
 * `WITHOUT_GTK=1` – disable use of GTK+ to provide launch windows and load/save file choosers.

Polymost renderer is disabled since it is not compatible with gfx
drawing functions provided fur GUI drawing, still the following options is
available as well:

 * `USE_POLYMOST=1` – enable the true 3D renderer.
 * `USE_POLYMOST=0` – disable the true 3D renderer.
 * `USE_OPENGL=1` – enable use of OpenGL 2.0 acceleration.
 * `USE_OPENGL=USE_GL2` – enable use of OpenGL 2.0 acceleration. (GCC/clang syntax.)
 * `USE_OPENGL=USE_GLES2` – enable use of OpenGL ES 2.0 acceleration. (GCC/clang syntax.)
 * `USE_OPENGL=0` – disable use of OpenGL acceleration.

Launching the XMAPEDIT
----------------------
1. Create `xmapedit` directory in the Blood directory.
2. Copy `xmapedit.exe` in the Blood directory.
3. Copy `xmapedit.rff` in `xmapedit` directory.
4. Launch `xmapedit.exe`.

If you didn't has `xmapedit.rff` inside `mapedit-data` for
some reason, you can create new one. See `mkxmprff` inside
`tools` directory.

Credits and Thanks
------------------
Original MAPEDIT version by:
Peter Freese, Nick Newhard

Original BUILD Engine by:
Ken Silverman

BARFC (Resource File Builder) by:
sirlemonhead
