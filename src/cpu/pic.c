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

#include "pic.h"

#include "../include/types.h"

#include "../io/io.h"

#define PIC_MASTER_CMNDSTAT     0x20
#define PIC_MASTER_IMRDATA      0x21

#define PIC_SLAVE_CMNDSTAT      0xA0
#define PIC_SLAVE_IMRDATA       0xA1

#define PIC_READ_ISR            0x0B

/* does not return an exit code (for now) */
void PIC_controller_setup(void)
{
    /* basically, this only remaps the PICs.
    order: ICW1, ICW2, ICW3 and ICW4.
    IOWAIT() is there for older hardware
    */

    ASM_OUTB(PIC_MASTER_CMNDSTAT, 0x11);
    ASM_IOWAIT();
    ASM_OUTB(PIC_SLAVE_CMNDSTAT,  0x11);
    
    ASM_IOWAIT();

    ASM_OUTB(PIC_MASTER_IMRDATA, 0x20);
    ASM_IOWAIT();
    ASM_OUTB(PIC_SLAVE_IMRDATA,  0x28);

    ASM_IOWAIT();

    ASM_OUTB(PIC_MASTER_IMRDATA, 0x04);
    ASM_IOWAIT();
    ASM_OUTB(PIC_SLAVE_IMRDATA,  0x02);

    ASM_IOWAIT();

    ASM_OUTB(PIC_MASTER_IMRDATA, 0x01);
    ASM_IOWAIT();
    ASM_OUTB(PIC_SLAVE_IMRDATA,  0x01);

    ASM_IOWAIT();
    ASM_OUTB(PIC_MASTER_IMRDATA, 0x00);
    ASM_IOWAIT();
    ASM_OUTB(PIC_SLAVE_IMRDATA,  0x00);
}

void PIC_mask(unsigned char IRQ)
{
    uint8_t mask;
    uint16_t port = PIC_MASTER_IMRDATA;
    
    if(IRQ > 7)
    {
        IRQ = IRQ - 8;
        port = PIC_SLAVE_IMRDATA;
    }

    mask = ASM_INB(port) | (1 << IRQ);
    ASM_OUTB(port, mask);
}

void PIC_umask(unsigned char IRQ)
{
    uint8_t mask;
    uint16_t port = PIC_MASTER_IMRDATA;
    
    if(IRQ > 7)
    {
        IRQ = IRQ - 8;
        port = PIC_SLAVE_IMRDATA;
    }

    mask = ASM_INB(port) & ~(1 << IRQ);
    ASM_OUTB(port, mask);
}

void PIC_EOI(unsigned char IRQ)
{
    uint16_t port = PIC_MASTER_IMRDATA;
    
    if(IRQ > 7)
        port = PIC_SLAVE_IMRDATA;

    ASM_OUTB(port, 0x20);
}

void PIC_read_ISR(void)
{
    ASM_OUTB(PIC_MASTER_CMNDSTAT, PIC_READ_ISR);
    ASM_OUTB(PIC_SLAVE_CMNDSTAT,  PIC_READ_ISR);

    return (ASM_INB(PIC_SLAVE_CMNDSTAT) << 8) | ASM_INB(PIC_MASTER_CMNDSTAT);    
}