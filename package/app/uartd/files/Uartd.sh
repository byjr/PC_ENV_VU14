#!/bin/sh
ltrace -f -o slave.log uartd -t /tmp/slave.fifo:/tmp/master.fifo -l1111 &
uartd -l1111 -t /tmp/master.fifo:/tmp/slave.fifo -m
