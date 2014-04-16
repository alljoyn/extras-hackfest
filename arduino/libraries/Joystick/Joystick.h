/**
 * @file
 * Arduino Joystick shield driver library.
 */

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

#include "Arduino.h"


/**
 * This library implements a generic Joystick driver.  It uses the Timer 1
 * interrupt to perform the actual reads of the buttons and joystick position.
 * This allows for a simple debounce algorithm and allows for averaging analog
 * reads to get a more stable joystick position reading.  This should work
 * with any joystick shield with an analog joystick.
 */
class Joystick
{
  public:
    /**
     * Setup reading from a Joystick shield.
     *
     * @param xPin          Analog pin for the X axis.
     * @param yPin          Analog pin for the Y axis.
     * @param buttonMap     Array of digital I/O pins that map to buttons.
     * @param numButtons    Size of the button map.
     * @param pressInd      Set to 1 if pressing a button causes the digital
     *                      input to read as 1.  Set to 0 if pressing a button
     *                      causes the digital input to read as 0.
     */
    Joystick(int xPin, int yPin, const int* buttonMap, int numButtons, int pressInd);
    ~Joystick();

    void begin();

    /**
     * Set the analog read calibration for the X axis.
     *
     * @param left      Analog value when the joystick is to the far left
     *                  position.  Any analog reads that are "more left" than
     *                  this value will be clamped down to this value.
     * @param right     Analog value when the joystick is to the far right
     *                  position.  Any analog reads that are "more right" than
     *                  this value will be clamped down to this value.
     */
    void setXCal(int left, int right);

    /**
     * Set the analog read calibration for the Y axis.
     *
     * @param up        Analog value when the joystick is to the far up
     *                  position.  Any analog reads that are "more up" than
     *                  this value will be clamped down to this value.
     * @param down      Analog value when the joystick is to the far down
     *                  position.  Any analog reads that are "more down" than
     *                  this value will be clamped down to this value.
     */
    void setYCal(int up, int down);

    /**
     * Sets the ouput range for the X axis.
     *
     * @param left      Output value for maximal left position.
     * @param right     Output value for maximal right position.
     */
    void setXRange(int left, int right);

    /**
     * Sets the ouput range for the Y axis.
     *
     * @param up        Output value for maximal up position.
     * @param down      Output value for maximal down position.
     */
    void setYRange(int up, int down);


    int getXCalLeft() const { return xCalOffset; }
    int getXCalRight() const { return xCalScale + xCalOffset; }
    int getYCalUp() const { return yCalOffset; }
    int getYCalDown() const { return yCalScale + yCalOffset; }


    int getXOutLeft() const { return xOutOffset; }
    int getXOutRight() const { return xOutScale + xOutOffset; }
    int getYOutUp() const { return yOutOffset; }
    int getYOutDown() const { return yOutScale + yOutOffset; }

    void reset();


    /**
     * Indicates if the state of any of the buttons or joystick position has
     * changed since the last read.  Calling this function does not constitute
     * as a "read" so this function will continue to return true until either
     * the button or joystick axis that change gets read or the state of the
     * joystick shield is restored to its previous state.
     *
     * @return  1 if the state changed; 0 if the state has not changed.
     */
    int stateChanged(void);

    /**
     * Read the button state.  This returns a bitmap of which buttons are
     * pressed and which are not.  Bit 0 is the first button in the buttonMap
     * that was passed into the constructor.  Bit 1 is the second button inthe
     * buttonMap, and so on.  This will update the internal state change
     * detection logic for button press events.
     *
     * @return  Bitmap of the button press state.
     */
    unsigned short readButtons(void);

    /**
     * Identify which buttons changed state since the last read.  This returns
     * a bitmap of which buttons changed state.  If the bit is set, the
     * associated button toggled.  If the bit is unset, the associated button
     * did not change.  Bit 0 is the first button in the buttonMap that was
     * passed into the constructor.  Bit 1 is the second button inthe
     * buttonMap, and so on.  This will update the internal state change
     * detection logic for button press events.
     *
     * @return  Bitmap of the button press state.
     */
    unsigned short buttonsChanged(void);

    /**
     * Read the joystick X axis value.
     *
     * @return  The X axis value.
     */
    int readXPos(void);

    /**
     * Read the joystick Y axis value.
     *
     * @return  The Y axis value.
     */
    int readYPos(void);

  private:
    int xCalOffset;
    int xCalScale;
    int xOutOffset;
    int xOutScale;

    int yCalOffset;
    int yCalScale;
    int yOutOffset;
    int yOutScale;

    unsigned short oldButtonState;
    int oldXPos;
    int oldYPos;

    const int xPin;
    const int yPin;
    const unsigned short pressIndNormalizer;

};

#endif
