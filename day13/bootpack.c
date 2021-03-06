#include<stdio.h>
#include "bootpack.h"

void putfonts8_asc_sht(Sheet *sht, int x, int y, int c, int b, char* s, int l);

void HariMain(void) {
    Bootinfo binfo = (Bootinfo) ADR_BOOTINFO;
    Fifo32 fifo;
    char s[40];//, keybuf[32], mousebuf[128], timerbuf[8], timerbuf2[8], timerbuf3[8];
    int fifobuf[128];
    Timer *timer, *timer2, *timer3;
    int mx, my, i, count;
    unsigned int memtotal;
    Mouse_dec mdec;
    Memman *man = (Memman *) MEMMAN_ADDR;
    ShtCtl *shtctl;
    Sheet *sht_back, *sht_mouse, *sht_win;
    unsigned char *buf_back, buf_mouse[256], *buf_win;

    // 初始化IDT GDT
    init_gdtidt();
    init_pic();
    io_sti();
    fifo32_init(&fifo, 128, fifobuf);
    
    init_keyboard(&fifo, 256);
    enable_mouse(&fifo, 512, &mdec);
    // fifo32_init(&fifo, 128, mousebuf);
    init_pit();
    io_out8(PIC0_IMR, 0xF8); /* PIC1 keyboard 和 PIC1許可(11111000) */
    io_out8(PIC1_IMR, 0xEF); /* mouse (11101111) */
    
    // fifo32_init( &fifo,   8,  timerbuf);
    timer  = timer_alloc();
    timer_init( timer, &fifo, 10);
    timer_settime( timer, 1000);
    timer2 = timer_alloc();
    timer_init(timer2, &fifo,  3);
    timer_settime(timer2,  300);
    timer3 = timer_alloc();
    timer_init(timer3, &fifo,  1);
    timer_settime(timer3,   50);

    memtotal = memtest(0x00400000, 0xBFFFFFFF);
    memman_init(man);
    memman_free(man, 0x00001000,            0x0009E000);
    memman_free(man, 0x00400000, memtotal - 0x00400000);
    
    init_palette();
    shtctl = shtctl_init(man, binfo->vram, binfo->scrnx, binfo->scrny);
    sht_back  = sheet_alloc(shtctl);
    sht_mouse = sheet_alloc(shtctl);
    sht_win   = sheet_alloc(shtctl);
    buf_back = (unsigned char *) memman_alloc_4k(man, binfo->scrnx * binfo->scrny);
    buf_win  = (unsigned char *) memman_alloc_4k(man,                    160 * 52);
    sheet_setbuf( sht_back,  buf_back, binfo->scrnx, binfo->scrny, -1);
    sheet_setbuf(sht_mouse, buf_mouse,           16,           16, 99);
    sheet_setbuf(  sht_win,   buf_win,          160,           52, -1);
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);
    init_mouse_cursor8(buf_mouse, 99);
    make_window8(buf_win, 160, 52, "Counter");
    sheet_slide(sht_back, 0, 0);
    mx = (binfo->scrnx - 16) / 2;
    my = (binfo->scrny - 28 - 16) / 2;
    sheet_slide(sht_mouse, mx, my);
    sheet_slide(sht_win, 80, 72);
    sheet_updown(sht_back,   0);
    sheet_updown(sht_win,    1);
    sheet_updown(sht_mouse,  2);
    sprintf(s, "(%3d, %3d)", mx, my);
    putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
    sprintf(s, "Memory %d MB    Free : %d KB", memtotal/(1024*1024), memman_total(man)/1024);
    putfonts8_asc_sht(sht_back, 0, 32, COL8_FFFFFF, COL8_008484, s, 40);


    while(1) {
        count++;
        io_cli();   // 屏蔽中断
        if(fifo32_status(&fifo) == 0) {
            io_sti();
        } else {
            i = fifo32_get(&fifo);
            if(i == 0) {
                timer_init(timer3, &fifo, 1);
                boxfill8(buf_back, binfo->scrnx, COL8_FFFFFF, 8, 96, 15, 111);
                timer_settime(timer3, 50);
                sheet_refresh(sht_back, 8, 96, 16, 112); 
            } else if(i == 1) {
                timer_init(timer3, &fifo, 0);
                boxfill8(buf_back, binfo->scrnx, COL8_008484, 8, 96, 15, 111);
                timer_settime(timer3, 50);
                sheet_refresh(sht_back, 8, 96, 16, 112); 
            } else if(i == 3) {
                sprintf(s, "%04d[sec]", timerctl.count/100);
                putfonts8_asc_sht(sht_back, 0, 80, COL8_FFFFFF, COL8_008484, s, 9);
                count = 0;
            } else if(i == 10) {
                sprintf(s, "%010d", count);
                putfonts8_asc_sht(sht_win, 40, 28, COL8_000000, COL8_C6C6C6, s, 10);
                sprintf(s, "%04d[sec]", timerctl.count/100);
                putfonts8_asc_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_008484, s, 9);
            } else if(i >= 256 && i <= 511) {
                sprintf(s, "%02X", i);
                putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
            } else if(i >= 512 && i <= 767) {
                if(mouse_decode(&mdec, i) != 0) {
                    sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
                    if((mdec.btn & 0x01) != 0)
                        s[1] = 'L';
                    if((mdec.btn & 0x02) != 0)
                        s[3] = 'R';
                    if((mdec.btn & 0x04) != 0)
                        s[2] = 'C';
                    putfonts8_asc_sht(sht_back, 32, 16, COL8_FFFFFF, COL8_008484, s, 15);
                    mx += mdec.x;
                    my += mdec.y;

                    mx = mx < 0 ? 0 : mx;
                    my = my < 0 ? 0 : my;

                    mx = mx > binfo->scrnx - 1 ? binfo->scrnx - 1 : mx;
                    my = my > binfo->scrny - 1 ? binfo->scrny - 1 : my;

                    sprintf(s, "(%3d, %3d)", mx, my);
                    putfonts8_asc_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s, 10);
                    sheet_slide(sht_mouse, mx, my);
                }
            }
        }
    }
}

void putfonts8_asc_sht(Sheet *sht, int x, int y, int c, int b, char* s, int l) {
    boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15);
    putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
    sheet_refresh(sht, x, y, x + l * 8, y + 16);
    return;
}
