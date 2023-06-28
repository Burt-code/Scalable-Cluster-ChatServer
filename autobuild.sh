#!/bin/bash

set -x  # debug mode

rm -rf `pwd`/build/*
cd `pwd`/build &&
	cmake .. &&
	make
