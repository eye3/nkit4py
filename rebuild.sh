#!/bin/bash

rm -r build
rm -r dist
rm -r nkit4py.egg-info

python setup.py build

