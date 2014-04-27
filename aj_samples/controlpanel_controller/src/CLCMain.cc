/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

#include <iostream>
#include <sstream>
#include <cstdio>
#include <signal.h>
#include "CommandLineController.h"

CommandLineController* commandLineController = 0;

void exitApp(int32_t signum)
{
    std::cout << "Exiting..." << std::endl;

    if(commandLineController != NULL)
        commandLineController->shutdown();    
    
    std::cout << "Peace!" << std::endl;
    exit(signum);
}

int main()
{
    // Allow CTRL+C to end application
    signal(SIGINT, exitApp);
    std::cout << "====================================================\n";
    std::cout << "Control Panel Controller \n";
    std::cout << "(Press CTRL+C to end application) \n";

    commandLineController = new CommandLineController();
    commandLineController->initialize();
}

