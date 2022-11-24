/*
 * Copyright (c) 2019 Opticks Team. All Rights Reserved.
 *
 * This file is part of Opticks
 * (see https://bitbucket.org/simoncblyth/opticks).
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License.  
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 */

#include <sstream>
#include <string>
#include "STime.hh"
#include "SPath.hh"
#include <time.h>

int STime::EpochSeconds()
{
    time_t now = time(0);
    return now ; 
}


const char* STime::FMT = "%Y%m%d_%H%M%S" ; 
 
std::string STime::Format(int epochseconds, const char* fmt)
{
    const char* ufmt = fmt == NULL ? FMT : fmt ;  

    int t = epochseconds == 0 ? EpochSeconds() : epochseconds ; 
    time_t now(t) ;  
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), ufmt, &tstruct);
    return buf ;
}


std::string STime::Stamp()
{
    return STime::Format(0, nullptr); 
}
std::string STime::Now()
{
    return STime::Format(0, "%c"); 
}


std::string STime::mtime(const char* base, const char* name)
{
    std::stringstream ss ;  
    ss << base << "/" << name ; 
    std::string s = ss.str(); 
    return mtime(s.c_str());  
}

std::string STime::mtime(const char* path)
{
    int mt = SPath::mtime(path);   
    return mt > 0 ? STime::Format(mt, nullptr) : "" ;
}





