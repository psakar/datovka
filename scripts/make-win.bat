@echo OFF

set NSISPATH="C:\Program Files (x86)\NSIS\makensis.exe"

cd ..

findstr /C:"VERSION =" datovka.pro > version.txt

set "string=var1;var2;var3;"
for /f "tokens=1,2,3 delims= " %%i in (version.txt) do set "variable1=%%i" &set "variable2=%%j" &set "VERSION=%%k"
endlocal

set VERSIONNSIS="%VERSION%"

del version.txt

IF EXIST packages (
  rmdir /S /Q packages
)

@echo.
@echo -----------------------------------------------------------------
@echo This batch creates Datovka packages for Windows in several steps:
@echo 1) build Datovka (datovka.exe) with QT build tools (requires Qt5.4)
@echo 2) create application packages to "packages" folder
@echo 3) create installation package to "packages" 
@echo    folder from NSIS script (requires NSIS)
@echo Current path to NSIS:  %NSISPATH%
@echo Warning: Build requires dependency libraries in the folder "dlls"!
@echo ------------------------------------------------------------------
@echo Current Datovka version: %VERSION%
@echo ------------------------------------------------------------------
@echo.
pause
@echo. 
@echo =================================
@echo Build Datovka normal (v%VERSION%)  
@echo =================================  
qmake.exe datovka.pro -r -spec win32-g++
mingw32-make.exe
mingw32-make.exe clean
@echo.
@echo Build done.
@echo ----------------------------------
@echo Creating normal package ...
set DATOVKAPATH=packages\datovka-%VERSION%
mkdir %DATOVKAPATH%
copy "release\datovka.exe" %DATOVKAPATH%
copy "locale\datovka_cs.qm" %DATOVKAPATH%\locale
copy "AUTHORS" %DATOVKAPATH%
copy "COPYING" %DATOVKAPATH%
copy "Changelog" %DATOVKAPATH%
xcopy "dlls\*" %DATOVKAPATH% /E
@echo.
set SEARCHTEXT="VERSIONXXX"
set file="nsis\datovka-install\datovka-install.nsi"
copy nsis\datovka-install\datovka-install.template %file%
SETLOCAL ENABLEEXTENSIONS
SETLOCAL DISABLEDELAYEDEXPANSION
if "%SEARCHTEXT%"=="" findstr "^::" "%~f0"&GOTO:EOF
for /f "tokens=1,* delims=]" %%A in ('"type %file%|find /n /v """') do (
    set "line=%%B"
    if defined line (
        call set "line=echo.%%line:%SEARCHTEXT%=%VERSIONNSIS%%%"
        for /f "delims=" %%X in ('"echo."%%line%%""') do %%~X >> %file%_new
    ) ELSE echo. >> %file%_new  
)
move /Y %file%_new %file% > nul
@echo Change %SEARCHTEXT% on %VERSIONNSIS%  
@echo Normal package ... Done.
start "Build Datovka installer" %NSISPATH% %file%

@echo.
@echo ===================================
@echo Build Datovka portable (v%VERSION%) 
@echo ===================================  
qmake.exe datovka.pro -r -spec win32-g++ PORTABLE_APPLICATION=1 
mingw32-make.exe
mingw32-make.exe clean
@echo.
@echo Build done.
@echo ----------------------------------
@echo Creating portable package ...
set DATOVKAPORTPATH=packages\datovka-%VERSION%-portable
mkdir %DATOVKAPORTPATH%
copy "release\datovka-portable.exe" %DATOVKAPORTPATH%
copy "locale\datovka_cs.qm" %DATOVKAPORTPATH%\locale
copy "AUTHORS" %DATOVKAPORTPATH%
copy "COPYING" %DATOVKAPORTPATH%
copy "Changelog" %DATOVKAPORTPATH%
xcopy "dlls\*" %DATOVKAPORTPATH% /E
@echo Portable package ... Done.
@echo.

rmdir /S /Q release
rmdir /S /Q debug

cd scripts