@echo off
set "GIT_DIR=%CD%\PortableGit"
if exist "%GIT_DIR%\cmd\git.exe" (
    set "PATH=%GIT_DIR%\cmd;%GIT_DIR%\bin;%GIT_DIR%\usr\bin;%PATH%"
    echo Using portable Git from %GIT_DIR%
) else (
    echo WARNING: PortableGit not found in %GIT_DIR%. Make sure git is available in PATH!
)
set "PYTHON_EXE=.\WPy64-312101\python\python.exe"
set "HOME=%CD%"
set GIT_EXEC_PATH=

REM --- Show git version for debugging ---
git --version
git submodule --version

REM --- Install requirements if needed ---
if exist requirements.txt (
    echo Installing Python requirements...
    "%PYTHON_EXE%" -m pip install --upgrade pip
    "%PYTHON_EXE%" -m pip install -r requirements.txt
)

REM --- Start the API ---
echo Starting API...
"%PYTHON_EXE%" api\main.py

pause