@echo off

rem Runs portable Datovka application wit enabled logging.
rem This file must be located in the same directory as the main executable.
rem Log file is generated into the application directory if run with portable application.

SET APP_EXE=datovka-portable.exe
SET LOG_FILE=datovka-portable.log
SET APP_PATH_BACKSLASH=%~dp0
SET APP_PATH=%APP_PATH_BACKSLASH:~0,-1%
SET APP=%APP_PATH%\%APP_EXE%
SET LOG=%APP_PATH%\%LOG_FILE%

@echo on
"%APP%" --debug-verbosity 2 --log-verbosity 2 --log-file "%LOG%"