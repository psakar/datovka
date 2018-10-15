@echo OFF

:: ===========================================================================
:: Datovka msi sign script
:: The script sign Datovka msi package.
:: CZ.NIC, z.s.p.o. 2018
:: ===========================================================================

:: Obtain current version from datovka.pro
cd ..
findstr /C:"VERSION =" pri\version.pri > version.txt
set "string=var1;var2;var3;"
for /f "tokens=1,2,3 delims= " %%i in (version.txt) do set "variable1=%%i" &set "variable2=%%j" &set "VERSION=%%k"
endlocal
del version.txt
cd scripts

setlocal
cd /d %~dp0
set MSIPATH=..\packages\datovka-%VERSION%-windows.msi
echo.
echo "---Sign msi package version %VERSION%---"
signtool sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /a %MSIPATH%
signtool verify /pa /v %MSIPATH%
exit /b