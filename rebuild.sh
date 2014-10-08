#!/bin/bash

rm -r build
rm -r dist
rm -r nkit4py.egg-info

#export CFLAGS='-Wall -O0 -ggdb3'
#export CPPFLAGS='-Wall -O0 -ggdb3'

python setup.py build
python setup.py install
