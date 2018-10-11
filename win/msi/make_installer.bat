@echo OFF

:: ===========================================================================
:: Datovka MSI installer build script.
:: The script creates MSI installer (*.msi) of Datovka.
:: CZ.NIC, z.s.p.o. 2017
:: Note: you can change settings into script
:: ===========================================================================
:: How to use it? | requirements
:: ---------------------------------------------------------------------------
:: First, install WiXToolSet (*.exe) from https://github.com/wixtoolset/wix3/releases/tag/wix311rtm
:: Then set path to WiXToolSet binaries to Windows Environment Variables (section PATH).
:: e.g. c:\Program Files (x86)\WiX Toolset v3.11\bin\
:: ---------------------------------------------------------------------------

:: Obtain current version from datovka.pri
cd ..
findstr /C:"VERSION =" pri\version.pri > version.txt
set "string=var1;var2;var3;"
for /f "tokens=1,2,3 delims= " %%i in (version.txt) do set "variable1=%%i" &set "variable2=%%j" &set "VERSION=%%k"
endlocal
del version.txt
cd msi

:: Define package name, User can change it if needed 
set MSIFILENAME=datovka-%VERSION%-windows
set PACKAGENAME="%MSIFILENAME%.msi"
:: Do not change this name
set SOURCEDIRNAME="SourceDir"
set TMPFILELISTNAME="tmpfilelist"
set SCRIPTNAME="datovka_installer"

@echo ===================Datovka MSI installer build script===================
@echo : The script creates MSI installer (*.msi) of Datovka.
@echo : CZ.NIC, z.s.p.o. 2017
@echo : Note: you can change settings into script before run...
@echo +-----------------------------------------------------------------------
@echo : Datovka version: "%VERSION%"
@echo : MSI file name: %PACKAGENAME%
@echo ========================================================================
@echo.
@pause
@echo.

:: Copy all Datovka sources (files and folders) to script root (SourceDir)
if exist %SOURCEDIRNAME% rd %SOURCEDIRNAME% /s /q
md %SOURCEDIRNAME%
xcopy /s /i /q /y /c /d "./../packages/datovka-%VERSION%" %SOURCEDIRNAME%

:: Run heat.exe to obtain xml files hierarchy
heat dir %SOURCEDIRNAME% -cg FileList -gg -ke -scom -sreg -sfrag -srd -dr FileInstallPath -out %TMPFILELISTNAME%.wxs
:: Run WiX compiler
candle %TMPFILELISTNAME%.wxs %SCRIPTNAME%.wxs -dProductVersion=%VERSION%
:: Run WiX linker
light %SCRIPTNAME%.wixobj -ext WixUIExtension -out %PACKAGENAME% -v %TMPFILELISTNAME%.wixobj -cultures:cs-cz
move /Y %PACKAGENAME% "./../packages/"

:: Clean up
if exist %SOURCEDIRNAME% rd %SOURCEDIRNAME% /s /q
del %TMPFILELISTNAME%.wxs
del %TMPFILELISTNAME%.wixobj
del %SCRIPTNAME%.wixobj
del %MSIFILENAME%.wixpdb
@echo.
REM @pause
