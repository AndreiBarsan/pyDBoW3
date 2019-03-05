#!/bin/bash


cd build
mkdir -p dist
cd dist
mkdir -p pyDBoW3
cp ../pyDBoW3.so pyDBoW3
cp ../../src/__init__.py pyDBoW3
cp ../../src/setup.py .
cp ../../src/MANIFEST.in .

if [[ "$BUILD_PYTHON" == "python3" ]]; then
    python3 setup.py bdist_wheel
else
    python setup.py bdist_wheel
fi

