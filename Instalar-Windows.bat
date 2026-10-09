
@echo off
setlocal
title Instalador de BL-Ceti

echo ==========================================
echo         BL-Ceti - Instalador Windows
echo ==========================================
echo.

REM Ir a la carpeta donde esta este archivo
cd /d "%~dp0"

REM Comprobar que existe el compilador de C
where gcc >nul 2>&1
if errorlevel 1 (
    echo ERROR: No se encontro GCC.
    echo Instala GCC y CMake antes de continuar.
    pause
    exit /b 1
)

REM Comprobar CMake
where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: No se encontro CMake.
    pause
    exit /b 1
)

REM Configurar el proyecto
echo [1/4] Configurando BL-Ceti...
cmake -S . -B build -G "MinGW Makefiles"
if errorlevel 1 goto error

REM Compilar
echo.
echo [2/4] Compilando BL-Ceti...
cmake --build build
if errorlevel 1 goto error

REM Verificar el ejecutable
if not exist "build\BL.exe" (
    echo ERROR: No se genero build\BL.exe.
    pause
    exit /b 1
)

REM Registrar la asociacion para el usuario actual
echo.
echo [3/4] Registrando la extension .bl...

reg add "HKCU\Software\Classes\.bl" /ve /d "BLCeti.Source" /f
if errorlevel 1 goto error

reg add "HKCU\Software\Classes\BLCeti.Source" /ve /d "Archivo fuente BL-Ceti" /f
if errorlevel 1 goto error

REM Configurar icono generico y descripcion
reg add "HKCU\Software\Classes\BLCeti.Source\DefaultIcon" /ve /d "%SystemRoot%\System32\imageres.dll,14" /f

REM Configurar apertura mediante el compilador
reg add "HKCU\Software\Classes\BLCeti.Source\shell\open\command" /ve /d "\"%CD%\build\BL.exe\" \"%%1\"" /f
if errorlevel 1 goto error

echo.
echo [4/4] Actualizando la asociacion de archivos...
assoc .bl=BLCeti.Source

echo.
echo ==========================================
echo     BL-Ceti se instalo correctamente.
echo ==========================================
echo.
echo Los archivos .bl estan asociados a BL-Ceti.
echo.
echo IMPORTANTE:
echo Esta version del programa solo analiza tokens.
echo Todavia no ejecuta las instrucciones del archivo.
echo.
pause
exit /b 0

:error
echo.
echo ERROR: La instalacion no pudo completarse.
pause
exit /b 1
