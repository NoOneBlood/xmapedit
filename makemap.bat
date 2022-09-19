cls
taskkill /f /IM xmapedit.exe>nul
del mapedit-data\xmapedit.exe
del E:\material\noname\xmapedit.exe
nmake /f Makefile.msvc %1 %2 %3 %4 %5
copy mapedit-data\xmapedit.exe E:\material\noname
::upx E:\material\noname\xmp32.exe
rh --zero E:\material\noname\xmapedit.exe
:end