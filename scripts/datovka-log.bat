@echo off

rem Runs Datovka application wit enabled logging.
rem This file must be located in the same directory as the main executable.
rem Log file is generated into the user profile directory if run with ordinary application.

SET APP_EXE=datovka.exe
SET LOG_FILE=datovka.log
SET APP_PATH_BACKSLASH=%~dp0
SET APP_PATH=%APP_PATH_BACKSLASH:~0,-1%
SET HOME_PATH=%userprofile%
SET APP=%APP_PATH%\%APP_EXE%
SET LOG=%HOME_PATH%\%LOG_FILE%

@echo on
"%APP%" --debug-verbosity 2 --log-verbosity 2 --log-file "%LOG%"