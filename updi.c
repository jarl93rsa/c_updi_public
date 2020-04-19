/*
C_UPDI updi.c
Author: Jonty   www.tyjean.com
                https://github.com/jarl93rsa
(2020)

Contains all updi-related functions, almost entirely a translation of PyUPDI (https://github.com/mraardvark/pyupdi) to be more suitable for my projects, so many thanks to mraardvark for his work on that.

Porting this to new systems shouldn't require any changes to this file.
*/

#include <inttypes.h>
#include <string.h>

#include "updi.h"
    
static void        send_handshake(Serial *serial);
static bool        send_double_break(Serial *serial);
static void        init(Serial *serial);
static bool        check(Serial *serial);
static void        get_device_info(Serial *serial, Device device, DeviceInfo *info);

static bool        in_prog_mode(Serial *serial);
static bool        enter_progmode(Serial *serial);
static void        leave_progmode(Serial *serial);    

static void        apply_reset(Serial *serial, bool reset);

static bool        unlock_device(Serial *serial);
static bool        chip_erase(Serial *serial, Device device);

static bool        read_data(Serial *serial, uint16_t address, uint16_t size, uint8_t *ret);
static bool        read_data_words(Serial *serial, uint16_t address, uint16_t numwords, uint8_t *buffer);
static uint8_t     read_fuse(Serial *serial, Device device, uint8_t fuse);
static bool        read_flash(Serial *serial, Device device, uint16_t address, uint16_t size, uint8_t *buffer);

static bool        write_data(Serial *serial, uint16_t address, uint8_t *data, uint16_t len);
static bool        write_data_words(Serial *serial, uint16_t address, uint8_t *data, uint16_t numwords);
static bool        write_fuse(Serial *serial, Device device, uint8_t fuse, uint8_t value);
static bool        write_flash(Serial *serial, Device device, uint16_t address, uint8_t *data, uint16_t len);

static bool        load_ihex(char *filename, uint8_t *data, uint16_t *length);

static uint8_t     ldcs(Serial *serial, uint8_t address);
static uint8_t     ld(Serial *serial, uint16_t address);
static bool        ld16(Serial *serial, uint16_t address, uint16_t *word);
static bool        ld_ptr_inc(Serial *serial, uint8_t *buffer, uint16_t size);
static bool        ld_ptr_inc16(Serial *serial, uint8_t *buffer, uint16_t numwords);

static void        stcs(Serial *serial, uint8_t address, uint8_t value);
static bool        st(Serial *serial, uint16_t address, uint8_t value);
static bool        st16(Serial *serial, uint16_t address, uint16_t value);
static bool        st_ptr(Serial *serial, uint16_t address);
static bool        st_ptr_inc(Serial *serial, uint8_t *data, uint16_t size);
static void        st_ptr_inc16(Serial *serial, uint8_t *data, uint16_t numwords);

static void        repeat(Serial *serial, uint16_t repeats);
static void        key(Serial *serial, uint8_t size, uint8_t *key);
static bool        progmode_key(Serial *serial);
static bool        wait_unlocked(Serial *serial, uint16_t timeout);
static bool        wait_flash_ready(Serial *serial, Device device);
static bool        execute_nvm_command(Serial *serial, Device device, uint8_t command);
static bool        write_nvm(Serial *serial, Device device, uint16_t address, uint8_t *data, uint16_t len, uint8_t command, bool use_word_acess);


void updi_init(UPDI *updi, uint8_t com_port, uint32_t baudrate, uint8_t dev, uint8_t args, char *fname, uint8_t fname_len){
    if(fname != NULL){
        memset(updi->hex_filename, 0, 256);
        memcpy(updi->hex_filename, fname, fname_len);
    }

    updi->com_port = com_port;
    updi->baudrate = baudrate;
    updi->dev = dev;    

    memset(updi->fuse_values_read, 0, updi->device.num_fuses);
    memset(updi->fuse_values_write, 0, updi->device.num_fuses);
    memset(updi->flash_data_read, 0, UPDI_MAX_FLASH_SIZE);
    memset(updi->flash_data_write, 0, UPDI_MAX_FLASH_SIZE);

    memset(updi->info.family, 0, 8);
    memset(updi->info.nvm_version, 0, 8);
    memset(updi->info.ocd_version, 0, 8);

    updi->args = args;    
    
    //check numfuses is correct for everything other than atmega4808/9
    switch(dev){
        case ATMEGA4808:
        case ATMEGA4809:{
            updi->device.flash_start =        0x4000;
            updi->device.flash_size =         48*1024;
            updi->device.flash_pagesize =     128;
            updi->device.syscfg_address =     0x0F00;
            updi->device.nvmctrl_address =    0x1000;
            updi->device.sigrow_address =     0x1100;
            updi->device.fuses_address =      0x1280;
            updi->device.userrow_address =    0x1300;
            updi->device.num_fuses =          11;
            break;
        }

        case ATMEGA3208:
        case ATMEGA3209:{
            updi->device.flash_start =        0x4000;
            updi->device.flash_size =         32*1024;
            updi->device.flash_pagesize =     128;
            updi->device.syscfg_address =     0x0F00;
            updi->device.nvmctrl_address =    0x1000;
            updi->device.sigrow_address =     0x1100;
            updi->device.fuses_address =      0x1280;
            updi->device.userrow_address =    0x1300;
            updi->device.num_fuses =          11;
            break;
        }

        case ATTINY3216:
        case ATTINY3217:{
            updi->device.flash_start =        0x8000;
            updi->device.flash_size =         32*1024;
            updi->device.flash_pagesize =     128;
            updi->device.syscfg_address =     0x0F00;
            updi->device.nvmctrl_address =    0x1000;
            updi->device.sigrow_address =     0x1100;
            updi->device.fuses_address =      0x1280;
            updi->device.userrow_address =    0x1300;
            updi->device.num_fuses =          11;
            break;
        }

        
        case ATTINY1604:
        case ATTINY1606:
        case ATTINY1607:
        case ATTINY1614:
        case ATTINY1616:
        case ATTINY1617:{
            updi->device.flash_start =        0x8000;
            updi->device.flash_size =         16*1024;
            updi->device.flash_pagesize =     64;
            updi->device.syscfg_address =     0x0F00;
            updi->device.nvmctrl_address =    0x1000;
            updi->device.sigrow_address =     0x1100;
            updi->device.fuses_address =      0x1280;
            updi->device.userrow_address =    0x1300;
            updi->device.num_fuses =          11;
            break;
        }         

        case ATTINY804:
        case ATTINY806:
        case ATTINY807:
        case ATTINY814:
        case ATTINY816:
        case ATTINY817:{
            updi->device.flash_start =        0x8000;
            updi->device.flash_size =         8*1024;
            updi->device.flash_pagesize =     64;
            updi->device.syscfg_address =     0x0F00;
            updi->device.nvmctrl_address =    0x1000;
            updi->device.sigrow_address =     0x1100;
            updi->device.fuses_address =      0x1280;
            updi->device.userrow_address =    0x1300;
            updi->device.num_fuses =          11;
            break;
        }
        
        case ATTINY402:
        case ATTINY404:
        case ATTINY406:
        case ATTINY412:
        case ATTINY414:
        case ATTINY416:
        case ATTINY417:{
            updi->device.flash_start =        0x8000;
            updi->device.flash_size =         4*1024;
            updi->device.flash_pagesize =     64;
            updi->device.syscfg_address =     0x0F00;
            updi->device.nvmctrl_address =    0x1000;
            updi->device.sigrow_address =     0x1100;
            updi->device.fuses_address =      0x1280;
            updi->device.userrow_address =    0x1300;
            updi->device.num_fuses =          11;
            break;
        }        

        case ATTINY202:
        case ATTINY204:
        case ATTINY212:
        case ATTINY214:{
            updi->device.flash_start =        0x8000;
            updi->device.flash_size =         2*1024;
            updi->device.flash_pagesize =     64;
            updi->device.syscfg_address =     0x0F00;
            updi->device.nvmctrl_address =    0x1000;
            updi->device.sigrow_address =     0x1100;
            updi->device.fuses_address =      0x1280;
            updi->device.userrow_address =    0x1300;
            updi->device.num_fuses =          11;
            break;
        }        
        default: break;
    }

    return;
}

//run the updi process 
void updi_process(UPDI *updi){    

    if(!(updi->args & (UPDI_PROCESS_GET_INFO | UPDI_PROCESS_READ_FUSES | UPDI_PROCESS_WRITE_FUSES | UPDI_PROCESS_READ_FLASH | UPDI_PROCESS_ERASE | UPDI_PROCESS_WRITE_FLASH | UPDI_PROCESS_WRITE_USERROW))){
        log_important("No process args set\r\n");
        return;
    }
    
    Device device = updi->device;
    DeviceInfo *info = &(updi->info); 

    //set up serial
    Serial *serial = &(updi->serial);   
    serial->com_port = updi->com_port;
    serial->baudrate = updi->baudrate;    
    if(!serial_init(serial)){
        log_error("Could not initialise serial\r\n");
        updi_cleanup(updi);
        return;
    }

    //handshake
    send_handshake(serial);
    init(serial);
    if(!check(serial)){
        log_str("UPDI not initialised\r\n");

        if(!send_double_break(serial)){
            log_error("Double break UPDI reset failed\r\n");
            updi_cleanup(updi);
            return;
        }
        init(serial);
        if(!check(serial)){
            log_error("Cannot initialise UPDI, aborting.\r\n");
            updi_cleanup(updi);
            return;
        }else{
            log_str("UPDI INITIALISED\r\n");
        }
    }else{
        log_str("UPDI INITIALISED\r\n");
    }    


    /*
    //Inrease UPDI clock speed to allow faster baudrates, for reference only, speed difference is negligable comapard to latency in usb-art process    
    stcs(serial, UPDI_ASI_CTRLA, 0x01);
    serial_close(serial);
    if(!serial_change_baud(serial, 900000)){
        log_error("Could not increase baud rate\r\n");
        updi_cleanup(updi);
        return;
    }
    */


    //enter progmode & unlock if need be && write flash / erase set since unlocking erases   
    if(!enter_progmode(serial)){
        log_str("Couldnt enter progmode\r\n");

        if(updi->args & UPDI_PROCESS_WRITE_FLASH | updi->args & UPDI_PROCESS_ERASE){
            log_str("erasing and unlocking device\r\n");

            unlock_device(serial);

            if(in_prog_mode(serial)){
                log_str("In prog mode\r\n");                
            }else{
                log_error("Could not enter programming mode, aborting.\r\n");
                updi_cleanup(updi);
                return;
            }

        }else{
            log_error("Need to erase device to unlock. Need process args UPDI_PROCESS_ERASE or UPDI_PROCESS_WRITE_FLASH set\r\n");
            updi_cleanup(updi);                
            return;
        }        
    }else{
        log_str("IN PROG MODE\r\n");
    }
   
    //do requested actions
    if(updi->args & UPDI_PROCESS_GET_INFO){
        log_important("\r\nGETTING DEVICE INFO\r\n");
        get_device_info(serial, device, info);        
    }   

    //save fuses into updi array
    if(updi->args & UPDI_PROCESS_READ_FUSES){
        log_important("\r\nREADING FUSES\r\n");        
        
        for(uint8_t i = 0; i < updi->device.num_fuses; i++){
            uint8_t value = read_fuse(serial, device, i);            
            updi->fuse_values_read[i] = value;            
        }       
    }   

    //write fuses from updi array    
    if(updi->args & UPDI_PROCESS_WRITE_FUSES){
        log_important("\r\nWRITING FUSES\r\n");

        for(uint8_t i = 0; i < updi->device.num_fuses; i++){
            write_fuse(serial, device, i, updi->fuse_values_write[i]);            
        }
    }   

    //save flash into updi array
    if(updi->args & UPDI_PROCESS_READ_FLASH){
        log_important("\r\nREADING FLASH\r\n");

        if(!read_flash(serial, device, device.flash_start, device.flash_size, updi->flash_data_read)){
            log_error("Read flash failed\r\n");
            leave_progmode(serial);
            updi_cleanup(updi);
            return;
        }
    }   

    //Erase flash
    if(updi->args & UPDI_PROCESS_ERASE){
        log_important("\r\nERASING FLASH\r\n");

        if(!chip_erase(serial, device)){
            log_error("Chip erase failed\r\n");
            leave_progmode(serial);
            updi_cleanup(updi);
            return;
        }
    }   

    //Write flash from hex file
    if(updi->args & UPDI_PROCESS_WRITE_FLASH){
        log_important("\r\nWRITING FLASH\r\n");

        if(updi->hex_filename == NULL){
            log_error("No filename specified to flash\r\n");
            leave_progmode(serial);
            updi_cleanup(updi);
            return;
        }

        if(!chip_erase(serial, device)){
            log_error("Chip erase failed\r\n");
            leave_progmode(serial);
            updi_cleanup(updi);
            return;
        }

        //load hex, get data and start address
        uint8_t data[UPDI_MAX_FLASH_SIZE];
        uint8_t read_back[UPDI_MAX_FLASH_SIZE];
        uint16_t length = 0;

        if(!load_ihex(updi->hex_filename, data, &length)){
            log_error("Load .hex file failed\r\n");
            leave_progmode(serial);
            updi_cleanup(updi);
            return;
        }

        log_str("loaded %d bytes: \r\n", length);
        
        log_important("\r\nThis will take several minutes, dont touch anything until complete\r\n");

        if(!write_flash(serial, device, device.flash_start, data, length)){
            log_error("Writing flash failed\r\n");
            leave_progmode(serial);
            updi_cleanup(updi);
            return;
        }else{
            log_important("\r\n\r\nFlash written\r\n");
        }

        if(updi->args & UPDI_PROCESS_VERIFY_FLASH){            
            log_important("\r\nREADING FLASH\r\n");

            if(!read_flash(serial, device, device.flash_start, length + (device.flash_pagesize - length % device.flash_pagesize), updi->flash_data_read)){
                log_str("Read flash failed\r\n");
                leave_progmode(serial);
                updi_cleanup(updi);
                return;
            }

            log_important("\r\nVERIFYING FLASH\r\n");

            bool fail = false;
            //only check length of written bytes since padded bytes dont exist in original data
            for(int i = 0; i < length; i++){
                if(data[i] != updi->flash_data_read[i]){
                    fail = true;                    
                    log_str("MEM MISMATCH at addr: %d, should be: %d, received: %d\r\n", i + device.flash_start, data[i], updi->flash_data_read[i]);
                }
            }

            if(fail){
                log_error("\r\nVerify flash failed, program may or may not be ok\r\n");
            }else{
                log_important("\r\nVerify flash passed\r\n");                
            }
        } 
    }   

    //leave progmode
    leave_progmode(serial);    
    
    //Tidy up
    updi_cleanup(updi);
    log_important("Process Finished\r\n");

    return;
}

//tidy up
void updi_cleanup(UPDI *updi){
    serial_close(&(updi->serial));
    return;
}

static void send_handshake(Serial *serial){
    uint8_t buf[1] = {UPDI_BREAK};
    serial_send(serial, buf, 1);
    
    return;
}

static void init(Serial *serial){
    stcs(serial, UPDI_CS_CTRLB, 1 << UPDI_CTRLB_CCDETDIS_BIT);
    stcs(serial, UPDI_CS_CTRLA, 1 << UPDI_CTRLA_IBDLY_BIT);

    return;
}

static bool check(Serial *serial){
    if(ldcs(serial, UPDI_CS_STATUSA) != 0){
        return true;
    }else{
        return false;
    }
}

static bool send_double_break(Serial *serial){
    log_str("Sending dbl break\r\n");
    serial_close(serial);
    if(!serial_init_dbl_break(serial)){
        log_str("couldnt re-init serial port to dbl break settings\r\n");
        return false;
    }

    uint8_t buf[2] = {UPDI_BREAK, UPDI_BREAK};
    uint8_t recv[2];
    serial_send_receive(serial, buf, 2, recv, 2);

    serial_close(serial);
    if(!serial_init(serial)){
        log_str("couldnt re-init serial to normal settings\r\n");
        return false;
    }    

    return true;
}

static bool in_prog_mode(Serial *serial){
    if(ldcs(serial, UPDI_ASI_SYS_STATUS) & (1 << UPDI_ASI_SYS_STATUS_NVMPROG)){
        return true;
    }else{
        return false;
    }
}

static bool enter_progmode(Serial *serial){
    //Enter NVMProg key
    if(!in_prog_mode(serial)){
        if(!progmode_key(serial)){
            return false;
        }
    }

    //Toggle reset
    apply_reset(serial, true);
    apply_reset(serial, false);

    //Wait for unlock
    if(!wait_unlocked(serial, 100)){
        log_str("FAILED TO ENTER NVM PROGRAMMING MODE, DEVICE IS LOCKED\r\n");        
        return false;
    }

    //Check for NVMPROG flag
    if(!in_prog_mode(serial)){
        log_str("STILL NOT IN PROG MODE\r\n");        
        return false;
    }

    return true;
}

//Disables UPDI which releases any keys enabled
static void leave_progmode(Serial *serial){
    log_str("leaving progmode...\r\n");    
    apply_reset(serial, true);
    apply_reset(serial, false);

    stcs(serial, UPDI_CS_CTRLB, (1 << UPDI_CTRLB_UPDIDIS_BIT) | (1 << UPDI_CTRLB_CCDETDIS_BIT));

    return;
}

static void apply_reset(Serial *serial, bool reset){
    if(reset){
        log_str("Applying reset\r\n");        
        stcs(serial, UPDI_ASI_RESET_REQ, UPDI_RESET_REQ_VALUE);
    }else{
        log_str("Releasing reset\r\n");        
        stcs(serial, UPDI_ASI_RESET_REQ, 0x00);
    }    
}

//Unlock and erase
static bool unlock_device(Serial *serial){
    log_str("UNLOCKING AND ERASING\r\n");
    
    //enter key
    key(serial, UPDI_KEY_64, (uint8_t*)UPDI_KEY_CHIPERASE);

    //check key status
    uint8_t key_status = ldcs(serial, UPDI_ASI_KEY_STATUS);

    if(!(key_status & (1 << UPDI_ASI_KEY_STATUS_CHIPERASE))){
        log_str("Unlock error: key not accepted\r\n");        
        return false;
    }

    //Insert NVMProg key as well
    //In case of CRC being enabled, the device must be left in programming mode after the erase
    //to allow the CRC to be disabled (or flash reprogrammed)
    progmode_key(serial);

    //Toggle reset
    apply_reset(serial, true);
    apply_reset(serial, false);

    //wait for unlock
    if(!wait_unlocked(serial, 100)){
        log_str("Failed to chip erase using key\r\n");        
        return false;
    }

    log_str("UNLOCKED DEVICE\r\n");

    return true;
}

//Get device info
static void get_device_info(Serial *serial, Device device, DeviceInfo *info){
    uint8_t buf[2] = {UPDI_PHY_SYNC, UPDI_KEY | UPDI_KEY_SIB | UPDI_SIB_16BYTES};
    uint8_t recv[16];

    if(!serial_send_receive(serial, buf, 2, recv, 16)){
        log_str("SIB recv error");        
    }else{
        for(uint8_t i = 0; i < 7; i++) info->family[i] = recv[i];
        for(uint8_t i = 8; i < 11; i++) info->nvm_version[i - 8] = recv[i];
        for(uint8_t i = 11; i < 14; i++) info->ocd_version[i - 11] = recv[i];
        info->dbg_osc_freq = recv[15];
    }
    
    info->pdi_rev = ldcs(serial, UPDI_CS_STATUSA) >> 4;

    if(in_prog_mode(serial)){
        uint8_t dev_id[3];
        uint8_t dev_rev;
        read_data(serial, device.sigrow_address, 3, dev_id);
        read_data(serial, device.syscfg_address, 1, &dev_rev);

        //Add 65 to dev_rev so that 0 = A, 1 = B etc
        for(int i = 0; i < 3; i++) info->dev_id[i] = dev_id[i];
        info->dev_rev = dev_rev + 65;
    }
    
    return;
}

//Does a chip erase using the NVM controller Note that on locked devices this it not possible and the ERASE KEY has to be used instead
static bool chip_erase(Serial *serial, Device device){
    log_str("ERASING CHIP...\r\n");

    //Wait until NVM CTRL is ready to erase
    if(!wait_flash_ready(serial, device)){
        log_str("in chip_erase() error: timeout waiting for flash ready before erase\r\n");
        return false;
    }

    //Erase
    if(!execute_nvm_command(serial, device, UPDI_NVMCTRL_CTRLA_CHIP_ERASE)){
        log_str("in chip_erase() error: execute_nvm_command() failed()\r\n");
        return false;
    }

    //Wait to finish
    if(!wait_flash_ready(serial, device)){
        log_str("in chip_erase() error: timeout waiting for flash ready after erase\r\n");
        return false;
    }

    log_str("ERASED\r\n");

    return true;
}

//Reads a number of bytes of data from UPDI
static bool read_data(Serial *serial, uint16_t address, uint16_t size, uint8_t *ret){
    uint8_t recv[size];

    //Range check
    if(size > UPDI_MAX_REPEAT_SIZE + 1){
        log_str("read_data error: cant read that many bytes at once\r\n");
        return false;
    }

    //Store address pointer
    st_ptr(serial, address);

    //Set repeat
    if(size > 1){
        repeat(serial, size);
    }

    if(!ld_ptr_inc(serial, recv, size)){
        log_str("in read_data(): ld_ptr_inc error\r\n");
        return false;
    }

    for(uint16_t i = 0; i < size; i++) ret[i] = recv[i];

    return true;
}

//Reads a number of words of data from UPDI
static bool read_data_words(Serial *serial, uint16_t address, uint16_t numwords, uint8_t *buffer){
    //Range check
    if(numwords > (UPDI_MAX_REPEAT_SIZE >> 1) + 1){
        log_str("in read_data_words() error: cant write that many words in a go\r\n");
        return false;
    }

    //store address
    if(!st_ptr(serial, address)){
        log_str("in read_data_words() error: st_ptr()\r\n");
        return false;
    }

    //set up repeat
    if(numwords > 1){
        repeat(serial, numwords);
    }

    if(!ld_ptr_inc16(serial, buffer, numwords)){
        log_str("in read_data_words() error: ld_ptr_inc16()\r\n");
        return false;
    }

    return true;
}

//Reads one fuse value
static uint8_t read_fuse(Serial *serial, Device device, uint8_t fuse){
    if(!in_prog_mode(serial)){
        log_str("in read_fuse() error: not in prog mode\r\n");
        return 0;
    }

    return ld(serial, device.fuses_address + fuse);
}

//Read flash
static bool read_flash(Serial *serial, Device device, uint16_t address, uint16_t size, uint8_t *buffer){
    if(!in_prog_mode(serial)){
        log_str("in read_flash() error: not in prog mode\r\n");
        return false;
    }

    uint16_t i;
    uint16_t buf_count = 0;    
    uint16_t numwords = device.flash_pagesize; //max read size
    uint8_t recv[numwords*2];

    uint8_t p_cnt = 10;
    uint16_t chunks = (size % (numwords * 2) != 0) ? (size / (numwords * 2)) + 1 : size / (numwords * 2);

    for(i = 0; i < size / (numwords * 2); i++){
        read_data_words(serial, address + i * (numwords * 2), numwords, recv);

        for(uint16_t j = 0; j < numwords*2; j++){
            buffer[buf_count++] = recv[j];
        }

        if(100 * i / chunks > p_cnt){
            log_important("%d percent done\r\n", p_cnt);
            p_cnt += 10;
        }

    }

    if(size % (numwords * 2) != 0){ 
        read_data(serial, address + i * (numwords * 2), size % (numwords * 2), recv);
        for(int j = 0; j < size % (numwords * 2); j++){
            buffer[buf_count++] = recv[j];
        }
    }

    log_important("100 percent done\r\n");

    return true;
}

//Writes a number of bytes to memory
static bool write_data(Serial *serial, uint16_t address, uint8_t *data, uint16_t len){
    if(len == 1){
        if(!st(serial, address, data[0])){
            log_str("in write_data() error: st ret false\r\n");
            return false;
        }
    }else if(len == 2){
        if(!st(serial, address, data[0])){
            log_str("in write_data() error: st ret false\r\n");
            return false;
        }
        if(!st(serial, address + 1, data[1])){
            log_str("in write_data() error: st ret false\r\n");
            return false;
        }
    }else{
        //Range check
        if(len > UPDI_MAX_REPEAT_SIZE + 1){
            log_str("in write_data() error: invalid length\r\n");
            return false;
        }

        //store address
        if(!st_ptr(serial, address)){
            log_str("in write_data() error: couldnt st_ptr(address)\r\n");
            return false;
        }

        //set up repeat
        repeat(serial, len);
        if(!st_ptr_inc(serial, data, len)){
            log_str("in write_data() error: couldnt st_ptr_inc() error\r\n");
            return false;
        }
    }

    return true;
}

//Writes a number of words to memory
static bool write_data_words(Serial *serial, uint16_t address, uint8_t *data, uint16_t numwords){
    if(numwords == 1){
        uint16_t value = data[0] + (data[1] << 8);
        if(!st16(serial, address, value)){
            log_str("in write_data_words error: st16() error\r\n");
            return false;
        }
    }else{
        //Range check
        if(numwords > (UPDI_MAX_REPEAT_SIZE + 1)){
            log_str("in write_data_words error: invalid length\r\n");
            return false;
        }

        //Store address
        if(!st_ptr(serial, address)){
            log_str("in write_data_words error: st_ptr() error\r\n");
            return false;
        }

        //Set up repeat
        repeat(serial, numwords);
        st_ptr_inc16(serial, data, numwords);
    }

    return true;
}

//Writes one fuse value
static bool write_fuse(Serial *serial, Device device, uint8_t fuse, uint8_t value){
    uint8_t data;

    if(!in_prog_mode(serial)){
        log_str("in write_fuse() error: not in prog mode\r\n");
        return false;
    }

    if(!wait_flash_ready(serial, device)){
        log_str("in write_fuse() error: cant wait flash ready\r\n");
        return false;
    }

    data = (device.fuses_address + fuse) & 0xff;
    if(!write_data(serial, device.nvmctrl_address + UPDI_NVMCTRL_ADDRL, &data, 1)){
        log_str("in write_fuse() error: write data fail\r\n");
        return false;
    }

    data = (device.fuses_address + fuse) >> 8;
    if(!write_data(serial, device.nvmctrl_address + UPDI_NVMCTRL_ADDRH, &data, 1)){
        log_str("in write_fuse() error: write data fail\r\n");
        return false;
    }

    if(!write_data(serial, device.nvmctrl_address + UPDI_NVMCTRL_DATAL, &value, 1)){
        log_str("in write_fuse() error: write data fail\r\n");
        return false;
    }

    data = UPDI_NVMCTRL_CTRLA_WRITE_FUSE;
    if(!write_data(serial, device.nvmctrl_address + UPDI_NVMCTRL_CTRLA, &data, 1)){
        log_str("in write_fuse() error: write data fail\r\n");
        return false;
    }

    return true;
}

//Write flash memory in pages
static bool write_flash(Serial *serial, Device device, uint16_t address, uint8_t *data, uint16_t len){
    if(!in_prog_mode(serial)){
        log_str("in write_flash error: not in prog mode\r\n");        
        return false;
    }

    if(len % device.flash_pagesize > 0){
        //pad data
        log_str("PADDING DATA\r\n");
        uint8_t padded_data[len + (len % device.flash_pagesize)];
        for(uint16_t i = 0; i < len; i++){
            padded_data[i] = data[i];
        }
        for(uint16_t i = len; i < len + (device.flash_pagesize - (len % device.flash_pagesize)); i++){
            padded_data[i] = 0xFF;
        }

        //program by page        
        uint16_t numpages = (len + (device.flash_pagesize - (len % device.flash_pagesize))) / device.flash_pagesize;        
        uint8_t p_cnt = 10;

        for(uint16_t i = 0; i < numpages; i++){
            uint8_t page[device.flash_pagesize];

            for(uint16_t j = 0; j < device.flash_pagesize; j++){
                page[j] = padded_data[device.flash_pagesize * i + j];
            }

            if(!write_nvm(serial, device, address, page, device.flash_pagesize, UPDI_NVMCTRL_CTRLA_WRITE_PAGE, true)){
                log_str("Write NVM error");                        
                return false;
            }
            
            address += device.flash_pagesize;
           
            if(((i+1)*100 / numpages) > p_cnt){                
                log_important("%d percent done\r\n", p_cnt);
                p_cnt += 10;
            }
        }
    }else{
        //program by page
        uint16_t numpages = len / device.flash_pagesize;

        uint8_t p_cnt = 10;
        for(uint16_t i = 0; i < numpages; i++){
            uint8_t page[device.flash_pagesize];

            for(uint16_t j = 0; j < device.flash_pagesize; j++){
                page[j] = data[device.flash_pagesize * i + j];
            }

            if(!write_nvm(serial, device, address, page, device.flash_pagesize, UPDI_NVMCTRL_CTRLA_WRITE_PAGE, true)){
                log_str("Write NVM error");                        
                return false;
            }

            address += device.flash_pagesize;

            if(((i+1)*100 / numpages) > p_cnt){                
                log_important("%d percent done\r\n", p_cnt);
                p_cnt += 10;
            }
        }
    }

    log_important("100 percent done");   

    return true;
}

static bool load_ihex(char *filename, uint8_t *data, uint16_t *length){
    uint16_t total_data_len = 0;

    File file;
    if(!open_file(&file, filename)){
        log_error("Couldnt open file\r\n");
        return false;
    }

    char line[256];
    memset(line, 0, sizeof(line));
    while(file_read_line(&file, line, sizeof(line))){        
        //21 bytes max size we'll encounter
        uint8_t record_bin[21];
        uint8_t record_bin_index = 0;
        memset(record_bin, 0, 21);
        uint8_t data_length = 0;
        uint16_t address = 0;
        uint8_t type = 0;
        uint8_t crc = 0;

       //i = 1 to skip ':' at start of record
        for(int i = 1; i < strlen(line); i += 2){
            uint8_t bin_value = 0;
            
            if(line[i] == '\r' || line[i] == '\n' || line[i+1] == '\r' || line[i+1] == '\n') break;             
        
            if(line[i] >= '0' && line[i] <= '9'){
                bin_value += ((uint8_t)(line[i]) - 48) * 16;
            }else if(line[i] >= 'A' && line[i] <= 'F'){
                bin_value += ((uint8_t)(line[i]) - 55) * 16;
            }

            if(line[i+1] >= '0' && line[i+1] <= '9'){
                bin_value += ((uint8_t)(line[i+1]) - 48);
            }else if(line[i+1] >= 'A' && line[i+1] <= 'F'){
                bin_value += ((uint8_t)(line[i+1]) - 55);
            }

            record_bin[record_bin_index++] = bin_value;
        }

        data_length = record_bin[0];
        total_data_len += data_length;
        address = record_bin[1] * 256 + record_bin[2];
        type = record_bin[3];

        if(type == 0){
            //data
            for(int i = 0; i < data_length; i++){
                data[address++] = record_bin[4+i];
            }
        }else if(type == 1){
            //End of records
            log_str("End of records reached\r\n");
        }else{
            log_error("load_ihex() error, unsupported hex file\r\n");
            return false;
        }
    }
    
    close_file(&file);

    *length = total_data_len;

    return true;
}

//Load data from Control/Status space
static uint8_t ldcs(Serial *serial, uint8_t address){

    uint8_t buf[2] = {UPDI_PHY_SYNC, (uint8_t)(UPDI_LDCS | (address & 0x0F))};
    uint8_t recv[1] = {0};

    if(!serial_send_receive(serial, buf, 2, recv, 1)){
        log_str("ldcs error\r\n");        
        return 0;
    }else{
        return recv[0];
    }
}

//Load a single byte direct from a 16-bit address
static uint8_t ld(Serial *serial, uint16_t address){

    uint8_t buf[4] = {UPDI_PHY_SYNC, UPDI_LDS | UPDI_ADDRESS_16 | UPDI_DATA_8, (uint8_t)(address & 0xFF), (uint8_t)((address >> 8) & 0xFF)};
    uint8_t recv[1] = {0};

    if(!serial_send_receive(serial, buf, 4, recv, 1)){
        log_str("ld error\r\n");
        return 0;
    }else{
        return recv[0];
    }
}

//Load a 16-bit word directly from a 16-bit address
static bool ld16(Serial *serial, uint16_t address, uint16_t *word){

    uint8_t buf[4] = {UPDI_PHY_SYNC, UPDI_LDS | UPDI_ADDRESS_16 | UPDI_DATA_16, (uint8_t)(address & 0xFF), (uint8_t)((address >> 8) & 0xFF)};
    uint8_t recv[2] = {0, 0};

    if(!serial_send_receive(serial, buf, 4, recv, 2)){
        log_str("ld16 error\r\n");
        return false;
    }

    *word  = (recv[0] << 8) | recv[1];

    return true;
}

//Loads a number of bytes from the pointer location with pointer post-increment
static bool ld_ptr_inc(Serial *serial, uint8_t *buffer, uint16_t size){
    uint8_t buf[2] = {UPDI_PHY_SYNC, UPDI_LD | UPDI_PTR_INC | UPDI_DATA_8};
    uint8_t recv[size];

    if(!serial_send_receive(serial, buf, 2, recv, size)){
        log_str("in ld_ptr_inc(): error\r\n");
        return false;
    }

    for(uint16_t i = 0; i < size; i++){
        buffer[i] = recv[i];
    }

    return true;
}

//Load a 16-bit word value from the pointer location with pointer post-increment
static bool ld_ptr_inc16(Serial *serial, uint8_t *buffer, uint16_t numwords){
    uint8_t buf[2] = {UPDI_PHY_SYNC, UPDI_LD | UPDI_PTR_INC | UPDI_DATA_16};
    uint8_t recv[numwords << 1];

    if(!serial_send_receive(serial, buf, 2, recv, numwords << 1)){
        log_str("ld_ptr_inc16 error\r\n");
        return false;
    }

    for(uint16_t i = 0; i < numwords << 1; i++){
        buffer[i] = recv[i];
    }

    return true;
}

//Store a value to Control/Status space
static void stcs(Serial *serial, uint8_t address, uint8_t value){
    uint8_t buf[3] = {UPDI_PHY_SYNC, (uint8_t)(UPDI_STCS | (address & 0x0F)), value};

    serial_send(serial, buf, 3);

    return;
}

//Store a single byte value directly to a 16-bit address
static bool st(Serial *serial, uint16_t address, uint8_t value){
    uint8_t buf[4] = {UPDI_PHY_SYNC, UPDI_STS | UPDI_ADDRESS_16 | UPDI_DATA_8, (uint8_t)(address & 0xFF), (uint8_t)((address >> 8) & 0xFF)};
    uint8_t recv[1] = {0};

    if(!serial_send_receive(serial, buf, 4, recv, 1)){
        log_str("ST error sending address");        
        return false;
    }else{
        if(recv[0] != UPDI_PHY_ACK){
            log_str("error: st, no ACK from sent address\r\n");
            return false;
        }
    }

    buf[0] = value & 0xFF;

    if(!serial_send_receive(serial, buf, 1, recv, 1)){
        log_str("st error sending valyue\r\n");
        return false;
    }else{
        if(recv[0] != UPDI_PHY_ACK){
            log_str("error: st, no ACK after value sent\r\n");
            return false;
        }
    }

    return true;
}

//Store a 16-bit word value directly to a 16-bit address
static bool st16(Serial *serial, uint16_t address, uint16_t value){
    uint8_t buf[4] = {UPDI_PHY_SYNC, UPDI_STS | UPDI_ADDRESS_16 | UPDI_DATA_16, (uint8_t)(address & 0xFF), (uint8_t)((address >> 8) & 0xFF)};
    uint8_t recv[1] = {0};

    if(!serial_send_receive(serial, buf, 4, recv, 1)){
        log_str("st16 error\r\n");
        return false;
    }else{
        if(recv[0] != UPDI_PHY_ACK){
            log_str("error: st16, no ACK\r\n");
            return false;
        }
    }

    buf[0] = (uint8_t)(value & 0xFF);
    buf[1] = (uint8_t)((value >> 8) & 0xFF);

    if(!serial_send_receive(serial, buf, 2, recv, 1)){
        log_str("st16 error\r\n");
        return false;
    }else{
        if(recv[0] != UPDI_PHY_ACK){
            log_str("error: st16, no ACK to value\r\n");
            return false;
        }
    }
    return true;
}

//Set the pointer location
static bool st_ptr(Serial *serial, uint16_t address){
    uint8_t buf[4] = {UPDI_PHY_SYNC, UPDI_ST | UPDI_PTR_ADDRESS | UPDI_DATA_16, (uint8_t)(address & 0xFF), (uint8_t)((address >> 8) & 0xFF)};
    uint8_t recv[1] = {0};

    if(!serial_send_receive(serial, buf, 4, recv, 1)){
        log_str("st ptr error\r\n");
        return false;
    }else{
        if(recv[0] != UPDI_PHY_ACK){
            log_str("error: st_ptr no ACK\r\n");
            return false;
        }
    }

    return true;
}

//Store data to the pointer location with pointer post-increment
static bool st_ptr_inc(Serial *serial, uint8_t *data, uint16_t size){
    uint8_t buf[3] = {UPDI_PHY_SYNC, UPDI_ST | UPDI_PTR_INC | UPDI_DATA_8, data[0]};
    uint8_t recv[1];

    if(!serial_send_receive(serial, buf, 3, recv, 1)){
        log_str("error st_ptr_inc\r\n");
        return false;
    }else{
        if(recv[0] != UPDI_PHY_ACK){
            log_str("error: no ACK with st_ptr_inc");
            return false;
        }
    }

    for(uint16_t i = 1; i < size; i++){
        recv[0] = 0;

        if(!serial_send_receive(serial, data+i, 1, recv, 1)){
            log_str("st_ptr_inc error\r\n");
            return false;
        }else{
            if(recv[0] != UPDI_PHY_ACK){
                log_str("error: no ACK with st_ptr_inc");
                return false;
            }
        }

    }

    return true;
}

//Store a 16-bit word value to the pointer location with pointer post-increment. Disable acks when we do this, to reduce latency.
static void st_ptr_inc16(Serial *serial, uint8_t *data, uint16_t numwords){
    uint8_t buf[2] = {UPDI_PHY_SYNC, UPDI_ST | UPDI_PTR_INC | UPDI_DATA_16};

    uint8_t ctrla_ackon = 1 << UPDI_CTRLA_IBDLY_BIT;
    uint8_t ctrla_ackoff = ctrla_ackon | (1 << UPDI_CTRLA_RSD_BIT);

    //disable acks
    stcs(serial, UPDI_CS_CTRLA, ctrla_ackoff);

    serial_send(serial, buf, 2); //no response expected
    serial_send(serial, data, numwords << 1);

    //reenable acks
    stcs(serial, UPDI_CS_CTRLA, ctrla_ackon);

    return;
}

//Store a value to the repeat counter
static void repeat(Serial *serial, uint16_t repeats){
    repeats -= 1;

    uint8_t buf[4] = {UPDI_PHY_SYNC, UPDI_REPEAT | UPDI_REPEAT_WORD, (uint8_t)(repeats & 0xFF), (uint8_t)((repeats >> 8) & 0xFF)};    
    
    serial_send(serial, buf, 4);

    return;
}

//Write a key
static void key(Serial *serial, uint8_t size, uint8_t *key){
    uint16_t len = 8 << size;
    uint8_t buf[2] = {UPDI_PHY_SYNC, (uint8_t)(UPDI_KEY | UPDI_KEY_KEY | size)};
    uint8_t key_reversed[len];

    for(uint16_t i = 0; i < len; i++){
        key_reversed[i] = key[len - 1 - i];
    }

    serial_send(serial, buf, 2);
    serial_send(serial, key_reversed, 8 << size);

    return;
}

//Inserts the NVMProg key and checks that its accepted
static bool progmode_key(Serial *serial){    
    key(serial, UPDI_KEY_64, (uint8_t*)UPDI_KEY_NVM);

    uint8_t key_status = ldcs(serial, UPDI_ASI_KEY_STATUS);

    if(!(key_status & (1 << UPDI_ASI_KEY_STATUS_NVMPROG))){        
        return false;
    }

    return true;
}

//Waits for the device to be unlocked. All devices boot up as locked until proven otherwise
static bool wait_unlocked(Serial *serial, uint16_t timeout){    
    unsigned long int start = millis();
    
    while(millis() - start < timeout){
        if(!(ldcs(serial, UPDI_ASI_SYS_STATUS) & (1 << UPDI_ASI_SYS_STATUS_LOCKSTATUS))){
            return true;
        }
    }

    log_str("TIMEOUT WAITING FOR DEVICE TO UNLOCK\r\n");    

    return false;
}

//Waits for the NVM controller to be ready
static bool wait_flash_ready(Serial *serial, Device device){
    unsigned long int start = millis();

    while(millis() - start < 10000){
        uint8_t status = ld(serial, device.nvmctrl_address + UPDI_NVMCTRL_STATUS);
        if (status & (1 << UPDI_NVM_STATUS_WRITE_ERROR)){
            log_str("in wait_flash_ready() error: nvm error\r\n");
            return false;
        }

        if(!(status & ((1 << UPDI_NVM_STATUS_EEPROM_BUSY) | (1 << UPDI_NVM_STATUS_FLASH_BUSY)))){
            return true;
        }
    }

    log_str("in wait_flash_ready() error: wait flash ready timed out\r\n");
    return false;
}

//Execute NVM command
static bool execute_nvm_command(Serial *serial, Device device, uint8_t command){
    if(!st(serial, device.nvmctrl_address + UPDI_NVMCTRL_CTRLA, command)){
        log_str("in execute_nvm_command() error: st() false return\r\n");
        return false;
    }
    return true;
}

//Writes a page of data to NVM. By default the PAGE_WRITE command is used, which requires that the page is already erased. By default word access is used (flash)
static bool write_nvm(Serial *serial, Device device, uint16_t address, uint8_t *data, uint16_t len, uint8_t command, bool use_word_acess){
    //wait for NVM controller to be ready
    if(!wait_flash_ready(serial, device)){
        log_str("in write_nvm() error: cant wait flash ready\r\n");        
        return false;
    }

    //Clear page buffer
    if(!execute_nvm_command(serial, device, UPDI_NVMCTRL_CTRLA_PAGE_BUFFER_CLR)){
        log_str("in write_nvm() error: execute nvm command\r\n");                
        return false;
    }

    //wait for NVM controller to be ready
    if(!wait_flash_ready(serial, device)){
        log_str("in write_nvm() error: cant wait flash ready after page buffer clear\r\n");        
        return false;
    }

    //Load the page buffer by writing directly to location
    if(use_word_acess){
        if(!write_data_words(serial, address, data, len >> 1)){
            log_str("in write_nvm() error: write_data_words() error\r\n");            
            return false;
        }
    }else{
        if(!write_data(serial, address, data, len)){
            log_str("in write_nvm() error: write_data() error\r\n");            
            return false;
        }
    }

    //Write the page to NVM, maybe erase first
    if(!execute_nvm_command(serial, device, command)){
        log_str("in write_nvm() error: execute_nvm_command() error committing page\r\n");        
        return false;
    }

    //wait for NVM controller to be ready
    if(!wait_flash_ready(serial, device)){
        log_str("in write_nvm() error: cant wait flash ready after commit page\r\n");        
        return false;
    }

    return true;
}