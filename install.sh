#!/bin/bash

export QT_SELECT=5

rm -f `find -name "Makefile"`

qmake && make -j20

sudo make install 
