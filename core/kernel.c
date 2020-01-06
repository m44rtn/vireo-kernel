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

#include "kernel.h"

#include "include/kernel_info.h"
#include "include/asm_functions.h"
#include "include/global_exit_codes.h"
#include "include/global_flags.h"
#include "include/types.h"

#include "boot/loader.h"

#include "io/io.h"

#include "screen/screen_basic.h"

#include "util/util.h"


#include "cpu/gdt.h"
#include "cpu/interrupts/IDT.h"
#include "cpu/pic.h"
#include "cpu/cpu.h"

#include "memory/memory.h"
#include "memory/paging.h"

#include "hardware/pci.h"

#include "dbg/dbg.h"


/* remove */
#include "hardware/driver.h"

void init_env(void);
void main(void);

typedef struct 
{
    uint32_t sign1;
    char *sign2;
} __attribute__((packed)) tester;


/* initializes 'the environment' */
void init_env(void)
{
     /* GDT structures */
    GDT_ACCESS access;
    GDT_FLAGS flags;
    uint8_t exit_code;

    exit_code = loader_detect();
    if(exit_code == EXIT_CODE_GLOBAL_NOT_IMPLEMENTED) 
        debug_print_warning((char *) "Support for current bootloader not implemented");

    /* setup GDT structures */
    access.dataisWritable   = true;
    access.codeisReadable   = true;

    flags.Align4k           = true;
    flags.use16             = false;
    
    GDT_setup(access, flags);
    PIC_controller_setup();

    IDT_setup();
    CPU_init();
    
    exit_code = memory_init();

    paging_init();

    pci_init();

    driver_init();
}

void main(void)
{
    unsigned int exit_code = 0;
    
    exit_code = screen_basic_init();
   
    if(exit_code != EXIT_CODE_GLOBAL_SUCCESS) 
        while(1);
    
    init_env();
    
   /* uint32_t *place = memsrch((void *) &test, 12, 0x10000, 0x3e4dbff);

    trace("interface: 0x%x\n", (uint32_t) hello_world);
    trace("place: 0x%x\n", place);
    trace("functionloc: 0x%x\n", (uint32_t)place | 12);

    ASM_CALL_FUNC(*(uint32_t *)((uint32_t)place | 12));*/

    #ifndef QUIET_KERNEL /* you can put this define in types.h and it'll have effect on all the modules */
    trace((char *) "[KERNEL] Vireo II build %i\n\n", BUILD);
    #endif
            
    while(1);
}
