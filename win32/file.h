/*
C_UPDI file.h
Author: Jonty   www.tyjean.com
                https://github.com/jarl93rsa
(2020)


Provide os-specific file functions open/close read/write etc for updi.c, using a common struct File.

Porting C_UPDI to a new platform will require re-writing these functions
*/

#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <stdbool.h>

//Non OS-specific struct for updi.c to access file handle, containing OS-specific handle
typedef struct {
    FILE *fp;
} File;


bool open_file(File *file, char *fname);
bool file_read_line(File *file, char *buffer, int length);
void close_file(File *file);


#endif