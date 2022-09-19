 The XMAPEDIT for Windows by nuke.YKT and NoOne
 =================

 * Website:  http://cruo.bloodgame.ru/xmapedit
 * E-Mail:   baitd@yandex.ru
 
This is an extended version of original Blood game editor with
fixed bugs, lots of new features and extended limits. It supports
editing and creating new maps, QAV or SEQ animation and ART files
which contain the game graphics. 
 
It's compatible with vanilla Blood, supports widescreen resolutions,
includes modern object types for level designers and most of the features
of the DOS XMAPEDIT version.
 
The Windows version uses the following source code parts:
 
Jonathon Fowler BUILD Engine port:
 * Website:       http://www.jonof.id.au
 * Source code:   https://github.com/jonof/jfbuild
 
nuke.YKT NBlood and SAMapedit port
 * Source code:   https://github.com/nukeykt/NBlood
 * Source code:   https://github.com/nukeykt/samapedit
 
Ken Silverman BUILD Engine and KPLIB image decoding library:
 * Website: http://www.advsys.net/ken

Minimum system requirements
---------------------------
* 32 or 64-bit CPU. These have been tried first-hand:
  * Intel x86, x86_64
* A Windows operating system:
  * Windows Vista/7/8/10/11+
  * Windows 98/ME/2K/XP (assuming that built with compatible compiler such as VC2005).
* At least 32MB memory (256 is recommended if you plan to edit ART files)
* Blood (with Plasma Pack 1.10 or higher recommended)

Compilation
-----------
XMAPEDIT for x86 platforms can be compiled under
Windows using Microsoft Visual C++ 2005 or higher
and NMAKE tool:

1. Clone this repository or unpack the source archive (you may need to do the same for jfaudiolib).
2. Open the command-line build prompt. e.g. x86 Native Tools Command Prompt for VS 2005 or higher.
3. Change into the xmapedit source code folder, then compile with: `nmake /f Makefile.msvc`
4. Once you got it compiled, you may find the xmapedit binary in the `mapedit-data` directory.


Compilation options
-------------------
Some engine features may be enabled or disabled at compile time. These can be
changed by editing the makefile. You will find detailed description of
each option in `Makefile.msvc` file.

Here is some of options:

 * `WINCOMPAT=N`	- change global Windows compatibility level (see details in `Makefile.msvc`)
 * `RELEASE=1`		– build with optimisations for release.
 * `RELEASE=2`		– build with a different optimisations for release.
 * `RELEASE=0`		– build for debugging.
 * `USE_ASM=1`		- enable the use of assembly code which can give performance boost on older systems.

Launching the XMAPEDIT
----------------------
1. Create `xmapedit` directory in the Blood directory.
2. Copy `xmapedit.exe` in the Blood directory.
3. Copy `xmapedit.rff` in `xmapedit` directory.
4. Launch `xmapedit.exe`.

If you don't have `xmapedit.rff` inside `mapedit-data` for
some reason, you can create new one. See `mkxmprff` inside
the `tools` directory.

Credits and Thanks
------------------
Original MAPEDIT version by:
Peter Freese, Nick Newhard

Original BUILD Engine by:
Ken Silverman

BARFC (Resource File Builder) by:
sirlemonhead
