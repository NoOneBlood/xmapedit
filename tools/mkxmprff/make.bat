@echo off
del xmapedit.rff>nul
barfc xmapedit -a @xmapedit
rename XMAPEDIT.RFF xmapedit.rff
del xmapedit.h>nul
pause