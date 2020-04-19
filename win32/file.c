/*
C_UPDI file.c
Author: Jonty   www.tyjean.com
                https://github.com/jarl93rsa
(2020)

Provide os-specific file functions open/close read/write etc for updi.c, using a common struct File.

Porting C_UPDI to a new platform will require re-writing these functions
*/

#include <stdio.h>
#include <stdbool.h>

#include "file.h"
#include "../log.h"


bool open_file(File *file, char *fname){

    file->fp = fopen(fname, "r");

    if(file->fp == NULL){
        log_str("couldnt open file\r\n");
        return false;
    }

    log_str("opened file\r\n");
    
    return true;
}

/*
Read lines up until EOF
*/
bool file_read_line(File *file, char *buffer, int length){

    if(fgets(buffer, length, file->fp) != NULL){
        return true;
    }

    return false;
}

void close_file(File *file){
    fclose(file->fp);
    return;
}

