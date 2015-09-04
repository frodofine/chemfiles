cd %SRC_DIR%
mkdir build
cd build

cmake .. -DBUILD_TESTS=ON -DPYTHON_BINDING=ON -DCMAKE_INSTALL_PREFIX=%PREFIX%
cmake --build . --target install