
REM Ensure that path to Qt (C:\Qt\5.9.1\mingw53_32\bin) is set.
REM Ensure that path to MinGW tools bundled with Qt (C:\Qt\Tools\mingw530_32\bin) is set.

SET QMAKE="qmake.exe"
SET MINGW_PATH="C:\Qt\Tools\mingw530_32"
SET MAKE="mingw32-make.exe"

"%QMAKE%" CONFIG+=debug rm_test_app.pro
"%MAKE%" -j 4

mkdir rm_test_app
copy debug\rm_test_app.exe rm_test_app
copy "%MINGW_PATH%\opt\bin\libeay32.dll" rm_test_app
copy "%MINGW_PATH%\opt\bin\ssleay32.dll" rm_test_app
copy rm_test_app.bat rm_test_app
