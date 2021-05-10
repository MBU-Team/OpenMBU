@echo off
call bison.bat CMD cmdgram.c CMDgram.y . cmdgram.cpp
..\..\bin\flex\flex -PCMD -oCMDscan.cpp CMDscan.l
