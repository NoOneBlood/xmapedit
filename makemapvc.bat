cls
del mapedit-data\xmapedit.exe
nmake /f Makefile.msvc %1 %2 %3 %4 %5 
copy mapedit-data\xmapedit.exe E:\material\noname\xmp32.exe
::E:
::cd \
::start E:\material\noname\xmp64.exe e1m1
::cd \jmapedit