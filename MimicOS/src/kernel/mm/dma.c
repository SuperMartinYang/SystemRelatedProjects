#include <kernel/mm/dma.h>
#include <kernel/io/port.h>
#include <kernel/interrupt.h>


BYTE dma_maskreg[8]     = { 0x0A, 0x0A, 0x0A, 0x0A, 0xD4, 0xD4, 0xD4, 0xD4 };
BYTE dma_modereg[8]     = { 0x0B, 0x0B, 0x0B, 0x0B, 0xD6, 0xD6, 0xD6, 0xD6 };
BYTE dma_clearreg[8]    = { 0x0C, 0x0C, 0x0C, 0x0C, 0xD8, 0xD8, 0xD8, 0xD8 };

// different port (BYTE) accept different data for different usage

// the page in memory which we are accessing
BYTE dma_pageport[8]    = { 0x87, 0x83, 0x81, 0x82, 0x8F, 0x8B, 0x89, 0x8A };
// the address port for data address
BYTE dma_addressport[8] = { 0x00, 0x02, 0x04, 0x06, 0xC0, 0xC4, 0xC8, 0xCC };
// the count port for the amount of data
BYTE dma_countport[8]   = { 0x01, 0x03, 0x05, 0x07, 0xC2, 0xC6, 0xCA, 0xCE };

// memory used by 8bit DMA (channels 0-3) may not cross 64K boundary
// memory used by 16bit DMA (channels 4-7) may not cross 128K boundary

void dma_transfer(BYTE channel, void * address, DWORD length, struct MODE mode){
    BYTE page;
    DWORD offset;
    // calculate the page
    page = (((DWORD)address) >> 16);        // e.g. 0x00010000  -> page number = 0x000001
    // and the offset   
    offset = (((DWORD)address) & 0xFFFF);   // e.g. 0x00012345  -> offset = 0x2345
    length--;
    
    // if channel = 1, after mask 0x0A (the real channel), 0x04 | 0x01 = 0x05 ( make 8bit DMA same as 16bit DMA)
    port_outb(dma_maskreg[channel], 0x04 | channel);    
    // clear the byte pointer flip flop
    port_outb(dma_clearreg[channel], 0x00);
    // set the transfer mode
    port_outb(dma_modereg[channel], mode.data);
    // set the address offset
    port_outb(dma_addressport[channel], (offset & 0x00FF));
    port_outb(dma_addressport[channel], ((offset & 0xFF00) >> 8));
    // set the page, page range [0 ... 0xFF]
    port_outb(dma_pageport[channel], page);
    // set the length
    port_outb(dma_countport[channel], (length & 0x00FF));
    port_outb(dma_countport[channel], ((length & 0xFF00) >> 8));
    // mask channel
    port_outb(dma_maskreg[channel], channel);
}

void dma_read(BYTE channel, void * address, DWORD length){
    struct MODE mode;
    // disable all other interrupts for reading bytes from dma
    interrupt_disableAll();
    // clear all the data first;
    mode.data = 0x00;
    mode.bits.modeselect = DMA_SINGLE;
    mode.bits.transfertype = DMA_READ;
    mode.bits.channel = channel;

    // perform transfer
    dma_transfer(channel, address, length, mode);
    //enable interrupts now
    interrupt_enableAll();
}

void dma_write(BYTE channel, void * address, DWORD length){
    struct MODE mode;

    interrupt_disableAll();

    mode.data = 0x00;
    mode.bits.modeselect = DMA_SINGLE;
    mode.bits.channel = channel;

    dma_transfer(channel, address, length, mode);

    interrupt_enableAll();
}