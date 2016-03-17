@echo off
set /p FILENAME=Name of the file:
wavpack.exe %FILENAME% -h
pause