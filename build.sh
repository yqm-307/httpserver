#!/bin/bash

isclear="$*"    #参数列表
clear()
{
    rm cmake_install.cmake
    rm CMakeCache.txt
    rm -rf Log
    rm -rf CMakeFiles
    rm Makefile
    rm webserver
}

build()
{
    cmake .
    make
}


if [ "${isclear}" = "clear" ]
then
    clear
else
    mkdir Log   #创建Log文件夹
    build
fi



