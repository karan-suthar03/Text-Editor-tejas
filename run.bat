
@echo off
setlocal

cd /D "%~dp0"

call "bin\editor.exe"

popd
endlocal
