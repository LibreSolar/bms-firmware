 /* mbed Library for EA DOG display
  *
  * This library is partly based on mbed DogLCD class by Igor Skochinsky and
  * the official Electronic Assembly library for Arduino
  */


#include "DogLCD.h"

// macro to make sure x falls into range from low to high (inclusive)
#define CLAMP(x, low, high) { if ( (x) < (low) ) x = (low); if ( (x) > (high) ) x = (high); } while (0);

void DogLCD::_send_commands(const unsigned char* buf, size_t size)
{
    // for commands, A0 is low
#ifdef READJUST_SPI_FREQUENCY
    _spi.format(8,0);
    _spi.frequency(10000000);
#endif
    _cs = 0;
    _a0 = 0;
    while ( size-- > 0 )
        _spi.write(*buf++);
    _cs = 1;
}

void DogLCD::_send_data(const unsigned char* buf, size_t size)
{
    // for data, A0 is high
#ifdef READJUST_SPI_FREQUENCY
    _spi.format(8,0);
    _spi.frequency(10000000);
#endif
    _cs = 0;
    _a0 = 1;
    while ( size-- > 0 )
        _spi.write(*buf++);
    _cs = 1;
    _a0 = 0;
}

// set column and page number
// void dog_1701::position(byte column, byte page)
void DogLCD::_set_xy(int x, int y)
{
    int column = x;
    int page = y;

    if (_view == VIEW_TOP)
		column += 30;

    unsigned char cmd[3];
	cmd[0] = 0x10 + (column>>4); 	//MSB adress column
	cmd[1] = 0x00 + (column&0x0F);	//LSB adress column
	cmd[2] = 0xB0 + (page&0x0F); 	//adress page

    /*
    //printf("_set_xy(%d,%d)\n", x, y);
    CLAMP(x, 0, LCDWIDTH-1);
    CLAMP(y, 0, LCDPAGES-1);
    unsigned char cmd[3];
    cmd[0] = 0xB0 | (y & 0xF);
    cmd[1] = (x & 0xF);
    cmd[2] = 0x10 | ((x >> 4) & 0xF);
        */
    _send_commands(cmd, 3);
}

// initialize and turn on the display
void DogLCD::init()
{
    const unsigned char init_seq[] = {
        0x40,    //Display start line 0
        0xa1,    //ADC reverse
        0xc0,    //Normal COM0...COM63
        0xa6,    //Display normal
        0xa2,    //Set Bias 1/9 (Duty 1/65)
        0x2f,    //Booster, Regulator and Follower On
        0xf8,    //Set internal Booster to 4x
        0x00,
        0x27,    //Contrast set
        0x81,
        0x09,
        0xac,    //No indicator
        0x00,
        0xaf,    //Display on
    };
#ifndef READJUST_SPI_FREQUENCY
    _spi.format(8,0);
    _spi.frequency(10000000);
#endif
    //printf("Reset=L\n");
    _reset = 0;
    _cs = 1;
    wait_ms(1);

    //printf("Reset=H\n");
    _reset = 1;
    wait_ms(5);
    //printf("Sending init commands\n");
    _send_commands(init_seq, sizeof(init_seq));
}

/*----------------------------
Func: string
Desc: shows string with selected font on position
Vars: column (0..127/131), page(0..3/7),  font adress in programm memory, stringarray
------------------------------*/
void DogLCD::string(int column, int page, const char *font_adress, const char *str)
{
	unsigned int pos_array; 										//Postion of character data in memory array
	int x, y, column_cnt, width_max;								//temporary column and page adress, couloumn_cnt tand width_max are used to stay inside display area
	int start_code, last_code, width, page_height, bytes_p_char;	//font information, needed for calculation
	const char *string;

	start_code 	 = font_adress[2];  //get first defined character
	last_code	 = font_adress[3];  //get last defined character
	width		 = font_adress[4];  //width in pixel of one char
	page_height  = font_adress[6];  //page count per char
	bytes_p_char = font_adress[7];  //bytes per char

	if(page_height + page > 8) //stay inside display area
		page_height = 8 - page;

	//The string is displayed character after character. If the font has more then one page,
	//the top page is printed first, then the next page and so on
	for(y = 0; y < page_height; y++)
	{
		_set_xy(column, page+y); //set startpositon and page
		column_cnt = column; //store column for display last column check
		string = str;             //temporary pointer to the beginning of the string to print
        _cs = 0;
		_a0 = 1;
		while(*string != 0)
		{
			if((int)*string < start_code || (int)*string > last_code) //make sure data is valid
				string++;
			else
			{
				//calculate positon of ascii character in font array
				//bytes for header + (ascii - startcode) * bytes per char)
				pos_array = 8 + (unsigned int)(*string++ - start_code) * bytes_p_char;
				pos_array += y*width; //get the dot pattern for the part of the char to print

				if(column_cnt + width > 102) //stay inside display area
					width_max = 102-column_cnt;
				else
					width_max = width;
				for(x=0; x < width_max; x++) //print the whole string
				{
					_spi.write(font_adress[pos_array+x]);
					//spi_out(pgm_read_byte(&font_adress[pos_array+x])); //double width font (bold)
				}
				column_cnt += width;
			}
		}
		_cs = 1;
        _a0 = 0;
	}
}

/*----------------------------
Func: view
Desc: ssets the display viewing direction
Vars: direction (top view 0xC8, bottom view (default) = 0xC0)
------------------------------*/
void DogLCD::view(unsigned int direction)
{
    unsigned char cmd[2];

	if(direction == VIEW_TOP)
	{
        cmd[0] = 0xA0;
        _view = VIEW_TOP;
	}
	else
	{
        cmd[0] = 0xA1;
        _view = VIEW_BOTTOM;
	}
    cmd[1] = direction;
    _send_commands(cmd, 2);

	clear_screen(); //Clear screen, as old content is not usable (mirrored)
}

void DogLCD::send_pic(const unsigned char* data)
{
    //printf("Sending picture\n");
    for (int i=0; i<LCDPAGES; i++)
    {
        _set_xy(0, i);
        _send_data(data + i*LCDWIDTH, LCDWIDTH);
    }
}

void DogLCD::clear_screen()
{
    //printf("Clear screen\n");
    memset(_framebuffer, 0, sizeof(_framebuffer));
    if ( _updating == 0 )
    {
        send_pic(_framebuffer);
    }
}

void DogLCD::all_on(bool on)
{
    //printf("Sending all on %d\n", on);
    unsigned char cmd = 0xA4 | (on ? 1 : 0);
    _send_commands(&cmd, 1);
}

void DogLCD::pixel(int x, int y, int colour)
{
    CLAMP(x, 0, LCDWIDTH-1);
    CLAMP(y, 0, LCDHEIGHT-1);
    int page = y / 8;
    unsigned char mask = 1<<(y%8);
    unsigned char *byte = &_framebuffer[page*LCDWIDTH + x];
    if ( colour == 0 )
        *byte &= ~mask; // clear pixel
    else
        *byte |= mask; // set pixel
    if ( !_updating )
    {
        _set_xy(x, page);
        _send_data(byte, 1);
    }
}

void DogLCD::fill(int x, int y, int width, int height, int colour)
{
    /*
      If we need to fill partial pages at the top:

      ......+---+---+.....
       ^    | = | = |     = : don't touch
       |    | = | = |     * : update
      y%8   | = | = |
       |    | = | = |
       v    | = | = |
    y---->  | * | * |
            | * | * |
            | * | * |
            +---+---+
    */
    //printf("fill(x=%d, y=%d, width=%d, height=%d, colour=%x)\n",  x, y, width, height, colour);
    CLAMP(x, 0, LCDWIDTH-1);
    CLAMP(y, 0, LCDHEIGHT-1);
    CLAMP(width, 0, LCDWIDTH - x);
    CLAMP(height, 0, LCDHEIGHT - y);
    int page = y/8;
    int firstpage = page;
    int partpage = y%8;
    if ( partpage != 0 )
    {
        // we need to process partial bytes in the top page
        unsigned char mask = (1<<partpage) - 1; // this mask has 1s for bits we need to leave
        unsigned char *bytes = &_framebuffer[page*LCDWIDTH + x];
        for ( int i = 0; i < width; i++, bytes++ )
        {
          // clear "our" bits
          *bytes &= mask;
          if ( colour != 0 )
            *bytes |= ~mask; // set our bits
        }
        height -= partpage;
        page++;
    }
    while ( height >= 8 )
    {
        memset(&_framebuffer[page*LCDWIDTH + x], colour == 0 ? 0 : 0xFF, width);
        page++;
        height -= 8;
    }
    if ( height != 0 )
    {
        // we need to process partial bytes in the bottom page
        unsigned char mask = ~((1<<partpage) - 1); // this mask has 1s for bits we need to leave
        unsigned char *bytes = &_framebuffer[page*LCDWIDTH + x];
        for ( int i = 0; i < width; i++, bytes++ )
        {
          // clear "our" bits
          *bytes &= mask;
          if ( colour != 0 )
            *bytes |= ~mask; // set our bits
        }
        page++;
    }
    //printf("_updating=%d\n", _updating);
    if ( !_updating )
    {
        int laststpage = page;
        for ( page = firstpage; page < laststpage; page++)
        {
            //printf("setting x=%d, page=%d\n", x, page);
            _set_xy(x, page);
            //printf("sending %d bytes at offset %x\n", width, page*LCDWIDTH + x);
            _send_data(&_framebuffer[page*LCDWIDTH + x], width);
        }
    }
}

void DogLCD::beginupdate()
{
    _updating++;
    //printf("beginupdate: %d\n", _updating);
}

void DogLCD::endupdate()
{
    _updating--;
    //printf("endupdate: %d\n", _updating);
    if ( _updating == 0 )
    {
        send_pic(_framebuffer);
    }
}
