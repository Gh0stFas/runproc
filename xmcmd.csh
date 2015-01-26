#!/bin/tcsh

# Start X-MIDAS
source $XMDISK/xm/unix/xmstart

xm python /home/jsegars/git/runproc/testpy.py

# Stop X-MIDAS
source $XMDISK/xm/unix/xmend
