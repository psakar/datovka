@echo OFF

:: ===========================================================================
:: Datovka build script
:: The script builds Datovka binary and creates an installation package (*.exe)
:: and an archive (*.zip) containing all dependencies.
:: Copyright: 2014-2018 CZ.NIC, z.s.p.o.
:: ===========================================================================

:: ---------------------------------------------------------------------------
:: How to use it?
:: ---------------------------------------------------------------------------
:: First, install Qt, NSIS and 7-Zip tools on you computer.
:: NSIS must include plugins: ReplaceInFile.nsh, StrRep.nsh in \nsis\nsis-libs\
:: Set the location of the Qt tools to the Windows PATH (system or user)
:: environment variable. See
:: https://www.computerhope.com/issues/ch000549.htm for example.
:: Add the location of Qt tools to the PATH variable (e.g.
::     'c:\Qt\5.9.1\mingw53_32\bin\' and 'c:\Qt\Tools\mingw530_32\bin\').
:: Set your path to NSIS and 7ZIP executables into variables below:
:: Set path to the NSIS and 7-Zip executables (set the NSISPATH and ZIPPATH
:: variables):
set NSISPATH="C:\Program Files (x86)\NSIS\makensis.exe"
set ZIPPATH="C:\Program Files (x86)\7-Zip\7z.exe"
:: You must provide built versions of libisds, OpenSSL and other dependencies
:: in the 'libs\' directory.
:: Then you can run this script.
:: ---------------------------------------------------------------------------

cd ..

:: Obtain current version from datovka.pro
findstr /C:"VERSION =" pri\version.pri > version.txt
set "string=var1;var2;var3;"
for /f "tokens=1,2,3 delims= " %%i in (version.txt) do set "variable1=%%i" &set "variable2=%%j" &set "VERSION=%%k"
endlocal
del version.txt

:: Define the package name. You can change it if needed.
set DATOVKAMSIEXE="datovka-%VERSION%-windows.exe"
set DATOVKAZIP="datovka-%VERSION%-windows.zip"

@echo -------------------------------------------------------------------------
@echo This batch creates Datovka package for Windows in a few steps:
@echo 1) Builds datovka.exe executable (requires Qt tools).
@echo 2) Creates application bundle inside the 'datovka.built' directory.
@echo 3) Creates installator (*.exe) inside source root (requires NSIS tool).
@echo 4) Creates ZIP package inside source root (requires 7-Zip application).
@echo -------------------------------------------------------------------------
@echo WARNING:
@echo You must set the location of the Qt tools to the Windows PATH (system or
@echo user) environment variable - otherwise the script will not run correctly.
@echo E.g. Add to the PATH variable the following entries (replace with actual
@echo location):
@echo   "c:\Qt\5.9.1\mingw53_32\bin\"
@echo   "c:\Qt\Tools\mingw530_32\bin\"
@echo -------------------------------------------------------------------------
@echo Current path to NSIS:  %NSISPATH%
@echo Current path to 7-Zip: %ZIPPATH%
@echo NOTE: If path to NSIS or 7-Zip is wrong then modify this script before
@echo running.
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
lupdate datovka.pro
lrelease datovka.pro
qmake.exe CONFIG+=release datovka.pro -r -spec win32-g++
mingw32-make.exe clean
mingw32-make.exe -j 4
mingw32-make.exe clean
qmake.exe CONFIG+=release datovka-cli.pro.noauto -r -spec win32-g++
mingw32-make.exe clean
mingw32-make.exe -j 4
mingw32-make.exe clean
@echo Build done.
@echo.
@echo --------------------------------------------------------------
@echo Create application bundle and copy all files and libraries ...
@echo --------------------------------------------------------------
:: Create app package folder.
set DATOVKAPATH="datovka.built"
rmdir /S /Q %DATOVKAPATH%
mkdir %DATOVKAPATH%
mkdir "%DATOVKAPATH%\locale"
:: Copy all required app files and libraries.
copy "release\datovka.exe" %DATOVKAPATH%
copy "release\datovka-cli.exe" %DATOVKAPATH%
copy "AUTHORS" %DATOVKAPATH%
copy "COPYING" %DATOVKAPATH%
copy "ChangeLog" %DATOVKAPATH%
copy "res\qt.conf_windows" "%DATOVKAPATH%\qt.conf"
copy "scripts\datovka-log.bat" %DATOVKAPATH%
::copy "nsis\datovka-install\datovka.ico" %DATOVKAPATH%
copy "locale\datovka_cs.qm" "%DATOVKAPATH%\locale"
copy "locale\datovka_en.qm" "%DATOVKAPATH%\locale"
for /R "mingw32built\bin\" %%x in (*.dll) do copy "%%x" %DATOVKAPATH% /Y
copy "libs\libgcc_s_sjlj-1.dll" %DATOVKAPATH%
cd %DATOVKAPATH%
windeployqt --release --libdir "./" --plugindir "plugins/" "datovka.exe"
cd ..
copy "%DATOVKAPATH%\translations\qt_cs.qm" "%DATOVKAPATH%\locale\qtbase_cs.qm"
copy "%DATOVKAPATH%\translations\qt_en.qm" "%DATOVKAPATH%\locale\qtbase_en.qm"
rmdir /S /Q "%DATOVKAPATH%\translations"
@echo Bundle done.
@echo.
@echo --------------------------------------------------
@echo Create executable installation package (*.exe) ...
@echo --------------------------------------------------
del %DATOVKAMSIEXE%
:: Replace version string in the NSIS script.
set SEARCHTEXT="VERSIONXXX"
set file="win\nsis\datovka-install\datovka-install.nsi"
copy win\nsis\datovka-install\datovka-install.template %file%
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
@echo Run Datovka NSIS script and create Datovka installer package (*.exe) ...
start /wait /Min "Build Datovka installer" %NSISPATH% %file%
del %file%
@echo Installer package done.
@echo.
@echo -----------------------------------------------
@echo Run 7-Zip and create ZIP archive of Datovka ...
@echo -----------------------------------------------
del %DATOVKAZIP%
set ZIPPACKAGEDIR="datovka-%VERSION%"
xcopy /s /i /q /y /c /d %DATOVKAPATH% %ZIPPACKAGEDIR%
start /wait /Min "Create Datovka ZIP archive" %ZIPPATH% a -tzip %DATOVKAZIP% %ZIPPACKAGEDIR%
rmdir /S /Q %ZIPPACKAGEDIR%
@echo ZIP archive done.
@echo.
rmdir /S /Q release
if NOT "%1" == "nopause" (
  cd .
)
