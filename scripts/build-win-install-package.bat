@echo OFF

:: ===========================================================================
:: Datovka build script
:: The script builds Datovka binary and creates installation package (*.exe)
:: and archive (*.zip) with all dependencies.
:: CZ.NIC, z.s.p.o. 2017
:: ===========================================================================

:: ---------------------------------------------------------------------------
:: How to use it?
:: ---------------------------------------------------------------------------
:: First, install Qt, NSIS and 7-ZIP tools on you computer.
:: NSIS must include plugins: ReplaceInFile.nsh, StrRep.nsh in \nsis\nsis-libs\
:: Set path to Qt, Qt compiler executables to Windows Environment Variables (section PATH).
:: See https://www.computerhope.com/issues/ch000549.htm for exmaple.
:: Set to PATH following paths (5.9.1 replace for your version of Qt): 
::      "C:\Qt\5.9.1\mingw53_32\bin\" and "C:\Qt\Tools\mingw530_32\bin\" 
:: Set your path to NSIS and 7ZIP executables into variables below:
set NSISPATH="C:\Program Files (x86)\NSIS\makensis.exe"
set ZIPPATH="C:\Program Files (x86)\7-Zip\7z.exe"
:: You must have built libisds, openssl and other dependencies in the folder "mingw32built"   
:: Then you can run this script.
:: ---------------------------------------------------------------------------

:: Obtain current version from datovka.pro
cd ..
findstr /C:"VERSION =" pri\version.pri > version.txt
set "string=var1;var2;var3;"
for /f "tokens=1,2,3 delims= " %%i in (version.txt) do set "variable1=%%i" &set "variable2=%%j" &set "VERSION=%%k"
endlocal
del version.txt

::  Define package name, User can change it if needed 
set DATOVKAZIP="datovka-%VERSION%-windows.zip"

@echo -------------------------------------------------------------------------
@echo This batch creates Datovka installation package for Windows in several steps:
@echo 1) Build Datovka binary (datovka.exe) with QT tool (requires Qt)
@echo 2) Create application bundle with dependencies to "packages" folder
@echo 3) Create installation package (*.exe) to "packages" folder (requires NSIS tool)
@echo 4) Create ZIP package to "packages" folder (requires 7-ZIP tool)
@echo -------------------------------------------------------------------------
@echo WARNING:
@echo You must set path to Qt, Qt compiler executables to Windows Environment
@echo Variables (section PATH) otherwise the script will not run correctly.
@echo Add to Windows PATH following paths (replace 5.9.1 for your Qt version): 
@echo   "C:\Qt\5.9.1\mingw53_32\bin\"
@echo   "C:\Qt\Tools\mingw530_32\bin\"
@echo -------------------------------------------------------------------------
@echo Current path to NSIS:  %NSISPATH%
@echo Current path to 7-ZIP: %ZIPPATH% 
@echo NOTE: If paths to NSIS and 7-ZIP are wrong, you must set correct paths
@echo       in the script before running.
@echo -------------------------------------------------------------------------
@echo Datovka version to build: %VERSION%
@echo -------------------------------------------------------------------------
@echo.

if NOT "%1" == "nopause" (
  pause
  if exist packages (
    rmdir /S /Q packages
  )
)

:: Datovka installation version
@echo.
@echo ----------------------------------------------------
@echo Build Datovka binary and CLI binary (v%VERSION%) ...
@echo ----------------------------------------------------
mingw32-make.exe clean
lupdate datovka.pro
lrelease datovka.pro
qmake.exe datovka.pro -r -spec win32-g++
mingw32-make.exe -j 4
mingw32-make.exe clean
lupdate datovka-cli.pro.noauto
lrelease datovka-cli.pro.noauto
qmake.exe datovka-cli.pro.noauto -r -spec win32-g++
mingw32-make.exe -j 4
mingw32-make.exe clean
@echo Build done.
@echo.
@echo --------------------------------------------------------------
@echo Create application bundle and copy all files and libraries ...
@echo --------------------------------------------------------------
:: Create app packege folder
set DATOVKAPATH=packages\datovka-%VERSION%
mkdir %DATOVKAPATH%
mkdir "%DATOVKAPATH%\locale"
:: Copy all required app files and libraries
copy "release\datovka.exe" %DATOVKAPATH%
copy "release\datovka-cli.exe" %DATOVKAPATH%
copy "AUTHORS" %DATOVKAPATH%
copy "copyING" %DATOVKAPATH%
copy "Changelog" %DATOVKAPATH%
copy "scripts\datovka-log.bat" %DATOVKAPATH%
copy "locale\datovka_cs.qm" "%DATOVKAPATH%\locale"
for /R "mingw32built\bin\" %%x in (*.dll) do copy "%%x" %DATOVKAPATH% /Y
windeployqt --release "%DATOVKAPATH%\datovka.exe"
copy "%DATOVKAPATH%\translations\qt_cs.qm" "%DATOVKAPATH%\locale\qtbase_cs.qm"
rmdir /S /Q "%DATOVKAPATH%\translations"
@echo Bundle done.
@echo.
@echo --------------------------------------------------
@echo Create executable installation package (*.exe) ...
@echo --------------------------------------------------
:: Replace version string in the NSIS script  
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
    ) else echo. >> %file%_new  
)
move /Y %file%_new %file% > nul
@echo Replace %SEARCHTEXT% to %VERSIONNSIS% in the NSIS script  
@echo Run Datovka NSIS script and create Datovka install package (*.exe) ...
start /wait /Min "Build Datovka installer" %NSISPATH% %file%
del %file%
@echo Install package done.
@echo.
@echo -----------------------------------------------
@echo Run 7-ZIP and create ZIP archive of Datovka ...
@echo -----------------------------------------------
cd packages
start /wait /Min "Create Datovka ZIP archive" %ZIPPATH% a -tzip %DATOVKAZIP% datovka-%VERSION%
cd ..
@echo ZIP archive done.
rmdir /S /Q release
rmdir /S /Q debug
if NOT "%1" == "nopause" (
  cd packages
)