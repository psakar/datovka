@echo OFF

:: ===========================================================================
:: Datovka sign script
:: The script sign Datovka binary.
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
:: Unzip application packages
REM Call :UnZipFile "..\packages\" "..\packages\datovka-%VERSION%-windows.zip"
REM Call :UnZipFile "..\packages\" "..\packages\datovka-portable-%VERSION%-windows.zip"

set DATOVKAPATH=..\packages\datovka-%VERSION%
set DATOVKAPORTPATH=..\packages\datovka-%VERSION%-portable

echo.
echo "---Sign install and portable version %VERSION%---"
for /r %DATOVKAPATH% %%B in (*.dll) DO call :AppendFile "%%~fB"
for /r %DATOVKAPATH% %%B in (*.exe) DO call :AppendFile "%%~fB"
for /r %DATOVKAPORTPATH% %%B in (*.dll) DO call :AppendFile "%%~fB"
for /r %DATOVKAPORTPATH% %%B in (*.exe) DO call :AppendFile "%%~fB"
signtool sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /a %signFiles%
exit /b

:UnZipFile <ExtractTo> <newzipfile>
set vbs="%temp%\_.vbs"
if exist %vbs% del /f /q %vbs%
>%vbs%  echo Set fso = CreateObject("Scripting.FileSystemObject")
>>%vbs% echo If NOT fso.FolderExists(%1) Then
>>%vbs% echo fso.CreateFolder(%1)
>>%vbs% echo End If
>>%vbs% echo set objShell = CreateObject("Shell.Application")
>>%vbs% echo set FilesInZip=objShell.NameSpace(%2).items
>>%vbs% echo objShell.NameSpace(%1).CopyHere(FilesInZip)
>>%vbs% echo Set fso = Nothing
>>%vbs% echo Set objShell = Nothing
cscript //nologo %vbs%
if exist %vbs% del /f /q %vbs%

:AppendFile
set signFiles=%signFiles% %1
goto :eof