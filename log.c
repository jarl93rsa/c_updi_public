/*
C_UPDI log.c
Author: Jonty   www.tyjean.com
                https://github.com/jarl93rsa
(2020)

Logging functions called from updi.c, change these to however you want to display output, as an example I have filled out the functions to 
resemble a basic printf() structure, and to only output log_str if VERBOSE = true, but always to output log_error() and log_important()
The reason for its existance is to not have to change all output in updi.c if you implement c_updi into new projects, a GUI for example
*/

#include <stdio.h>
#include <stdarg.h>

#include "log.h"


/*
Generic messages sent here
Chosen here to only output when VERBOSE = true
*/
void log_str(char *str, ...){
    if(!LOG_VERBOSE) return;

    va_list argp;
    va_start(argp, str);

    while(*str != '\0'){
        if(*str == '%'){
            str++;
            if(*str == '%'){
                printf("%%");
            }else if(*str == 'c'){
                char chr = va_arg(argp, int); //types narrower than int promoted to int
                printf("%c", chr);
            }else if(*str == 'd'){
                int num = va_arg(argp, int); //types narrower than int promoted to int
                printf("%d", num);
            }else{
                printf("%c", *str);
            }
        }else{
            printf("%c", *str);
        }        
        str++;
    }

    va_end(argp);
    
    return;
}


/*
Important messages / updates sent here
*/
void log_important(char *str, ...){
    va_list argp;
    va_start(argp, str);

    while(*str != '\0'){
        if(*str == '%'){
            str++;
            if(*str == '%'){
                printf("%%");
            }else if(*str == 'c'){
                char chr = va_arg(argp, int); //types narrower than int promoted to int
                printf("%c", chr);
            }else if(*str == 'd'){
                int num = va_arg(argp, int); //types narrower than int promoted to int
                printf("%d", num);
            }else{
                printf("%c", *str);
            }
        }else{
            printf("%c", *str);
        }        
        str++;
    }

    va_end(argp);

    return;
}

/*
Error messages sent here
*/
void log_error(char *str, ...){

    printf("ERROR: ");

    va_list argp;
    va_start(argp, str);

    while(*str != '\0'){
        if(*str == '%'){
            str++;
            if(*str == '%'){
                printf("%%");
            }else if(*str == 'c'){
                char chr = va_arg(argp, int); //types narrower than int promoted to int
                printf("%c", chr);
            }else if(*str == 'd'){
                int num = va_arg(argp, int); //types narrower than int promoted to int
                printf("%d", num);
            }else{
                printf("%c", *str);
            }
        }else{
            printf("%c", *str);
        }        
        str++;
    }

    va_end(argp);

    return;
}