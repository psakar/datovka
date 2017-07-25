
REM Ensure that path to Qt (C:\Qt\5.9.1\mingw53_32\bin) is set.
REM Ensure that path to MinGW tools bundled with Qt (C:\Qt\Tools\mingw530_32\bin) is set.

SET APP_DIR="rm_test_app"

SET QMAKE="qmake.exe"
SET MINGW_PATH="C:\Qt\Tools\mingw530_32"
SET MAKE="mingw32-make.exe"
SET DEPLOYQT="windeployqt.exe"

SET CONF="release"

"%QMAKE%" CONFIG+=%CONF% rm_test_app.pro
"%MAKE%" -j 4

mkdir "%APP_DIR%"
copy "%CONF%\rm_test_app.exe" "%APP_DIR%"
copy "%MINGW_PATH%\opt\bin\libeay32.dll" "%APP_DIR%"
copy "%MINGW_PATH%\opt\bin\ssleay32.dll" "%APP_DIR%"
copy rm_test_app.bat "%APP_DIR%"
"%DEPLOYQT%" --dir "%APP_DIR%" --%CONF% "%APP_DIR%\rm_test_app.exe"
