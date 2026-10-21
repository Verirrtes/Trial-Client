@echo off
setlocal
set "ROOT=%~dp0"
set "STAGE=%ROOT%dist\Trial-1.0.0-win64"
set "ZIP=%ROOT%dist\Trial-1.0.0-win64.zip"

if not exist "%ROOT%build\Trial.exe" (
	echo Build not found. Run configure.bat and build_client.bat first.
	exit /b 1
)

if exist "%STAGE%" rmdir /s /q "%STAGE%"
mkdir "%STAGE%"

copy /y "%ROOT%build\Trial.exe" "%STAGE%\" >nul
copy /y "%ROOT%build\*.dll" "%STAGE%\" >nul
copy /y "%ROOT%build\storage.cfg" "%STAGE%\" >nul
copy /y "%ROOT%LICENSE" "%STAGE%\" >nul
copy /y "%ROOT%license.txt" "%STAGE%\" >nul
copy /y "%ROOT%TRIAL.md" "%STAGE%\" >nul
xcopy /e /i /y /q "%ROOT%build\data" "%STAGE%\data" >nul
if exist "%ROOT%data\BestClient" xcopy /e /i /y /q "%ROOT%data\BestClient" "%STAGE%\data\BestClient" >nul

if exist "%ZIP%" del /q "%ZIP%"
powershell -NoProfile -Command "Compress-Archive -Path '%STAGE%\*' -DestinationPath '%ZIP%' -CompressionLevel Optimal"

echo Package: %ZIP%
endlocal
