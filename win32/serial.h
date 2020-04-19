/*
C_UPDI serial.h
Author: Jonty   www.tyjean.com
                https://github.com/jarl93rsa
(2020)

Provide os-specific serial functions open/close read/write configure etc for updi.c, using a common struct Serial.

Porting C_UPDI to a new platform will require re-writing these functions
*/

#ifndef SERIAL_H
#define SERIAL_H

#include <windows.h>
#include <inttypes.h>
#include <stdbool.h>

#define MAX_RECV_LEN 256

typedef struct {
    HANDLE h_serial;
    DCB dcb_serial_params;
    uint8_t com_port;
    uint32_t baudrate;
} Serial;

bool serial_init(Serial *serial);
bool serial_init_dbl_break(Serial *serial);
bool serial_change_baud(Serial *serial, uint32_t baudrate);
bool serial_send(Serial *serial, uint8_t *data, uint16_t length);
bool serial_send_receive(Serial *serial, uint8_t *data, uint16_t send_len, uint8_t *recv, uint16_t recv_len);
void serial_close(Serial *serial);

#endif