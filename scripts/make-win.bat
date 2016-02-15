@echo OFF

:: There are define paths to QT, NSIS and 7-ZIP
:: Qt paths must be set into PATH on Windows envirom.varialbe
:: User have to change it
set QTPATH="C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\" 
set QTMAKEPATH="C:\Qt\Qt5.5.1\Tools\mingw492_32\bin\"
set NSISPATH="C:\Program Files (x86)\NSIS\makensis.exe"
set ZIPPATH="C:\Program Files (x86)\7-Zip\7z.exe"

:: Get current version from datovka.pro
cd ..
findstr /C:"VERSION =" datovka.pro > version.txt
set "string=var1;var2;var3;"
for /f "tokens=1,2,3 delims= " %%i in (version.txt) do set "variable1=%%i" &set "variable2=%%j" &set "VERSION=%%k"
endlocal
del version.txt

::  Define packages names
::  User can change it 
set DATOVKAZIP="datovka-%VERSION%-windows.zip"
set DATOVKAPZIP="datovka-portable-%VERSION%-windows.zip"

@echo --------------------------------------------------------------------
@echo This batch creates Datovka packages for Windows in several steps:
@echo 1) build Datovka (datovka.exe) with QT build tools (requires Qt5.4)
@echo 2) create application packages to "packages" folder
@echo 3) create installation package to "packages" 
@echo    folder from NSIS script (requires NSIS)
@echo 4) create ZIP packages to "packages" folder (requires 7-ZIP)
@echo ---------------------------------------------------------------------
@echo Warning: Script requires dependency libraries in the folder "dlls"!
@echo          This folder and its content must create user - see
@echo          "notes/libdepends.win" for more details about "dlls" content.
@echo          Qt paths must be set in Windows Environment Variables (PATH)!
@echo          NSIS must include plugins: ReplaceInFile.nsh, StrRep.nsh
@echo          {available in: \nsis\nsis-libs\}
@echo ---------------------------------------------------------------------
@echo Windows PATH to Qt:      %QTPATH%
@echo Windows PATH to Mingw32: %QTMAKEPATH%
@echo Current path to NSIS:    %NSISPATH%
@echo Current path to 7-ZIP:   %ZIPPATH%
@echo ---------------------------------------------------------------------
@echo Current Datovka version: %VERSION%
@echo ---------------------------------------------------------------------
@echo.
pause

IF EXIST packages (
  rmdir /S /Q packages
)

@echo. 
@echo =================================
@echo Build Datovka normal (v%VERSION%)  
@echo =================================  
mingw32-make.exe clean
lupdate datovka.pro
lrelease datovka.pro
qmake.exe datovka.pro -r -spec win32-g++
mingw32-make.exe -j 2
mingw32-make.exe clean
@echo Build done.
@echo.
@echo -------------------------------------------------
@echo Creating normal package ...
set DATOVKAPATH=packages\datovka-%VERSION%
mkdir %DATOVKAPATH%
copy "release\datovka.exe" %DATOVKAPATH%
copy "AUTHORS" %DATOVKAPATH%
copy "COPYING" %DATOVKAPATH%
copy "Changelog" %DATOVKAPATH%
xcopy "dlls\*" %DATOVKAPATH% /E
copy "locale\datovka_cs.qm" "%DATOVKAPATH%\locale"
set SEARCHTEXT="VERSIONXXX"
set file="nsis\datovka-install\datovka-install.nsi"
copy nsis\datovka-install\datovka-install.template %file%
set VERSIONNSIS="%VERSION%"
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
@echo Replace %SEARCHTEXT% to %VERSIONNSIS% in the NSIS script  
@echo Run Datovka NSIS script ...
start /wait /Min "Build Datovka installer" %NSISPATH% %file%
@echo Done.
@echo Run 7-ZIP and create ZIP archive of Datovka ...
cd packages
start /wait /Min "Build Datovka ZIP archive" %ZIPPATH% a -tzip %DATOVKAZIP% datovka-%VERSION%
cd ..
del %file%
@echo Datovka packages ... Done.

@echo.
@echo ===================================
@echo Build Datovka portable (v%VERSION%) 
@echo ===================================  
mingw32-make.exe clean
lupdate datovka.pro
lrelease datovka.pro
qmake.exe datovka.pro -r -spec win32-g++ PORTABLE_APPLICATION=1 
mingw32-make.exe -j 2
mingw32-make.exe clean
@echo Build done.
@echo.
@echo -------------------------------------------------
@echo Creating portable package ...
set DATOVKAPORTPATH=packages\datovka-%VERSION%-portable
mkdir %DATOVKAPORTPATH%
copy "release\datovka-portable.exe" %DATOVKAPORTPATH%
copy "AUTHORS" %DATOVKAPORTPATH%
copy "COPYING" %DATOVKAPORTPATH%
copy "Changelog" %DATOVKAPORTPATH%
xcopy "dlls\*" %DATOVKAPORTPATH% /E
copy "locale\datovka_cs.qm" "%DATOVKAPORTPATH%\locale"
@echo Run 7-ZIP and create ZIP archive of portable Datovka ...
cd packages
start /wait /Min "Build portable Datovka ZIP archive" %ZIPPATH% a -tzip %DATOVKAPZIP% datovka-%VERSION%-portable
cd ..
@echo Portable Datovka package ... Done.
@echo.

rmdir /S /Q release
rmdir /S /Q debug
cd scripts