@echo off
setlocal
set "RECOMP_WALKER_SHADOW_MIN=1"
start "" /D "%~dp0..\build\Release" "%~dp0..\build\Release\serious_sam_recomp.exe"
endlocal
