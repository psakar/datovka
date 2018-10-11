@echo OFF

:: ===========================================================================
:: Datovka MSI installer building script.
:: The script creates the MSI installer package for Datovka.
:: Copyright: 2014-2018 CZ.NIC, z.s.p.o.
:: Note: You are free to change the settings inside this script.
:: ===========================================================================
:: How to use it? | requirements
:: ---------------------------------------------------------------------------
:: First, install WiX Toolset 3 from https://github.com/wixtoolset/wix3/releases/tag/wix311rtm
:: Then set PATH to the directory containing the WiX binaries (e.g.
:: c:\Program Files (x86)\WiX Toolset v3.11\bin\).
:: ---------------------------------------------------------------------------

:: Obtain current version from datovka.pri
cd ..\..
findstr /C:"VERSION =" pri\version.pri > version.txt
set "string=var1;var2;var3;"
for /f "tokens=1,2,3 delims= " %%i in (version.txt) do set "variable1=%%i" &set "variable2=%%j" &set "VERSION=%%k"
endlocal
del version.txt
cd win\msi

:: Define package name, User can change it if needed 
set MSIFILENAME=datovka-%VERSION%-windows
set PACKAGENAME="%MSIFILENAME%.msi"
:: Do not change this name
set SOURCEDIRNAME="SourceDir"
set TMPFILELISTNAME="tmpfilelist"
set SCRIPTNAME="datovka_installer"

@echo ===================Datovka MSI installer build script===================
@echo : The script creates the MSI installer package for Datovka.
@echo : Copyright: 2014-2018 CZ.NIC, z.s.p.o.
@echo : Note: You are free to change the settings inside this script.
@echo +-----------------------------------------------------------------------
@echo : Datovka version: "%VERSION%"
@echo : MSI file name: %PACKAGENAME%
@echo ========================================================================
@echo.
@pause
@echo.

:: Copy all Datovka binaries and other required files and folders to script root (SourceDir)
if exist %SOURCEDIRNAME% rd %SOURCEDIRNAME% /s /q
md %SOURCEDIRNAME%
xcopy /s /i /q /y /c /d "./../../datovka.built" %SOURCEDIRNAME%
copy ".\..\nsis\datovka-install\datovka.ico" ".\"

:: Run heat.exe to obtain xml files hierarchy
heat dir %SOURCEDIRNAME% -cg FileList -gg -ke -scom -sreg -sfrag -srd -dr FileInstallPath -out %TMPFILELISTNAME%.wxs
:: Run WiX compiler
candle %TMPFILELISTNAME%.wxs %SCRIPTNAME%.wxs -dProductVersion=%VERSION%
:: Run WiX linker
light %SCRIPTNAME%.wixobj -ext WixUIExtension -out %PACKAGENAME% -v %TMPFILELISTNAME%.wixobj -cultures:cs-cz
move /Y %PACKAGENAME% "./../../"

:: Clean up
if exist %SOURCEDIRNAME% rd %SOURCEDIRNAME% /s /q
del %TMPFILELISTNAME%.wxs
del %TMPFILELISTNAME%.wixobj
del %SCRIPTNAME%.wixobj
del %MSIFILENAME%.wixpdb
del datovka.ico
@echo.
REM @pause
