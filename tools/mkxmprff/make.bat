@echo off
del xmapedit.rff>nul
barfc xmapedit -a @xmapedit
rename XMAPEDIT.RFF xmapedit.rff
copy /Y xmapedit.rff ..\..\mapedit-data\xmapedit
del xmapedit.h>nul
pause