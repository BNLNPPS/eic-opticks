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

#include "SProc.hh"
#include "SLOG.hh"   // needed for plog::info enum etc..
    
// https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

/**
parseLine
-----------

Expects a line the below form with digits and ending in " Kb"::

   VmSize:	  108092 kB

**/

int parseLine(char* line){
    int i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = atoi(p);
    return i;
}

float SProc::VirtualMemoryUsageKB()
{
    FILE* file = fopen("/proc/self/status", "r");
    float result = 0.f ;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmSize:", 7) == 0){
            result = parseLine(line);   // value in Kb 
            break;
        }
    }
    fclose(file);
    return result;
}


float SProc::ResidentSetSizeKB()
{
    FILE* file = fopen("/proc/self/status", "r");
    float result = 0.f ;
    char line[128];

    while (fgets(line, 128, file) != NULL){
        if (strncmp(line, "VmRSS:", 6) == 0){
            result = parseLine(line);   // value in Kb 
            break;
        }
    }
    fclose(file);
    return result;
}


float SProc::VirtualMemoryUsageMB()
{
    float result = VirtualMemoryUsageKB() ; 
    return result/1000.f ;   
}

float SProc::ResidentSetSizeMB()
{
    float result = ResidentSetSizeKB() ; 
    return result/1000.f ;   
}


/**
SProc::ExecutablePath
-----------------------

* https://stackoverflow.com/questions/799679/programmatically-retrieving-the-absolute-path-of-an-os-x-command-line-app/1024933#1024933

**/

#include <unistd.h>
#include <limits.h>

const char* SProc::ExecutablePath(bool basename)
{
    char buf[PATH_MAX];
    ssize_t len = ::readlink("/proc/self/exe", buf, sizeof(buf)-1);
    if (len != -1) buf[len] = '\0';

    const char* s = basename ? strrchr(buf, '/') : NULL ;  
    return s ? strdup(s+1) : strdup(buf) ; 
}


const char* SProc::ExecutableName()
{
    bool basename = true ; 
    return ExecutablePath(basename); 
}
