@echo OFF

:: ===========================================================================
:: Datovka sign script for windows
:: The script sign Datovka binaries and installers.
:: Copyright: 2014-2018 CZ.NIC, z.s.p.o.
:: ===========================================================================

:: ---------------------------------------------------------------------------
:: How to use it?
:: ---------------------------------------------------------------------------
:: 1. Install MSVS 2015 with signtool on you computer.
:: 2. Install 7-Zip tools on you computer and set full install path bellow:
set ZIPPATH="C:\Program Files (x86)\7-Zip\7z.exe"
:: 3. Install SafeNet Authentication Client and restart computer.
::    see https://support.comodo.com/index.php?/Knowledgebase/Article/View/1211/106/safenet-download-for-ev-codesigning-certificates
:: 4. Connect your sign token to USB and run SafeNet Authentication Client software.
:: 5. Run "VS2015 x86 Native Tools Command Prompt" from start menu.
:: 6. In prompt navigate to script location and run the script with files to be signed.
:: 7. Enter usb token password when will need.
:: NOTE: Active connection to internet is required (for timestamp of signature)
:: ---------------------------------------------------------------------------

:: Not change these variables
set SIGN_CERT_ID="CZ.NIC, z.s.p.o."
set APPNAME="Datovka"
set SIGNDIR="signdir"
set SIGN_PREFIX=signed_

setlocal
cd /d %~dp0

:: Delete sign work directory if exists
if exist %SIGNDIR% (
	rmdir /S /Q %SIGNDIR%
)

:: Test if any file was set
set cmdArgsNumber=0
for %%x in (%*) do Set /A cmdArgsNumber+=1
if %cmdArgsNumber%==0 call :Usage %0

:: Run sign procedure over all files
for %%F IN (%*) DO (
	call :DoSignAction %%F
)
exit /b

:: Show usage information
:Usage
	echo.
	echo Usage: %1 file1 [file2 ...]
	echo Script for signing %APPNAME% software packages.
	echo It fill create signed counterparts in same location as the original file.
	echo (E.g. for '/tmp/datovka-4.11.0-windows.zip' it will create
	echo           '/tmp/%SIGN_PREFIX%datovka-4.11.0-windows.zip'.)
	echo It accepts these file extensions:
	echo *.zip - archive containing Windows Datovka package
	echo *.exe - NSIS installer for Windows
	echo *.msi - MSI installer for Windows
goto :eof

:: Detect file extension and run sign procedure
:DoSignAction
	setlocal
	set FILEEXT=%~x1
	if "%FILEEXT%"==".exe" (
		call :SignInstaller "%1" exe
	) else if "%FILEEXT%"==".zip" (
		call :SignZip "%1"
	) else if "%FILEEXT%"==".msi" (
		call :SignInstaller "%1" msi
	) else (
		echo.
		echo Skip unsuported file extension "%FILEEXT%!
	) 
goto :eof

:: Sign msi|exe installer
:SignInstaller
	setlocal
	set FILEPATH=%~p1
	set FILENAME=%~nx1
	mkdir %SIGNDIR%
	copy /Y %1 %SIGNDIR%
	echo.
	echo Sign %2 installer: "%1"
	for /r %SIGNDIR% %%B in (*.*) DO call :AppendFile "%%~fB"
	signtool sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /n %SIGN_CERT_ID% %signFiles%
	if %ERRORLEVEL% EQU 1 goto IsError
	move /Y %signFiles% %FILEPATH%%SIGN_PREFIX%%FILENAME%
	rmdir /Q /S %SIGNDIR%
	echo Done.
	call :VerifySignature %FILEPATH%%SIGN_PREFIX%%FILENAME%
goto :eof

:: Sign all dll|exe from datovka package
:SignZip
	setlocal
	set FILENAME=%~nx1
	set FILEPATH=%~p1
	echo.
	echo Sign zip package: "%1"
	echo Unzip package: "%1"
	start /wait /Min "UNZIP Datovka archive" %ZIPPATH% x %1 -o%SIGNDIR%
	echo Done
	echo.
	echo Sign dll and exe files:
	for /r %SIGNDIR% %%B in (*.dll) DO call :AppendFile "%%~fB"
	for /r %SIGNDIR% %%B in (*.exe) DO call :AppendFile "%%~fB"
	signtool sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /n %SIGN_CERT_ID% %signFiles%
	if %ERRORLEVEL% EQU 1 goto IsError
	for /r %SIGNDIR% %%B in (*.exe) DO call :VerifySignature "%%~fB"
	echo.
	echo Create signed zip packeage: "%FILEPATH%%SIGN_PREFIX%%FILENAME%"
	cd %SIGNDIR%
	start /wait /Min "Create Datovka ZIP archive" %ZIPPATH% a -tzip %FILEPATH%%SIGN_PREFIX%%FILENAME% .
	cd ..
	rmdir /Q /S %SIGNDIR%
	echo Done.
goto :eof

:: Create list of all binary files to be signed
:AppendFile
	set signFiles=%signFiles% %1
goto :eof

:: Verify signature
:VerifySignature
	echo.
	echo Verify signature: "%1"
	signtool verify /pa /v %1
	echo Signature has been verified.
goto :eof

:: Error during sign
:IsError
	echo.
	echo ERROR: Packages have not been signed!
	rmdir /Q /S %SIGNDIR%
	exit /b
goto :eof


