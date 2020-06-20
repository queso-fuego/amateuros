#!/bin/sh
for file in ../src/*.asm
do
	fasm $file ${file%%asm}bin;
done
