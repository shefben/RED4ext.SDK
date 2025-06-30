@echo off
setlocal
set THIRD_PARTY=%~dp0\..\third_party
if not exist "%THIRD_PARTY%" mkdir "%THIRD_PARTY%"

where curl >nul 2>&1 || (echo curl not found & goto err_clean)
if not "%OS%"=="Windows_NT" (
  echo This script is Windows-only
  goto err_clean
)

set ZSTD_VER=1.5.5
set ZSTD_URL=https://github.com/facebook/zstd/releases/download/v%ZSTD_VER%/zstd-v%ZSTD_VER%-win64.zip
set SODIUM_VER=1.0.19
set SODIUM_URL=https://download.libsodium.org/libsodium/releases/libsodium-%SODIUM_VER%-msvc.zip

if not exist "%THIRD_PARTY%\zstd" mkdir "%THIRD_PARTY%\zstd"
if not exist "%THIRD_PARTY%\libsodium" mkdir "%THIRD_PARTY%\libsodium"

echo Downloading zstd %ZSTD_VER%
curl -L -o zstd.zip %ZSTD_URL%
if errorlevel 1 goto err_clean
powershell -Command "Expand-Archive -Path 'zstd.zip' -DestinationPath 'tmp_zstd' -Force"
if errorlevel 1 goto err_clean
copy /Y tmp_zstd\include\zstd.h "%THIRD_PARTY%\zstd\" >nul
if errorlevel 1 goto err_clean
copy /Y tmp_zstd\lib\zstd.lib "%THIRD_PARTY%\zstd\" >nul
if errorlevel 1 goto err_clean
rmdir /S /Q tmp_zstd
del zstd.zip

echo Downloading libsodium %SODIUM_VER%
curl -L -o sodium.zip %SODIUM_URL%
if errorlevel 1 goto err_clean
powershell -Command "Expand-Archive -Path 'sodium.zip' -DestinationPath 'tmp_sodium' -Force"
if errorlevel 1 goto err_clean
copy /Y tmp_sodium\win64\include\sodium.h "%THIRD_PARTY%\libsodium\" >nul
if errorlevel 1 goto err_clean
copy /Y tmp_sodium\win64\lib\libsodium.lib "%THIRD_PARTY%\libsodium\" >nul
if errorlevel 1 goto err_clean
rmdir /S /Q tmp_sodium
del sodium.zip

echo Done.
endlocal
goto :eof

:err_clean
echo Failed.
if exist zstd.zip del zstd.zip
if exist sodium.zip del sodium.zip
if exist tmp_zstd rmdir /S /Q tmp_zstd
if exist tmp_sodium rmdir /S /Q tmp_sodium
endlocal
exit /b 1

