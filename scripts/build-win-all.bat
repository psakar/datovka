@echo OFF

:: ===========================================================================
:: Datovka build script
:: The script builds Datovka binary and creates an installation package (*.exe)
:: and an archive (*.zip) containing all dependencies.
:: Copyright: 2014-2018 CZ.NIC, z.s.p.o.
:: ===========================================================================

@echo ------------------------------------------------------------------------
@echo This batch creates Datovka installation packages (*.exe) and a portable
@echo Datovka package (*.zip) for Windows.
@echo NOTE: For more information see inside 'build-win-install-package.bat',
@echo 'build-win-portable-package.bat' and set required paths and variables.
@echo ------------------------------------------------------------------------
@echo.
pause
cd ..
if exist packages (
  rmdir /S /Q packages
)
cd scripts
:: Run Datovka installation build script with parameter "nopause"
call build-win-install-package.bat nopause
cd scripts
:: Run Portable Datovka build script with parameter "nopause"
call build-win-portable-package.bat nopause
cd scripts
