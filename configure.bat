@echo off
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSPATH="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if defined VSPATH goto havevs
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" set "VSPATH=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools"
if defined VSPATH goto havevs
echo Could not locate Visual Studio with C++ tools.
exit /b 1
:havevs
echo Using Visual Studio: "%VSPATH%"
call "%VSPATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -no_logo
if errorlevel 1 exit /b 1
cmake -S "%~dp0." -B "%~dp0build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCLIENT=ON -DSERVER=OFF -DTOOLS=OFF -DSTEAM=OFF -DVULKAN=OFF -DVIDEORECORDER=OFF -DDOWNLOAD_GTEST=OFF -DDEV=OFF -DCLIENT_EXECUTABLE=Trial -DAUTOUPDATE=OFF -DINFORM_UPDATE=OFF
exit /b %errorlevel%
