@echo OFF

:: ===========================================================================
:: Datovka MSI installer build script.
:: The script creates installation MSI package (*.msi).
:: CZ.NIC, z.s.p.o. 2017
:: ===========================================================================

:: ---------------------------------------------------------------------------
:: How to use it?
:: ---------------------------------------------------------------------------
:: First, install WiXToolSet (*.exe) from https://github.com/wixtoolset/wix3/releases/tag/wix311rtm
:: Then set path to WiXToolSet binaries to Windows Environment Variables (section PATH).
:: e.g. c:\Program Files (x86)\WiX Toolset v3.11\bin\
:: ---------------------------------------------------------------------------

@echo -------------------------------------------------------------------------
@echo Datovka MSI installer build script.
@echo The script creates installation MSI package (*.msi).
@echo CZ.NIC, z.s.p.o. 2017
@echo -------------------------------------------------------------------------
@echo Note: Edit the script for more detail and settings before run!
@echo -------------------------------------------------------------------------
@pause

:: Obtain current version from datovka.pri
cd ..
findstr /C:"VERSION =" pri\version.pri > version.txt
set "string=var1;var2;var3;"
for /f "tokens=1,2,3 delims= " %%i in (version.txt) do set "variable1=%%i" &set "variable2=%%j" &set "VERSION=%%k"
endlocal
del version.txt
cd msi

:: Define package name, User can change it if needed 
set MSIFILENAME="datovka-%VERSION%-windows"
set PACKAGENAME="%MSIFILENAME%.msi"
:: Do not change this name
set SOURCEDIRNAME="SourceDir"

:: Copy all Datovka sources (files and folders) to script root (SourceDir)
if exist %SOURCEDIRNAME% rd %SOURCEDIRNAME% /s /q
md %SOURCEDIRNAME%
xcopy /s /i /q /y /c /d "./../packages/datovka-%VERSION%" %SOURCEDIRNAME%

:: Run heat.exe to obtain xml files hierarchy
heat dir %SOURCEDIRNAME% -cg FileList -gg -ke -scom -sreg -sfrag -srd -dr INSTALLLOCATION -out "tmpfilelist.wxs"
:: Run WiX compiler
candle tmpfilelist.wxs datovka_installer.wxs
:: Run WiX linker
light datovka_installer.wixobj -out %PACKAGENAME% -v tmpfilelist.wixobj
move /Y %PACKAGENAME% "./../packages/"

:: Clean up
if exist %SOURCEDIRNAME% rd %SOURCEDIRNAME% /s /q
del tmpfilelist.wxs
del tmpfilelist.wixobj
del datovka_installer.wixobj
del %MSIFILENAME%.wixpdb
@pause