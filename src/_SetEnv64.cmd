@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
set sdkenv="C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x64
rem call %sdkenv% /debug
call %sdkenv% /release
set xmsbuild=msbuild /p:TargetFrameworkMoniker=".NETFramework,Version=v3.5" /p:Platform=x64
rem set xmsbuild=msbuild /p:Platform=x64
echo __________________________________________
echo %%xmsbuild%% /t:Clean "T-Clock 2010.sln" /p:Configuration=Release
echo %%xmsbuild%% /t:Clean "T-Clock 2010.sln" /p:Configuration=Debug
echo.
echo %%xmsbuild%% /m "T-Clock 2010.sln" /p:Configuration=Release
echo.
echo %%xmsbuild%% /m "T-Clock 2010.sln" /p:Configuration=Debug
echo.
echo __________________________________________
cmd /K