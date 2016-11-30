/* mbed Library for EA DOG display
 *
 * This library is partly based on mbed DogLCD class by Igor Skochinsky and
 * the official Electronic Assembly library for Arduino
 */


#ifndef __DOGLCD_H__
#define __DOGLCD_H__

#include "mbed.h"

/***********
 * Module for Electronic Assembly's DOGL128-6 display module
 * Should be compatible with other modules using ST7565 controller
 ***********/

#define LCDWIDTH 102
#define LCDHEIGHT 64
#define LCDPAGES  (LCDHEIGHT+7)/8


#define VIEW_BOTTOM 0xC0
#define VIEW_TOP 	0xC8

/*

 Each page is 8 lines, one byte per column

         Col0
        +---+--
        | 0 |
Page 0  | 1 |
        | 2 |
        | 3 |
        | 4 |
        | 5 |
        | 6 |
        | 7 |
        +---+--
*/

/*
  LCD interface class.
  Usage:
    DogLCD dog(spi, pin_power, pin_cs, pin_a0, pin_reset);
    where spi is an instance of SPI class
*/

class DogLCD
{
public:

    DogLCD(SPI& spi, PinName cs, PinName a0, PinName reset):
    _spi(spi), _cs(cs), _a0(a0), _reset(reset), _updating(0)
    { }

    void clear			(void);
    void contrast       (int contr);
    void view			(unsigned int direction);
    void string         (int column, int page, const char *font_adress, const char *str);
    void rectangle		(int start_column, int start_page, int end_column, int end_page, int pattern);
    void picture		(int column, int page, const int *pic_adress);

    // initialize and turn on the display
    void init();
    // send a 128x64 picture for the whole screen
    void send_pic(const unsigned char* data);
    // clear screen
    void clear_screen();
    // turn all pixels on
    void all_on(bool on = true);

    virtual int width()  {return LCDWIDTH;};
    virtual int height() {return LCDHEIGHT;};
    virtual void pixel(int x, int y, int colour);
    virtual void fill(int x, int y, int width, int height, int colour);
    virtual void beginupdate();
    virtual void endupdate();

private:
    SPI& _spi;
    DigitalOut _cs, _a0, _reset;
    int _view;
    int _updating;

    void _send_commands(const unsigned char* buf, size_t size);
    void _send_data(const unsigned char* buf, size_t size);
    void _set_xy(int x, int y);
    unsigned char _framebuffer[LCDWIDTH*LCDPAGES];

};

#endif //__DOGLCD_H__
