@echo off
pushd %~dp0
mkdir debug
mkdir release

cd debug
cmake ../../ -G"MinGW Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug
cd ..

cd release
cmake ../../ -G"MinGW Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release
cd ..

popd
