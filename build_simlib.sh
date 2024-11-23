#!/bin/bash

if [ ! -d "./simlib" ]; then
    sudo apt update
    sudo apt install -y wget tar

    wget https://www.fit.vutbr.cz/~peringer/SIMLIB/source/simlib-3.09-20221108.tar.gz

    tar -xzf simlib-3.09-20221108.tar.gz

    cd simlib
    make
    cd ..

    rm -f simlib-3.09-20221108.tar.gz

    echo "Simlib installed successfully."
fi
