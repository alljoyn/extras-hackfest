#!/bin/bash

# Copyright (c) 2014 AllSeen Alliance. All rights reserved.
#
#    Permission to use, copy, modify, and/or distribute this software for any
#    purpose with or without fee is hereby granted, provided that the above
#    copyright notice and this permission notice appear in all copies.
#
#    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
#    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
#    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
#    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
#    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
#    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
#    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

if [ -d "allseen" ]; then
   read -p "The folder allseen already exists, running this script will overwrite it. Do you still wish to run it? (Y/N)" yn
   case $yn in
      [Yy]* ) rm -rf allseen;;
      [Nn]* ) exit 1;;
      * ) echo "Quitting"; exit 1;;
   esac
fi

if [ -f "duktape-0.11.0.tar.xz" ]; then
   read -p "The archive duktape-0.11.0.tar.xz already exists, running this script will overwrite it. Do you still wish to run it? (Y/N)" yn
   case $yn in
      [Yy]* ) rm -f duktape-0.11.0.tar.xz;;
      [Nn]* ) exit 1;;
      * ) echo "Quitting"; exit 1;;
   esac
fi

if ! which scons > /dev/null; then
   sudo apt-get install scons
fi

mkdir allseen
mkdir allseen/core
mkdir allseen/services

git clone https://git.allseenalliance.org/gerrit/core/ajtcl.git         allseen/core/ajtcl
git clone https://git.allseenalliance.org/gerrit/services/base_tcl.git  allseen/services/base_tcl
git clone https://git.allseenalliance.org/gerrit/core/alljoyn-js.git    allseen/core/alljoyn-js

wget http://duktape.org/duktape-0.11.0.tar.xz

tar xvfJ duktape-0.11.0.tar.xz -C $PWD/allseen/core/alljoyn-js/external

export DUKTAPE_DIST=$PWD/allseen/core/alljoyn-js/external/duktape-0.11.0

cd allseen/core/ajtcl
scons WS=off

cd ../alljoyn-js
scons WS=off

echo "Add this command to you bash startup script to .bashrc or .bashprofile or manually run it when you startup a terminal"
echo "export LD_LIBRARY_PATH=$PWD/allseen/core/ajtcl:$LD_LIBRARY_PATH"
