
mkdir build > nul
cd build
cmake -G "Unix Makefiles" ../
cmake --build .
cmake --build . --config Release
cd ..
