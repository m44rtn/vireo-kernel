/*
MIT license
Copyright (c) 2019-2021 Maarten Vermeulen

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

#include "iso9660.h"

#include "../FS_TYPES.H"

#include "../COMMANDS.H"
#include "../FS_commands.h"

#include "../../include/types.h"
#include "../../include/exit_code.h"

#include "../../dbg/dbg.h"

#include "../../hardware/driver.h"

#include "../../screen/screen_basic.h"

#include "../../memory/memory.h"
#include "../../memory/paging.h"

#include "../../dsk/diskio.h"
#include "../../dsk/diskdefines.h"

#include "../../util/util.h"

// =========================================

#define SECTOR_SIZE				2048
#define CD_INFO_SIZE			512
#define VOL_IDENT_SIZE			32

#define FIRST_DESCRIPTOR_LBA	0x10

// Descriptor types
#define VD_TYPE_PRIMARY			0x01
#define VD_TYPE_TERMINATOR		0xFF // I'll be back

// offsets within the Primary Volume Descriptor
#define PVD_VOL_IDENT           40
#define PVD_VOL_SIZE            80
#define PVD_BLOCK_SIZE          128
#define PVD_PATHTABLE_SIZE      132
#define PVD_PATHTABLE_LBA       140
#define PVD_ROOTDIR_ENTRY       156

// supported file flags
#define FF_HIDDEN               1 << 0
#define FF_DIRECTORY            1 << 1
#define FF_NOT_FINAL_DIR        1 << 7

typedef struct 
{
    uint8_t ident_len;  // dir name len
    uint8_t ext_AR_len; // extended attribute record length
    uint32_t lba;
    uint16_t parent;

    // [directory name here (variable length)]
} __attribute__((packed)) pathtable_t;

typedef struct
{
    uint8_t DR_len; // length of directory record
    uint8_t EAR_len; // extended attribute record length
    uint32_t lba_extend;
    uint32_t lba_extendMSB; // unused
    uint32_t size;          // extend size
    uint32_t sizeMSB;       // same as above, not used
    uint8_t datetime[7];    // date and time thing
    uint8_t file_flags;
    uint8_t interleavedsize; // ????
    uint8_t interleavedgap;
    uint16_t volseqnr;      // volume sequence number
    uint16_t volseqnrMSB;   // unused
    uint8_t ident_len;      // length of file name
    
    // [directory name here (variable length)]
} __attribute__((packed)) direntry_t;

typedef struct
{
	uint32_t path_table_lba;
	uint32_t path_table_size;
	uint32_t rootdir_lba;

	char volident[VOL_IDENT_SIZE+1]; // volume name
	uint32_t vol_size;	// in blocks of 2048 bytes
} __attribute__((packed)) cd_info_t;


/* the indentifier for drivers + information about our driver */
struct DRIVER ISO_driver_id = {(uint32_t) 0xB14D05, "VIREODRV", (FS_TYPE_ISO | DRIVER_TYPE_FS), (uint32_t) (iso_handler)};

uint32_t atapi_devices  = 0;
uint8_t n_atapi_devs    = 0;

uint32_t *cd_info_ptr 	= NULL;

uint8_t gerror;

void iso_handler(uint32_t * drv)
{
	gerror = 0;

    switch(drv[0])
    {
        case DRV_COMMAND_INIT:
            iso_init((uint8_t) drv[1]);
        break;
    }

	drv[4] = gerror;
    
}

void iso_init(uint8_t drive)
{    
	// do we know this drive already?
	if(atapi_devices >> drive)
		return;

	// allocate a cd info buffer if we don't have one yet
	if(cd_info_ptr == NULL)
		cd_info_ptr = iso_allocate_bfr(CD_INFO_SIZE);

	uint8_t *buffer = (uint8_t *) iso_allocate_bfr(SECTOR_SIZE);

	if(gerror)
		// oh no! something went wrong with the allocation of memory (buffer / cd_info)
		return; 

	// search and read the primary vol. desc.
	search_descriptor(drive, buffer, VD_TYPE_PRIMARY);
	dbg_assert(buffer[0] == VD_TYPE_PRIMARY);

	// save all interesting data
	save_pvd_data(buffer, drive);

	if(n_atapi_devs < IDE_DRIVER_MAX_DRIVES)
		atapi_devices |= (1 << drive);

	// free buffer space with the right function depending on the type
	if(buffer > memory_get_malloc_end())
		vfree(buffer);
	else
		kfree(buffer);

	#ifndef NO_DEBUG_INFO
		cd_info_t * info = (cd_info_t *) cd_info_ptr;
		trace("[ISO9660 DRIVER] Vol. ident.: %s\n", (uint32_t) &(info->volident));
		trace("[ISO9660 DRIVER] Vol. size (in 2048 byte blocks): %i\n", (uint32_t) &(info->vol_size));
		trace("[ISO9660 DRIVER] Path table size: %i\n", (uint32_t) &(info->path_table_size));
		trace("[ISO9660 DRIVER] Path tanle lba: %i\n", (uint32_t) &(info->path_table_lba));
		trace("[ISO9660 DRIVER] Rootdir lba: %i\n", (uint32_t) &(info->rootdir_lba));
	#endif
	
}

void search_descriptor(uint8_t drive, uint8_t * buffer, uint8_t type)
{
	uint32_t lba = FIRST_DESCRIPTOR_LBA;

	while(1)
	{
		// read disk
		uint8_t error = read(drive, lba, 0x01, (uint16_t *) buffer);

		dbg_assert(!error); // asserts when drive is out of range

		if((buffer[0] == type) || (buffer[0] == VD_TYPE_TERMINATOR))
			break;

		++lba;

		dbg_assert(lba != 0xFF);	// we don't want to read the whole CD until we 'find' what we
									// are looking for
	}
}

void save_pvd_data(uint8_t * pvd, uint8_t drive)
{
	// since I don't want to use 882 bytes of my precious kernel space
    // for stuff I'm probably never going to use, I'm going to use pointers 
    // and array indexes for searching all the information I need (this is a warning
    // because it may get messy)

	cd_info_t * info = (cd_info_t *) cd_info_ptr;
	uint32_t * dword;
	uint16_t * word;

	// save volume identifier
	memcpy(&(info->volident), &pvd[PVD_VOL_IDENT], VOL_IDENT_SIZE);
	info->volident[VOL_IDENT_SIZE] = '\0';

	// save size of volume (in blocks of 2048)
	dword = (uint32_t *) &pvd[PVD_VOL_SIZE];
	info->vol_size = *(dword);

	// check if sector size is equal to standard sector size (this driver does not
	// support ISO's that deviate from that sector size)
	word = (uint16_t *) &pvd[PVD_BLOCK_SIZE];
	dbg_assert(*(word) == SECTOR_SIZE);

	// store path table size and lba
	dword = (uint32_t *) &pvd[PVD_PATHTABLE_SIZE];
	info->path_table_size = *(dword);

	dword = (uint32_t *) &pvd[PVD_PATHTABLE_LBA];
	info->path_table_lba = *(dword);

	// store root dir lba
	direntry_t *root = (direntry_t *) &pvd[PVD_ROOTDIR_ENTRY];
	info->rootdir_lba = (root->lba_extend);
}

uint32_t * iso_allocate_bfr(size_t size)
{
	uint32_t * ptr = kmalloc(size);

	if(ptr)
		return ptr;

	// no kernel memory left, so let's use a page
	PAGE_REQ req;
	req.attr = PAGE_REQ_ATTR_READ_WRITE | PAGE_REQ_ATTR_SUPERVISOR;
	req.pid  = PID_KERNEL;
	req.size = size;

	ptr = valloc(&req);

	if(!ptr)
		gerror = EXIT_CODE_OUT_OF_MEMORY;
	
	return ptr;
}

void iso_read(void)
{
    // use path table
}

