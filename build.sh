#/usr/bin/env bash

CC=gcc
CFLAGS=-std=c99
BIN=bin

mkdir -p ${BIN}
find ${BIN} -type f -delete

${CC} ${CFLAGS} holysnake.c -o ${BIN}/holysnake -lSDL2
${CC} ${CFLAGS} mapwalk.c -o ${BIN}/mapwalk -lSDL2
${CC} ${CFLAGS} mapwalk2.c -o ${BIN}/mapwalk2 -lSDL2
