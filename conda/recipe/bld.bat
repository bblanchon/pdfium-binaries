@echo ON

rem Copy over the include and lib dirs
robocopy %SRC_DIR%\include\ %LIBRARY_INC%\ *.* /E
if %ERRORLEVEL% GEQ 8 exit 1

robocopy %SRC_DIR%\lib\ %LIBRARY_LIB%\ *.* /E
robocopy %SRC_DIR%\bin\ %LIBRARY_BIN%\ *.* /E
if %ERRORLEVEL% GEQ 8 exit 1

exit 0
