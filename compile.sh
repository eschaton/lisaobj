#!/bin/sh

cc -g -I lisa -I utils src/*.c lisa/*.c utils/*.c -o lisaobj
