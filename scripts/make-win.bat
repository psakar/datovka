@echo OFF

cd ..

findstr /C:"VERSION =" datovka.pro > version.txt
set "string=var1;var2;var3;"
for /f "tokens=1,2,3 delims= " %%i in (version.txt) do set "variable1=%%i" &set "variable2=%%j" &set "VERSION=%%k"
endlocal

del version.txt

IF EXIST packages (
  rmdir /S /Q packages
)

@echo.
@echo This batch creates Datovka packages in two steps:
@echo 1) build Datovka with QT build tools
@echo 2) create packages to "packages" folder
@echo Current version: %VERSION%
@echo Warning: Build requires dependency libraries in the folder "dlls"!
@echo.
pause


@echo. 
@echo =================================
@echo Build Datovka normal (v%VERSION%)  
@echo =================================  
qmake.exe datovka.pro -r -spec win32-g++
mingw32-make.exe
mingw32-make.exe clean
@echo.
@echo Build done.
@echo ----------------------------------
@echo Creating normal package ...
set DATOVKAPATH=packages\datovka-%VERSION%
mkdir %DATOVKAPATH%
copy "release\datovka.exe" %DATOVKAPATH%
copy "locale\datovka_cs.qm" %DATOVKAPATH%\locale
copy "AUTHORS" %DATOVKAPATH%
copy "COPYING" %DATOVKAPATH%
copy "Changelog" %DATOVKAPATH%
xcopy "dlls\*" %DATOVKAPATH% /E
@echo.
@echo Copy the content of package to NSIS for creation of install package
IF EXIST nsis\app (
  rmdir /S /Q nsis\app
)
xcopy %DATOVKAPATH%\* "nsis\app\" /E
@echo Normal package ... Done.


@echo.
@echo ===================================
@echo Build Datovka portable (v%VERSION%) 
@echo ===================================  
qmake.exe datovka.pro -r -spec win32-g++ PORTABLE_APPLICATION=1 
mingw32-make.exe
mingw32-make.exe clean
@echo.
@echo Build done.
@echo ----------------------------------
@echo Creating portable package ...
set DATOVKAPORTPATH=packages\datovka-%VERSION%-portable
mkdir %DATOVKAPORTPATH%
copy "release\datovka-portable.exe" %DATOVKAPORTPATH%
copy "locale\datovka_cs.qm" %DATOVKAPORTPATH%\locale
copy "AUTHORS" %DATOVKAPORTPATH%
copy "COPYING" %DATOVKAPORTPATH%
copy "Changelog" %DATOVKAPORTPATH%
xcopy "dlls\*" %DATOVKAPORTPATH% /E
@echo Portable package ... Done.
@echo.

rmdir /S /Q release
rmdir /S /Q debug

cd scripts

