#!/bin/bash

rm -r build

export ENV_ROOT=$HOME/env

python setup.py install

