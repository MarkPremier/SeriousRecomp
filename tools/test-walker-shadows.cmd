@echo off
setlocal
set "RECOMP_WALKER_SHADOW_MIN=1"
set "SAM_DIR=%~dp0.."
if not exist "%SAM_DIR%\serious_sam_recomp.exe" set "SAM_DIR=%~dp0..\build\Release"
start "" /D "%SAM_DIR%" "%SAM_DIR%\serious_sam_recomp.exe"
endlocal
