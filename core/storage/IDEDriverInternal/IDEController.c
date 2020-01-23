/*
MIT license
Copyright (c) 2020 Maarten Vermeulen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "IDEController.h"

#include "../IDE_commands.h"

#include "../../include/types.h"

#ifndef QUIET_KERNEL
    #include "../../screen/screen_basic.h"
#endif

#include "../../hardware/driver.h"

#include "../../memory/memory.h"

#include "../../io/io.h"

#include "../../dbg/dbg.h"

#include "../../util/util.h"

#define IDEController_PCI_CLASS_SUBCLASS    0x101

#define ATA_PRIMARY_DATA      0x1F0
#define ATA_SECONDARY_DATA    0x170

#define ATA_PORT_PRIMARY_CONTROL    0x3F6
#define ATA_PORT_SECONDARY_CONTROL  0x376

#define ATA_PORT_FEATURES   0x01
#define ATA_PORT_SCTRCNT    0x02
#define ATA_PORT_LBALOW     0x03
#define ATA_PORT_LBAMID     0x04
#define ATA_PORT_LBAHI      0x05
#define ATA_PORT_SELECT     0x06
#define ATA_PORT_COMSTAT    0x07 //command and status port

#define IDE_DRIVER_MAX_DRIVES   4

#define IDE_DRIVER_TYPE_PATA    0x00
#define IDE_DRIVER_TYPE_PATAPI  0x01
#define IDE_DRIVER_TYPE_UNKNOWN 0xFF


typedef struct
{
    uint8_t type;
    /* there'll be more here, probably */
} DRIVE_INFO;


static void IDEDriverInit(unsigned int device);
static void IDE_software_reset(uint16_t port);
static uint8_t IDE_getDriveType(uint32_t port);


DRIVE_INFO drive_info_t[IDE_DRIVER_MAX_DRIVES];
uint32_t PCI_controller;

/* the indentifier for drivers + information about our driver */
struct DRIVER driver_id = {(uint32_t) 0xB14D05, "VIREODRV", (IDEController_PCI_CLASS_SUBCLASS | DRIVER_TYPE_PCI), (uint32_t) (IDEController_handler)};


void IDEController_handler(uint32_t *data)
{
    DRIVER_PACKET *drv = (DRIVER_PACKET *) data;

    switch(drv->command)
    {
        case IDE_COMMAND_INIT:
            IDEDriverInit(drv->parameter1);
        break;

        case IDE_COMMAND_READ:
        break;

        default:
            dbg_assert(0);
        break;
    }
}


static void IDEDriverInit(uint32_t device)
{
    /* technically, you should first enumerate the PCI bus but it's not reliable and 
    most controllers support the standard IO ports at boot up anyway. */
    uint8_t drive, slavebit;
    uint16_t port;

    #ifndef QUIET_KERNEL
    print((char *) "[IDE_DRIVER] Vireo Internal PIO IDE/ATA Driver Mark I\n");
    trace((char *) "[IDE_DRIVER] Kernel reported PCI controller %x\n", device);
    #endif

    PCI_controller = device;
    
    IDE_software_reset(ATA_PORT_PRIMARY_CONTROL);
    IDE_software_reset(ATA_PORT_SECONDARY_CONTROL);

    /* get the drive types */
    for(drive = 0; drive < IDE_DRIVER_MAX_DRIVES; ++drive)
    {
        port = (drive > 1) ? ATA_SECONDARY_DATA : ATA_PRIMARY_DATA;
        slavebit = (drive > 1) ? (uint8_t) (drive - 2) : drive;

        ASM_OUTB((uint32_t) (port | ATA_PORT_SELECT), (uint32_t) (0xA0 | slavebit << 4));

        /* TODO: 
            - cleanup 
            - ATAPI READ
            - ATA READ/WRITE?*/
        sleep(1);

        drive_info_t[drive].type = IDE_getDriveType(port);

        trace("[IDE_DRIVER] found drive type: %x\n", drive_info_t[drive].type);
    }
    
    #ifndef QUIET_KERNEL
        print((char *) "\n");
    #endif
}


static void IDE_software_reset(uint16_t port){
    uint8_t value = (uint8_t) ASM_INB(port);
    ASM_OUTB(port, (value | 0x04));
  
    sleep(1);

    /* unset the reset bit */
    value = (uint8_t) ASM_INB(port);
    ASM_OUTB(port, (value & 0xFFFB));
}

static uint8_t IDE_getDriveType(uint32_t port)
{
    /* expects the ATA_PORT command to be already send to the port */

    /* use IDENTIFY */    
    uint16_t lo = ASM_INB((uint16_t) (port | ATA_PORT_LBAMID));
    uint16_t hi = ASM_INB((uint16_t) (port | ATA_PORT_LBAHI));

    if(hi == 0xEB && lo == 0x14) return (uint8_t) IDE_DRIVER_TYPE_PATAPI;
    else if(hi == 0 && lo == 0) return (uint8_t) IDE_DRIVER_TYPE_PATA;
    else return (uint8_t) IDE_DRIVER_TYPE_UNKNOWN;
}