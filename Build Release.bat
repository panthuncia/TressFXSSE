@echo off

RMDIR dist /S /Q

cmake -S . --preset=FLATRIM --check-stamp-file "build\CMakeFiles\generate.stamp"
if %ERRORLEVEL% NEQ 0 exit 1
cmake --build build --config Release
if %ERRORLEVEL% NEQ 0 exit 1

xcopy "build\release\*.dll" "D:\MO2 installations\MO2 1.5.97 Dev\mods\TressFXSSE\SKSE\Plugins\" /I /Y
xcopy "build\release\*.pdb" "D:\MO2 installations\MO2 1.5.97 Dev\mods\TressFXSSE\SKSE\Plugins\" /I /Y

xcopy "package" "dist" /I /Y /E

pause