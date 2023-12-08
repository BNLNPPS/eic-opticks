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


// https://stackoverflow.com/questions/1908687/how-to-redirect-the-output-back-to-the-screen-after-freopenout-txt-a-stdo

//  https://stackoverflow.com/questions/5846691/how-to-restore-stdout-after-using-freopen

#include <iostream>
#include <cassert>
#include <csignal>
#include "S_freopen_redirect.hh"
#include "spath.h"
#include "sdirectory.h"
#include "ssys.h"


/**
only the std::cerr appears in output, the rest is directed to the file
**/

void test_redirect()
{
    const char* path = spath::Resolve("$TMP/S_freopen_redirect_test/redirect.log") ;
    sdirectory::MakeDirsForFile(path); 
    std::cout << "test_redirect writing to " << path << std::endl ;  

    ssys::Dump("before redirect, should appear in output"); 

    {
       S_freopen_redirect fr(stdout, path) ; 
       ssys::Dump("during redirect, should NOT appear in output, should be written to file : observe only std::cerr in output"); 
    }

    ssys::Dump("after redirect, should appear in output"); 
}


void test_runpython()
{
    //int rc = ssys::run("python -c 'print \"hello\"' " );
    int rc = ssys::run("python -c 'import sys ; sys.stderr.write(\"hello stderr\\n\")' " );
    //int rc = ssys::run("python -c 'import sys ; sys.stdout.write(\"hello stdout\\n\")' " );
    if(rc) std::raise(SIGINT); 
    assert( rc == 0 );
}



int main(void)
{
    test_runpython(); 
    test_redirect(); 

    return 0;
}



/*

close failed in file object destructor:
sys.excepthook is missing
lost sys.stderr

::

    double OContext::launch_redirected_(unsigned entry, unsigned width, unsigned height)
    {
        assert( m_llogpath ) ; 
        S_freopen_redirect sfr(stdout, m_llogpath );
        double dt = launch_( entry, width, height ) ;
        return dt ;                              
    }

OContext::launch_redirected_ succeeds to write kernel rtPrintf 
logging to file BUT a subsequent same process "system" invokation 
of python has problems indicating that cleanup is not complete
::

    Traceback (most recent call last):
      File "/Users/blyth/opticks/ana/tboolean.py", line 62, in <module>
        print ab
    IOError: [Errno 9] Bad file descriptor
    2017-12-13 15:33:13.436 INFO  [321569] [SSys::run@50] tboolean.py --tag 1 --tagoffset 0 --det tboolean-box --src torch   rc_raw : 256 rc : 1
    2017-12-13 15:33:13.436 WARN  [321569] [SSys::run@57] SSys::run FAILED with  cmd tboolean.py --tag 1 --tagoffset 0 --det tboolean-box --src torch  
    2017-12-13 15:33:13.436 INFO  [321569] [OpticksAna::run@79] OpticksAna::run anakey tboolean cmdline tboolean.py --tag 1 --tagoffset 0 --det tboolean-box --src torch   rc 1 rcmsg OpticksAna::run non-zero RC from ana script
    2017-12-13 15:33:13.436 FATAL [321569] [Opticks::dumpRC@186]  rc 1 rcmsg : OpticksAna::run non-zero RC from ana script
    2017-12-13 15:33:13.436 INFO  [321569] [SSys::WaitForInput@151] SSys::WaitForInput OpticksAna::run paused : hit RETURN to continue...


This issue is reproduced by this test

Perhaps dup2 can put humpty back together again
  
* https://msdn.microsoft.com/en-us/library/8syseb29.aspx

*/

