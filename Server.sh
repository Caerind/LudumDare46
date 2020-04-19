#!/bin/bash

git pull
cd build
cmake ..
make
cd ..
./build/LudumDare46-Server/LudumDare46Server
