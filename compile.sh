#!/bin/sh

cc -g -I lisa -I utils src/lisaobj.c lisa/*.c utils/*.c -o lisaobj
cc -g -I lisa -I utils src/lisapack.c lisa/*.c utils/*.c -o lisapack
