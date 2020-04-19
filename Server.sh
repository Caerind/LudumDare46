#!/bin/bash

git pull
cd build
cmake ..
make
cd ..
rm -f ServerApp
cp build/LudumDare46-Server/LudumDare46Server ServerApp
./ServerApp

