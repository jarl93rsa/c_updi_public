/*
C_UPDI time.h
Author: Jonty   www.tyjean.com
                https://github.com/jarl93rsa
(2020)

Provide an os-specific arduino-style millis() functions for use in updi.c when checking if a process has timed out.

Porting C_UPDI to a new platform will require re-writing this function
*/

#ifndef TIME_H
#define TIME_H

unsigned long int millis(void);

#endif