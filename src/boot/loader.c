/*
MIT license
Copyright (c) 2019 Maarten Vermeulen

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

#include "loader.h"

#include "multiboot.h"

#include "../include/types.h"
#include "../screen/screen_basic.h"

#define LOADER_MAGICNUMBER_MULTIBOOT    0x2BADB002

static void loader_multiboot_compliant(void);

extern const uint32_t MAGICNUMBER;
extern const uint32_t *BOOTLOADER_STRUCT_ADDR;

static uint8_t loader_type = LOADER_TYPE_UNKNOWN;

void loader_detect(void)
{
    /* there'll be more supported loaders in the future so that's why we have this here */

    if(MAGICNUMBER == LOADER_MAGICNUMBER_MULTIBOOT)
        loader_multiboot_compliant();    
    
}

uint8_t loader_get_type()
{
    return loader_type;
}

uint32_t *loader_get_infoStruct()
{
    if(loader_type == LOADER_TYPE_MULTIBOOT)
        return BOOTLOADER_STRUCT_ADDR;

    return NULL;
}

static void loader_multiboot_compliant(void)
{
    multiboot_info_t *info = (multiboot_info_t *) BOOTLOADER_STRUCT_ADDR;
    char *bootloader_name = (char *) info->boot_loader_name;

    print((char *) "[LOADER] Reports multiboot compliant\n");
    trace((char *) "[LOADER] Loaded by %s\n\n", (unsigned int) bootloader_name);

    loader_type   = LOADER_TYPE_MULTIBOOT;
}


