#!/bin/bash

rm -r dist
rm -r nkit4py.egg-info

python setup.py register sdist --formats=gztar,zip upload

