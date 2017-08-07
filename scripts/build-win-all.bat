@echo OFF

:: ===========================================================================
:: Datovka build script
:: The script builds Datovka binary and creates installation package (*.exe)
:: and archive (*.zip) with all dependencies.
:: CZ.NIC, z.s.p.o. 2017
:: ===========================================================================

@echo ------------------------------------------------------------------------
@echo This batch creates Datovka installation packages (*.exe) for Windows
@echo and Portable Datovka package (*.zip) for Windows.
@echo NOTE: For more info see build-win-install-package.bat script
@echo and build-win-portable-package.bat and set required paths and variables. 
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