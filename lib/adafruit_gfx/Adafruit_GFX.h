/***********************************
This is a our graphics core library, for all our displays. 
We'll be adapting all the
existing libaries to use this core to make updating, support 
and upgrading easier!

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above must be included in any redistribution
****************************************/

/*
 *  Modified by Neal Horman 7/14/2012 for use in mbed
 */

#ifndef _ADAFRUIT_GFX_H_
#define _ADAFRUIT_GFX_H_

#include "Adafruit_GFX_Config.h"

static inline void swap(int16_t &a, int16_t &b)
{
    int16_t t = a;
    
    a = b;
    b = t;
}

#ifndef _BV
#define _BV(bit) (1<<(bit))
#endif

#define BLACK 0
#define WHITE 1

/**
 * This is a Text and Graphics element drawing class.
 * These functions draw to the display buffer.
 *
 * Display drivers should be derived from here.
 * The Display drivers push the display buffer to the
 * hardware based on application control.
 *
 */
class Adafruit_GFX : public Stream
{
 public:
    Adafruit_GFX(int16_t w, int16_t h)
        : _rawWidth(w)
        , _rawHeight(h)
        , _width(w)
        , _height(h)
        , cursor_x(0)
        , cursor_y(0)
        , textcolor(WHITE)
        , textbgcolor(BLACK)
        , textsize(1)
        , rotation(0)
        , wrap(true)
        {};

    /// Paint one BLACK or WHITE pixel in the display buffer
    // this must be defined by the subclass
    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) = 0;
    // this is optional
    virtual void invertDisplay(bool i) {};
    
    // Stream implementation - provides printf() interface
    // You would otherwise be forced to use writeChar()
    virtual int _putc(int value) { return writeChar(value); };
    virtual int _getc() { return -1; };

#ifdef GFX_WANT_ABSTRACTS
    // these are 'generic' drawing functions, so we can share them!
    
    /** Draw a Horizontal Line
     * @note GFX_WANT_ABSTRACTS must be defined in Adafruit_GFX_config.h
     */
    virtual void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    /** Draw a rectangle
     * @note GFX_WANT_ABSTRACTS must be defined in Adafruit_GFX_config.h
     */
    virtual void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    /** Fill the entire display
     * @note GFX_WANT_ABSTRACTS must be defined in Adafruit_GFX_config.h
     */
    virtual void fillScreen(uint16_t color);

    /** Draw a circle
     * @note GFX_WANT_ABSTRACTS must be defined in Adafruit_GFX_config.h
     */
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color);
    
    /** Draw and fill a circle
     * @note GFX_WANT_ABSTRACTS must be defined in Adafruit_GFX_config.h
     */
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color);

    /** Draw a triangle
     * @note GFX_WANT_ABSTRACTS must be defined in Adafruit_GFX_config.h
     */
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    /** Draw and fill a triangle
     * @note GFX_WANT_ABSTRACTS must be defined in Adafruit_GFX_config.h
     */
    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    
    /** Draw a rounded rectangle
     * @note GFX_WANT_ABSTRACTS must be defined in Adafruit_GFX_config.h
     */
    void drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
    /** Draw and fill a rounded rectangle
     * @note GFX_WANT_ABSTRACTS must be defined in Adafruit_GFX_config.h
     */
    void fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color);
    /** Draw a bitmap
     * @note GFX_WANT_ABSTRACTS must be defined in Adafruit_GFX_config.h
     */
    void drawBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);
#endif

#if defined(GFX_WANT_ABSTRACTS) || defined(GFX_SIZEABLE_TEXT)
    /** Draw a line
     * @note GFX_WANT_ABSTRACTS or GFX_SIZEABLE_TEXT must be defined in Adafruit_GFX_config.h
     */
    virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    /** Draw a vertical line
     * @note GFX_WANT_ABSTRACTS or GFX_SIZEABLE_TEXT must be defined in Adafruit_GFX_config.h
     */
    virtual void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    /** Draw and fill a rectangle
     * @note GFX_WANT_ABSTRACTS or GFX_SIZEABLE_TEXT must be defined in Adafruit_GFX_config.h
     */
    virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
#endif

    /// Draw a text character at a specified pixel location
    void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size);
    /// Draw a text character at the text cursor location
    size_t writeChar(uint8_t);

    /// Get the width of the display in pixels
    inline int16_t width(void) { return _width; };
    /// Get the height of the display in pixels
    inline int16_t height(void) { return _height; };

    /// Set the text cursor location, based on the size of the text
    inline void setTextCursor(int16_t x, int16_t y) { cursor_x = x; cursor_y = y; };
#if defined(GFX_WANT_ABSTRACTS) || defined(GFX_SIZEABLE_TEXT)
    /** Set the size of the text to be drawn
     * @note Make sure to enable either GFX_SIZEABLE_TEXT or GFX_WANT_ABSTRACTS
     */
    inline void setTextSize(uint8_t s) { textsize = (s > 0) ? s : 1; };
#endif
    /// Set the text foreground and background colors to be the same
    inline void setTextColor(uint16_t c) { textcolor = c; textbgcolor = c; }
    /// Set the text foreground and background colors independantly
    inline void setTextColor(uint16_t c, uint16_t b) { textcolor = c; textbgcolor = b; };
    /// Set text wraping mode true or false
    inline void setTextWrap(bool w) { wrap = w; };

    /// Set the display rotation, 1, 2, 3, or 4
    void setRotation(uint8_t r);
    /// Get the current rotation
    inline uint8_t getRotation(void) { rotation %= 4; return rotation; };

protected:
    int16_t  _rawWidth, _rawHeight;   // this is the 'raw' display w/h - never changes
    int16_t  _width, _height; // dependent on rotation
    int16_t  cursor_x, cursor_y;
    uint16_t textcolor, textbgcolor;
    uint8_t  textsize;
    uint8_t  rotation;
    bool  wrap; // If set, 'wrap' text at right edge of display
};

#endif
