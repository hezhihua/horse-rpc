/**
 * Tencent is pleased to support the open source community by making Tars available.
 *
 * Copyright (C) 2016THL A29 Limited, a Tencent company. All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use this file except 
 * in compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software distributed 
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR 
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the 
 * specific language governing permissions and limitations under the License.
 */

#include "util/tc_ex.h"

#include <execinfo.h>


#include <string.h>
#include <stdlib.h>
#include <cerrno>
#include <iostream>

namespace horsedb
{

TC_Exception::TC_Exception(const string &buffer)
: _code(0), _buffer(buffer)
{
}


TC_Exception::TC_Exception(const string &buffer, int err)
{
	_buffer = buffer + " :" + parseError(err);
    _code   = err;
}

TC_Exception::~TC_Exception() throw()
{
}

const char* TC_Exception::what() const throw()
{
    return _buffer.c_str();
}

void TC_Exception::getBacktrace()
{

    void * array[64];
    int nSize = backtrace(array, 64);
    char ** symbols = backtrace_symbols(array, nSize);

    for (int i = 0; i < nSize; i++)
    {
        _buffer += symbols[i];
        _buffer += "\n";
    }
	free(symbols);

}



string TC_Exception::parseError(int err)
{
    string errMsg;


    errMsg = strerror(err);

    return errMsg;
}

int TC_Exception::getSystemCode()
{

    return errno; 
}

}
