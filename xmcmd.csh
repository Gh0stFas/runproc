#!/bin/tcsh

# Instruct the shell to ignore interrupts. This allows the python module
# to catch interrupts properly, returning to the shell allowing it to
# drop X-MIDAS properly with xmend.
onintr -

# Start xm
source $XMDISK/xm/unix/xmstart

# The -u instructs python to setup stdout, stderr and stdin as completely
# unbuffured. This allows for quick updates to the log file produced by
# runproc.
xm python -u testpy.py

# Stop xm
source $XMDISK/xm/unix/xmend
