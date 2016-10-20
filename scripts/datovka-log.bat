
rem Runs Datovka application wit enabled logging.
rem This file must be located in the same directory as the main executable.
rem Log file is generated into the user profile directory if run with ordinary
rem application.
rem Log file is generated into the application directory if run with portable
rem application.

@echo off

SET APP_PORT=datovka-portable.exe
SET APP_NORM=datovka.exe

SET LOG_FILE=datovka.log

SET APP_PATH_BACKSLASH=%~dp0
SET APP_PATH=%APP_PATH_BACKSLASH:~0,-1%

SET HOME_PATH=%userprofile%

if exist "%APP_PATH%\%APP_PORT%" (
	SET APP=%APP_PATH%\%APP_PORT%
	SET LOG=%APP_PATH%\%LOG_FILE%
) else (
	if exist "%APP_PATH%\%APP_NORM%" (
		SET APP=%APP_PATH%\%APP_NORM%
		SET LOG=%HOME_PATH%\%LOG_FILE%
	)
)

if not defined APP (
	echo Cannot find application in %APP_PATH%!
	exit /b 1
) else (
	echo Foud application "%APP%".
	echo Generating log file "%LOG%".
)

rem del "%LOG%"
echo "%APP%" --debug-verbosity 2 --log-verbosity 2 --log-file "%LOG%" > "%LOG%"
"%APP%" --debug-verbosity 2 --log-verbosity 2 --log-file "%LOG%"
