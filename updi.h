/*
C_UPDI updi.h
Author: Jonty   www.tyjean.com
                https://github.com/jarl93rsa
(2020)
*/

#ifndef UPDI_H
#define UPDI_H

#include <inttypes.h>

/*
Pick which OS-specific files to include, ideally will port to several systems
*/
#if defined UPDI_WIN32
    #include "win32\file.h"
    #include "win32\serial.h"
    #include "win32\time.h"
#elif defined UPDI_LINUX
    #include "linux\file.h"
    #include "linux\serial.h"
    #include "linux\time.h"
#endif

#include "log.h"

#define UPDI_BREAK                          0x00

#define UPDI_LDS                            0x00
#define UPDI_STS                            0x40
#define UPDI_LD                             0x20
#define UPDI_ST                             0x60
#define UPDI_LDCS                           0x80
#define UPDI_STCS                           0xC0
#define UPDI_REPEAT                         0xA0
#define UPDI_KEY                            0xE0

#define UPDI_PTR                            0x00
#define UPDI_PTR_INC                        0x04
#define UPDI_PTR_ADDRESS                    0x08

#define UPDI_ADDRESS_8                      0x00
#define UPDI_ADDRESS_16                     0x04

#define UPDI_DATA_8                         0x00
#define UPDI_DATA_16                        0x01

#define UPDI_KEY_SIB                        0x04
#define UPDI_KEY_KEY                        0x00

#define UPDI_KEY_64                         0x00
#define UPDI_KEY_128                        0x01

#define UPDI_SIB_8BYTES                     UPDI_KEY_64
#define UPDI_SIB_16BYTES                    UPDI_KEY_128

#define UPDI_REPEAT_BYTE                    0x00
#define UPDI_REPEAT_WORD                    0x01

#define UPDI_PHY_SYNC                       0x55
#define UPDI_PHY_ACK                        0x40

#define UPDI_MAX_REPEAT_SIZE                0xFF

//CS and ASI Register Address map
#define UPDI_CS_STATUSA                     0x00
#define UPDI_CS_STATUSB                     0x01
#define UPDI_CS_CTRLA                       0x02
#define UPDI_CS_CTRLB                       0x03
#define UPDI_ASI_KEY_STATUS                 0x07
#define UPDI_ASI_RESET_REQ                  0x08
#define UPDI_ASI_CTRLA                      0x09
#define UPDI_ASI_SYS_CTRLA                  0x0A
#define UPDI_ASI_SYS_STATUS                 0x0B
#define UPDI_ASI_CRC_STATUS                 0x0C

#define UPDI_CTRLA_IBDLY_BIT                7
#define UPDI_CTRLA_RSD_BIT                  3
#define UPDI_CTRLB_CCDETDIS_BIT             3
#define UPDI_CTRLB_UPDIDIS_BIT              2

#define UPDI_KEY_NVM                        "NVMProg "
#define UPDI_KEY_CHIPERASE                  "NVMErase"
#define UPDI_KEY_USERROW_WRITE              "NVMUs&te"

#define UPDI_ASI_STATUSA_REVID              4
#define UPDI_ASI_STATUSB_PESIG              0

#define UPDI_ASI_KEY_STATUS_CHIPERASE       3
#define UPDI_ASI_KEY_STATUS_NVMPROG         4
#define UPDI_ASI_KEY_STATUS_UROWWRITE       5

#define UPDI_ASI_SYS_STATUS_RSTSYS          5
#define UPDI_ASI_SYS_STATUS_INSLEEP         4
#define UPDI_ASI_SYS_STATUS_NVMPROG         3
#define UPDI_ASI_SYS_STATUS_UROWPROG        2
#define UPDI_ASI_SYS_STATUS_LOCKSTATUS      0

#define UPDI_ASI_SYS_CTRLA_UROWWRITE_FINAL  1
#define UPDI_ASI_SYS_CTRLA_CLKREQ           0

#define UPDI_RESET_REQ_VALUE                0x59

//FLASH CONTROLLER
#define UPDI_NVMCTRL_CTRLA                  0x00
#define UPDI_NVMCTRL_CTRLB                  0x01
#define UPDI_NVMCTRL_STATUS                 0x02
#define UPDI_NVMCTRL_INTCTRL                0x03
#define UPDI_NVMCTRL_INTFLAGS               0x04
#define UPDI_NVMCTRL_DATAL                  0x06
#define UPDI_NVMCTRL_DATAH                  0x07
#define UPDI_NVMCTRL_ADDRL                  0x08
#define UPDI_NVMCTRL_ADDRH                  0x09

//CTRLA
#define UPDI_NVMCTRL_CTRLA_NOP              0x00
#define UPDI_NVMCTRL_CTRLA_WRITE_PAGE       0x01
#define UPDI_NVMCTRL_CTRLA_ERASE_PAGE       0x02
#define UPDI_NVMCTRL_CTRLA_ERASE_WRITE_PAGE 0x03
#define UPDI_NVMCTRL_CTRLA_PAGE_BUFFER_CLR  0x04
#define UPDI_NVMCTRL_CTRLA_CHIP_ERASE       0x05
#define UPDI_NVMCTRL_CTRLA_ERASE_EEPROM     0x06
#define UPDI_NVMCTRL_CTRLA_WRITE_FUSE       0x07

#define UPDI_NVM_STATUS_WRITE_ERROR         2
#define UPDI_NVM_STATUS_EEPROM_BUSY         1
#define UPDI_NVM_STATUS_FLASH_BUSY          0


//SUPPORTED DEVICES
#define ATMEGA4808                          0
#define ATMEGA4809                          1
#define ATMEGA3208                          2
#define ATMEGA3209                          3

#define ATTINY3216                          4
#define ATTINY3217                          5

#define ATTINY1604                          6
#define ATTINY1606                          7
#define ATTINY1607                          8
#define ATTINY1614                          9
#define ATTINY1616                          10
#define ATTINY1617                          11

#define ATTINY804                           12
#define ATTINY806                           13
#define ATTINY807                           14
#define ATTINY814                           15
#define ATTINY816                           16
#define ATTINY817                           17

#define ATTINY402                           18
#define ATTINY404                           19
#define ATTINY406                           20
#define ATTINY412                           21
#define ATTINY414                           22
#define ATTINY416                           23
#define ATTINY417                           24

#define ATTINY202                           25
#define ATTINY204                           26
#define ATTINY212                           27
#define ATTINY214                           28

#define UPDI_MAX_FLASH_SIZE                 48*1024
#define UPDI_MAX_FUSES                      11


//PROCESS ARGS
#define UPDI_PROCESS_ERASE                  1
#define UPDI_PROCESS_READ_FUSES             2
#define UPDI_PROCESS_WRITE_FUSES            4
#define UPDI_PROCESS_READ_FLASH             8
#define UPDI_PROCESS_WRITE_FLASH            16
#define UPDI_PROCESS_VERIFY_FLASH           32
#define UPDI_PROCESS_GET_INFO               64
#define UPDI_PROCESS_WRITE_USERROW          128

typedef struct {
    uint16_t    flash_start;
    uint16_t    flash_size;
    uint8_t     flash_pagesize;
    uint16_t    syscfg_address;
    uint16_t    nvmctrl_address;
    uint16_t    sigrow_address;
    uint16_t    fuses_address;
    uint16_t    userrow_address;
    uint8_t     num_fuses;
} Device;

typedef struct {
    char family[8];
    char nvm_version[4];
    char ocd_version[4];
    char dbg_osc_freq;

    uint8_t pdi_rev;
    uint8_t dev_id[3];
    char dev_rev;
} DeviceInfo;


typedef struct {
    Serial serial;
    Device device;
    DeviceInfo info;

    uint8_t com_port;
    uint32_t baudrate;
    uint8_t dev;
    uint8_t args;
    char hex_filename[256];

    uint8_t fuse_values_read[UPDI_MAX_FUSES];
    uint8_t fuse_values_write[UPDI_MAX_FUSES];

    uint8_t flash_data_read[UPDI_MAX_FLASH_SIZE];
    uint8_t flash_data_write[UPDI_MAX_FLASH_SIZE];
} UPDI;

void updi_init(UPDI *updi, uint8_t com_port, uint32_t baudrate, uint8_t dev, uint8_t args, char *fname, uint8_t fname_len);
void updi_process(UPDI *updi);
void updi_cleanup(UPDI *updi);


#endif