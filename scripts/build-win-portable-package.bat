@echo OFF

:: ===========================================================================
:: Datovka build script
:: The script builds portable Datovka binary and creates an archive (*.zip)
:: containing all dependencies.
:: Copyright: 2014-2018 CZ.NIC, z.s.p.o.
:: ===========================================================================

:: ---------------------------------------------------------------------------
:: How to use it?
:: ---------------------------------------------------------------------------
:: First, install Qt and 7-Zip tools on you computer.
:: Set the location of the Qt tools to the Windows PATH (system or user)
:: environment variable. See
:: https://www.computerhope.com/issues/ch000549.htm for example.
:: Add the location of Qt tools to the PATH variable (e.g.
::     'c:\Qt\5.9.1\mingw53_32\bin\' and 'c:\Qt\Tools\mingw530_32\bin\').
:: Set path to the 7-Zip executables (set the ZIPPATH variable):
set ZIPPATH="C:\Program Files (x86)\7-Zip\7z.exe"
:: You must provide built version of libisds, OpenSSL and other dependencies
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
set DATOVKAPZIP="datovka-portable-%VERSION%-windows.zip"

@echo -------------------------------------------------------------------------
@echo This batch creates portable Datovka package for Windows in a few steps:
@echo 1) Builds datovka-portable.exe executable (requires Qt tools).
@echo 2) Creates application bundle inside the 'datovka-portable.built' directory.
@echo 3) Creates ZIP package inside source root (requires 7-Zip application).
@echo -------------------------------------------------------------------------
@echo WARNING:
@echo You must set the location of the Qt tools to the Windows PATH (system or
@echo user) environment variable - otherwise the script will not run correctly.
@echo E.g. Add to the PATH variable the following entries (replace with actual
@echo location):
@echo   "c:\Qt\5.9.1\mingw53_32\bin\"
@echo   "c:\Qt\Tools\mingw530_32\bin\"
@echo -------------------------------------------------------------------------
@echo Current path to 7-Zip: %ZIPPATH%
@echo NOTE: If path to 7-Zip is wrong then modify this script before running.
@echo -------------------------------------------------------------------------
@echo Portable Datovka version to build: %VERSION%
@echo -------------------------------------------------------------------------
@echo.

if NOT "%1" == "nopause" (
  pause
  if exist packages (
    rmdir /S /Q packages
  )
)

:: Datovka portable version
@echo.
@echo -------------------------------------------------------------
@echo Build portable Datovka binary and CLI binary (v%VERSION%) ...
@echo -------------------------------------------------------------
lupdate datovka.pro
lrelease datovka.pro
qmake.exe CONFIG+=release datovka.pro -r -spec win32-g++ PORTABLE_APPLICATION=1
mingw32-make.exe clean
mingw32-make.exe -j 4
mingw32-make.exe clean
qmake.exe CONFIG+=release datovka-cli.pro.noauto -r -spec win32-g++ PORTABLE_APPLICATION=1
mingw32-make.exe clean
mingw32-make.exe -j 4
mingw32-make.exe clean
@echo Build done.
@echo.
@echo --------------------------------------------------------------
@echo Create application bundle and copy all files and libraries ...
@echo --------------------------------------------------------------
:: Create app package folder.
set DATOVKAPORTPATH="datovka-portable.built"
rmdir /S /Q %DATOVKAPORTPATH%
mkdir %DATOVKAPORTPATH%
mkdir "%DATOVKAPORTPATH%\locale"
:: Copy all required app files and libraries.
copy "release\datovka-portable.exe" %DATOVKAPORTPATH%
copy "release\datovka-cli-portable.exe" %DATOVKAPORTPATH%
copy "AUTHORS" %DATOVKAPORTPATH%
copy "COPYING" %DATOVKAPORTPATH%
copy "ChangeLog" %DATOVKAPORTPATH%
copy "res\qt.conf_windows" "%DATOVKAPORTPATH%\qt.conf"
copy "scripts\datovka-portable-log.bat" %DATOVKAPORTPATH%
::copy "nsis\datovka-install\datovka.ico" %DATOVKAPORTPATH%
copy "locale\datovka_cs.qm" "%DATOVKAPORTPATH%\locale"
copy "locale\datovka_en.qm" "%DATOVKAPORTPATH%\locale"
for /R "libs\shared_built\bin\" %%x in (*.dll) do copy "%%x" %DATOVKAPORTPATH% /Y
copy "libs\libgcc_s_sjlj-1.dll" %DATOVKAPORTPATH%
cd %DATOVKAPORTPATH%
windeployqt --release --libdir "./" --plugindir "plugins/" "datovka-portable.exe"
cd ..
copy "%DATOVKAPORTPATH%\translations\qt_cs.qm" "%DATOVKAPORTPATH%\locale\qtbase_cs.qm"
copy "%DATOVKAPORTPATH%\translations\qt_en.qm" "%DATOVKAPORTPATH%\locale\qtbase_en.qm"
rmdir /S /Q "%DATOVKAPORTPATH%\translations"
@echo Bundle done.
@echo.
@echo --------------------------------------------------------
@echo Run 7-Zip and create ZIP archive of Portable Datovka ...
@echo --------------------------------------------------------
del %DATOVKAPZIP%
set ZIPPACKAGEDIR="datovka-%VERSION%-portable"
xcopy /s /i /q /y /c /d %DATOVKAPORTPATH% %ZIPPACKAGEDIR%
start /wait /Min "Create portable Datovka ZIP archive" %ZIPPATH% a -tzip %DATOVKAPZIP% %ZIPPACKAGEDIR%
rmdir /S /Q %ZIPPACKAGEDIR%
@echo ZIP archive done.
@echo.
rmdir /S /Q release
if NOT "%1" == "nopause" (
  cd .
)
