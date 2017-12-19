#! /usr/bin/env bash

python tools/build.py --jerry-libc=on --jerry-libm=on --jerry-cmdline=on --snapshot-exec=ON --snapshot-save=on --mem-heap=18 --clean

