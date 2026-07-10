@echo off
rem Wrapper per lanciare setup-wxwidgets.ps1 senza scrivere a mano la riga
rem di comando PowerShell. Inoltra tutti gli argomenti allo script .ps1, es.:
rem     scripts\setup-wxwidgets.bat -MinGwBin C:\devTools\mingw64\bin -Force
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup-wxwidgets.ps1" %*
exit /b %ERRORLEVEL%
