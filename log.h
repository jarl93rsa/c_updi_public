/*
C_UPDI log.h
Author: Jonty   www.tyjean.com
                https://github.com/jarl93rsa
(2020)

Basic logging functions to control which and how output is displayed from the updi process, to be implemented however you choose
*/

#ifndef LOG_H
#define LOG_H

#include <stdbool.h>

bool LOG_VERBOSE;

void log_str(char *str, ...);
void log_important(char *str, ...);
void log_error(char *str, ...);

#endif