/*
C_UPDI
Author: Jonty   www.tyjean.com
                https://github.com/jarl93rsa

Basically a c version of PyUPDI (https://github.com/mraardvark/pyupdi) so thanks mraardvark for that. 

Basic Usage:
-Change the log functions in log.c to whatever logging method you want (I just use printf() but that wouldnt be usefull as feedback in a GUI based program etc) 
-Make sure you define the OS, this selects which os-specific src files to grab
    Only options at the moment are
    -DUPDI_WIN32    
    Will make a generic linux one soon

Eg build with gcc:  gcc main.c -DUPDI_WIN32 win32\file.c win32\serial.c win32\time.c log.c updi.c -o main
                    gcc main.c -DUPDI_LINUX linux\file.c linux\serial.c linux\time.c log.c updi.c -o main
    

-Check updi.h for available process args not covered in the basic example below
-Check updi.h for available device list

To port to a new operating system add a folder for the time / file / serial files, make a define and add to the #if, #elif, #else in updi.h
*/

#include "updi.h"

void example_write_verify_flash();
void example_read_flash();
void example_read_write_fuses();
void example_erase_flash();
void example_get_device_info();


/*
Simple example usage program to demonstrate how to use c_updi and its various capabilities and time how long they take.
*/
int main(){
    LOG_VERBOSE = false;    //look at log.h / log.c to implement updi output however you want

    example_get_device_info();    
    //example_write_verify_flash();
    //example_read_flash();
    //example_read_write_fuses();
    //example_erase_flash();

    return 0;
}

/*
Get the System Information Block as well as some other bits from device
*/
void example_get_device_info(){
    UPDI updi;     
    updi_init(&updi, 5, 115200, ATMEGA4809, UPDI_PROCESS_GET_INFO, NULL, 0); //updi, comport, baudrate, device, process args, filename, filename length
    long unsigned int start = millis();
    updi_process(&updi);
    printf("\r\nELAPSED TIME: %ld ms\r\n", millis() - start);

    printf("Device info:\r\n");
    printf("Family: %s\r\n", updi.info.family);
    printf("NVM version: %s\r\n", updi.info.nvm_version);
    printf("OCD version: %s\r\n", updi.info.ocd_version);
    printf("DBG OSC freq: %c\r\n", updi.info.dbg_osc_freq);
    printf("PDI rev: %d\r\n", updi.info.pdi_rev);
    printf("Device ID: 0x%x 0x%x 0x%x\r\n", updi.info.dev_id[0], updi.info.dev_id[1], updi.info.dev_id[2]);
    printf("Device rev: %c\r\n", updi.info.dev_rev);

    return;
}

/*
Write .hex file to flash, read back flash and compare to file to check no memory mismatch
*/
void example_write_verify_flash(){
    UPDI updi;     
    updi_init(&updi, 5, 115200, ATMEGA4809, UPDI_PROCESS_WRITE_FLASH | UPDI_PROCESS_VERIFY_FLASH, "your_hex_file.hex", 17); //updi, comport, baudrate, device, process args, filename, filename length            
    long unsigned int start = millis();
    updi_process(&updi);
    printf("\r\nELAPSED TIME: %ld ms\r\n", millis() - start);    

    return;
}

/*
Read full flash of device
*/
void example_read_flash(){
    UPDI updi;     
    updi_init(&updi, 5, 115200, ATMEGA4809, UPDI_PROCESS_READ_FLASH, NULL, 0); //updi, comport, baudrate, device, process args, filename, filename length
    long unsigned int start = millis();
    updi_process(&updi);
    printf("\r\nELAPSED TIME: %ld ms\r\n", millis() - start);
    printf("FLASH:\r\n");
    for(int i = 0; i < updi.device.flash_size; i++){
        printf("%d ", updi.flash_data_read[i]);
    }
    printf("\r\n\r\n");   

    return;
}

/*
Read fuse values, then write them back
*/
void example_read_write_fuses(){
    UPDI updi;     
    updi_init(&updi, 5, 115200, ATMEGA4809, UPDI_PROCESS_READ_FUSES, NULL, 0); //updi, comport, baudrate, device, process args, filename, filename length
    long unsigned int start = millis();
    updi_process(&updi);
    printf("\r\nELAPSED TIME: %ld ms\r\n", millis() - start);

    printf("FUSES: fuse:value\r\n");
    uint8_t fuses[updi.device.num_fuses];
    memcpy(fuses, updi.fuse_values_read, updi.device.num_fuses);    //read fuse values saved in updi.fuse_values_read

    for(int i = 0; i < updi.device.num_fuses; i++){
        printf("%d:%d\r\n", i, fuses[i]);
    }

    //write same values back
    printf("\r\nWriting values back:\r\n");    

    updi_init(&updi, 5, 115200, ATMEGA4809, UPDI_PROCESS_WRITE_FUSES, NULL, 0);
    memcpy(updi.fuse_values_write, fuses, updi.device.num_fuses);   //write in values you want to updi.fuse_values_write after calling init()

    start = millis();
    updi_process(&updi);
    printf("\r\nELAPSED TIME: %ld ms\r\n", millis() - start);

    return;
}

/*
Erase memory
*/
void example_erase_flash(){
    UPDI updi;     
    updi_init(&updi, 5, 115200, ATMEGA4809, UPDI_PROCESS_ERASE, NULL, 0); //updi, comport, baudrate, device, process args, filename, filename length
    long unsigned int start = millis();
    updi_process(&updi);
    printf("\r\nELAPSED TIME: %ld ms\r\n", millis() - start);

    return;
}


