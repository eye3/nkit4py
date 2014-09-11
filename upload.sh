#!/bin/bash

rm -r build
rm -r dist
rm -r nkit4py.egg-info

python setup.py register sdist --formats=gztar,zip upload

