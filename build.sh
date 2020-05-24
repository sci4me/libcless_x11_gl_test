#!/bin/bash

g++ -s -nostdlib -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-stack-protector -Wa,--noexecstack -fno-builtin start.s syscall.s libcless_x11_gl_test.cpp -lX11 -lGL -o test