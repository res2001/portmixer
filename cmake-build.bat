@echo off
setlocal enabledelayedexpansion

set "srcdir=."
for %%a in ("%srcdir%") do set "srcdir=%%~fa"
set "cmake=cmake.exe"
set "install_prefix=%~dp0\build"

set "cmake_opt=-DCMAKE_VERBOSE_MAKEFILE=ON -DPA_DIR=D:/Users/res/msyshome/git/portaudio -DBUILD_TEST=ON"
::set "cmake_opt=-DPA_BUILD_STATIC=OFF -DPA_BUILD_SHARED=ON-DPA_DLL_LINK_WITH_STATIC_RUNTIME=ON -DPA_CONFIG_LIB_OUTPUT_PATH=ON -DPA_LIBNAME_ADD_SUFFIX=OFF"
::set "cmake_config_type=Release;Debug"
set "cmake_config_type=Debug"

set "cmake_config_type_space=%cmake_config_type:;= %"

call:cmakebuild x64

::call:cmakebuild win32

::md "x32" 2>nul
::pushd "x32"
::"%cmake%" ../.. -G "Visual Studio 15 2017" -A Win32 -DCMAKE_CONFIGURATION_TYPE=%cmake_config_type% %cmake_opt% %ext_lib% %ext_include% %ext_found%
::for %%a in (%cmake_config_type_space%) do "%cmake%" --build . --config %%a --target install
::popd

exit /b 0

:cmakebuild
set "builddir=tmp\%~1"
md "%builddir%" 2>nul
pushd "%builddir%"
"%cmake%" %srcdir% -G "Visual Studio 15 2017" -A %~1 -DCMAKE_BUILD_TYPE=%cmake_config_type% %cmake_opt% -DCMAKE_INSTALL_PREFIX=%install_prefix%\%~1 %ext_lib% %ext_include% %ext_found%
for %%a in (%cmake_config_type_space%) do "%cmake%" --build . --config %%a --target install
popd

::call:movetests %~1 examples
::call:movetests %~1 test

::rd /s /q "%builddir%"

exit /b 0

:movetests
if exist "%~1\%~2\Release" (
	1>nul 2>&1 md %install_prefix%\%~1\bin\%~2
	1>nul 2>&1 copy "%~1\%~2\Release\*.exe" %install_prefix%\%~1\bin\%~2
)
exit /b 0

:set_ext_var
set "ext_lib="
set "ext_include="
set "ext_found="
for %%a in (OGG VORBIS VORBISENC FLAC SQLITE3) do (
	set "ext_lib=!ext_lib! -D%%a_LIBRARY=%VCPKG_PATH%\installed\%VCPKG_TARGET_TRIPLET%\lib\%%a.lib"
	set "ext_include=!ext_include! -D%%a_INCLUDE_DIR=%VCPKG_PATH%\installed\%VCPKG_TARGET_TRIPLET%\include"
	set "ext_found=!ext_found! -D%%a_FOUND=ON"
)
exit /b 0
