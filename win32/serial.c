/*
C_UPDI serial.c
Author: Jonty   www.tyjean.com
                https://github.com/jarl93rsa
(2020)

Provide os-specific serial functions open/close read/write configure etc for updi.c, using a common struct Serial.

Porting C_UPDI to a new platform will require re-writing these functions
*/

#include <windows.h>
#include <stdbool.h>
#include <stdio.h>

#include "../log.h" 
#include "serial.h"

/*
Open serial connection at desired settings
*/
bool serial_init(Serial *serial){

    log_str("in serial.init()\r\n");

    char com_str[20];
    memset(com_str, 0, 20);
    sprintf(com_str, "\\\\.\\COM%d", serial->com_port);
    serial->h_serial = CreateFile(com_str, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if(serial->h_serial == INVALID_HANDLE_VALUE){        
        log_error("error opening serial port\r\n");
        return false;
    }
    
    log_str("opened serial port\r\n");    

    serial->dcb_serial_params.DCBlength = sizeof(serial->dcb_serial_params);

    int status = GetCommState(serial->h_serial, &(serial->dcb_serial_params));
    
    serial->dcb_serial_params.BaudRate = serial->baudrate;
    serial->dcb_serial_params.ByteSize = 8;         // Setting ByteSize = 8
    serial->dcb_serial_params.StopBits = TWOSTOPBITS;// Setting StopBits = 1
    serial->dcb_serial_params.Parity   = EVENPARITY;  // Setting Parity = None

    SetCommState(serial->h_serial, &(serial->dcb_serial_params));

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout         = 50; // in milliseconds       max interval between arrival between 2 bytes
    timeouts.ReadTotalTimeoutConstant    = 50; // in milliseconds       total timeout period for read ops, added to multiplier * numbytes
    timeouts.ReadTotalTimeoutMultiplier  = 10; // in milliseconds       multiplied by requested number of bytes 
    timeouts.WriteTotalTimeoutConstant   = 50; // in milliseconds       
    timeouts.WriteTotalTimeoutMultiplier = 10; // in milliseconds

    SetCommTimeouts(serial->h_serial, &timeouts);

    return true;
}

/*
Change the baud rate of serial connection.
This function is not necessary for normal use and wont necessarily require re-writing when porting to a new system.
This is used in commented-out section in updi_process where the updi clock speed is increased to allow faster baudrates to try and decrease programming time.
Tested the process with much higher baud rates, up to 900000 (datasheet max), without loss of information but with diminishing speed returns. Far too much latency in the
whole USB-UART process to make a significant difference. 
Left this here, and the commented-out speed change in updi_process() for reference.
*/
bool serial_change_baud(Serial *serial, uint32_t baudrate){
    char com_str[20];
    memset(com_str, 0, 20);
    sprintf(com_str, "\\\\.\\COM%d", serial->com_port);
    serial->h_serial = CreateFile(com_str, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if(serial->h_serial == INVALID_HANDLE_VALUE){        
        log_error("error opening serial port\r\n");
        return false;
    }
    
    log_str("opened serial port\r\n");
    
    serial->dcb_serial_params.DCBlength = sizeof(serial->dcb_serial_params);

    int status = GetCommState(serial->h_serial, &(serial->dcb_serial_params));
    
    serial->dcb_serial_params.BaudRate = baudrate;//CBR_115200; 
    serial->dcb_serial_params.ByteSize = 8;         // Setting ByteSize = 8
    serial->dcb_serial_params.StopBits = TWOSTOPBITS;// Setting StopBits = 1
    serial->dcb_serial_params.Parity   = EVENPARITY;  // Setting Parity = None

    SetCommState(serial->h_serial, &(serial->dcb_serial_params));

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout         = 50; // in milliseconds       max interval between arrival between 2 bytes
    timeouts.ReadTotalTimeoutConstant    = 50; // in milliseconds       total timeout period for read ops, added to multiplier * numbytes
    timeouts.ReadTotalTimeoutMultiplier  = 10; // in milliseconds       multiplied by requested number of bytes 
    timeouts.WriteTotalTimeoutConstant   = 50; // in milliseconds       
    timeouts.WriteTotalTimeoutMultiplier = 10; // in milliseconds

    SetCommTimeouts(serial->h_serial, &timeouts);

    return true;
}

/*
Reconfigure the serial port at a much lower baud rate to be able to send a "double break" of required length to UPDI
*/
bool serial_init_dbl_break(Serial *serial){
    char com_str[20];
    memset(com_str, 0, 20);
    sprintf(com_str, "\\\\.\\COM%d", serial->com_port);
    serial->h_serial = CreateFile(com_str, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if(serial->h_serial == INVALID_HANDLE_VALUE){        
        log_error("error opening serial port\r\n");
        return false;
    }
    
    log_str("opened serial port\r\n");
    
    serial->dcb_serial_params.DCBlength = sizeof(serial->dcb_serial_params);

    int status = GetCommState(serial->h_serial, &(serial->dcb_serial_params));

    serial->dcb_serial_params.BaudRate = 300;  // Setting BaudRate = 9600
    serial->dcb_serial_params.ByteSize = 8;         // Setting ByteSize = 8
    serial->dcb_serial_params.StopBits = ONESTOPBIT;// Setting StopBits = 1
    serial->dcb_serial_params.Parity   = NOPARITY;  // Setting Parity = None

    SetCommState(serial->h_serial, &(serial->dcb_serial_params));

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout         = 50; // in milliseconds       max interval between arrival between 2 bytes
    timeouts.ReadTotalTimeoutConstant    = 50; // in milliseconds       total timeout period for read ops, added to multiplier * numbytes
    timeouts.ReadTotalTimeoutMultiplier  = 10; // in milliseconds       multiplied by requested number of bytes 
    timeouts.WriteTotalTimeoutConstant   = 50; // in milliseconds       
    timeouts.WriteTotalTimeoutMultiplier = 10; // in milliseconds

    SetCommTimeouts(serial->h_serial, &timeouts);

    
    return true;
}

/*

*/
void serial_close(Serial *serial){
    CloseHandle(serial->h_serial);
}


/*
Send bytes to serial, these will echo back
*/
bool serial_send(Serial *serial, uint8_t *data, uint16_t length){
    long unsigned int bytes_written = 0;
    int status = WriteFile(serial->h_serial, data, length, &bytes_written, NULL);

    //read back echo
    long unsigned int bytes_read = 0;
    char recv[length];
    ReadFile(serial->h_serial, recv, length, &bytes_read, NULL);
    
    if(bytes_read != length){
        log_error("serial_send error, bytes received != bytes sent\r\n");
        return false;
    }
    
    return true;
}

/*
Send bytes to serial, read echo as well as expected reply
*/
bool serial_send_receive(Serial *serial, uint8_t *data, uint16_t send_len, uint8_t *recv, uint16_t recv_len){
    long unsigned int bytes_written = 0;
    int status = WriteFile(serial->h_serial, data, send_len, &bytes_written, NULL);    

    long unsigned int bytes_read = 0;
    char recv_buf[send_len + recv_len];

    ReadFile(serial->h_serial, recv_buf, send_len + recv_len, &bytes_read, NULL);
   
    if(bytes_read != send_len + recv_len){
        log_error("serial_send error, bytes received != bytes sent + bytes wanted\r\n");
        return false;
    }

    for(uint16_t i = send_len; i < send_len + recv_len; i++){        
        recv[i - send_len] = recv_buf[i];
    }     
    
    return true;
}
