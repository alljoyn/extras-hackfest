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

#include "ControlPanelProvided.h"
#include <qcc/String.h>
#include <iostream>     // std::cout
#include <sstream>      // std::stringstream
#include <stdlib.h>     // atoi

static uint16_t currentMessage = 1;
static uint32_t counterValue = 1;
static qcc::String counterBuffer = "1";

uint16_t getCurrentMessage() {
    return currentMessage;
}

void setCurrentMessage(uint16_t newMessage)
{
    currentMessage = newMessage;
}

void printCurrentMessage() {
	const char* message;
	switch (currentMessage) {
	case 1:
		message = "Message One";
		break;
	case 2:
		message = "Message Two";
		break;
	case 3:
		message = "Message Three";
		break;
	default:
		message = "Invalid message value";
		break;
	}
	std::cout << "\n ### printCurrentMessage:  " << message << "\n" << std::endl;
}

char const* getCounterValueString()
{
    std::stringstream sMessageId;
    sMessageId << counterValue;
    counterBuffer = sMessageId.str().c_str();
    return counterBuffer.c_str();
}

void setCounterValueString(char const* newCounterValue)
{
	counterValue = (uint32_t) atoi(newCounterValue);
}
