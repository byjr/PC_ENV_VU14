#!/bin/sh
CUR_DIR=`pwd`
export LD_LIBRARY_PATH=$CUR_DIR/staging/usr/lib:$LD_LIBRARY_PATH
export PATH=$CUR_DIR/staging/usr/bin:$PATH