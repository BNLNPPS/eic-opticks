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
#include <limits>
#include <csignal>

#include "SSys.hh"

#include "OPTICKS_LOG.hh"


int test_tpmt()
{
    return SSys::run("tpmt.py");
}

int test_RC(int irc)
{
    //assert( irc < 0xff && irc >= 0 ) ; 
    std::stringstream ss ; 
    ss << "python -c 'import sys ; sys.exit(" << irc << ")'" ;
    std::string s = ss.str();
    return SSys::run(s.c_str());
}

void test_RunPythonScript()
{
    const char* script = "np.py" ; 
    SSys::RunPythonScript(script); 
}


void test_RC()
{
    int rc(0);
    for(int irc=0 ; irc < 500 ; irc+=10 )
    {
        int xrc = irc & 0xff ;   // beyond 0xff return codes get truncated 
        rc = test_RC(irc);       
        bool xrc_expect =  rc == xrc ;  
        assert( xrc_expect ); 
        if(!xrc_expect) std::raise(SIGINT); 
    } 
}


void test_DumpEnv()
{
    SSys::DumpEnv("OPTICKS"); 
}

void test_IsNegativeZero()
{
    float f = -0.f ; 
    float z = 0.f ; 

    bool expect = SSys::IsNegativeZero(f) == true 
               && SSys::IsNegativeZero(z) == false
               && SSys::IsNegativeZero(-1.f) == false
               ; 
    assert( expect );  
    if(!expect) std::raise(SIGINT); 
}

void test_hostname()
{
    LOG(info) << SSys::hostname() ; 
}


void test_POpen(bool chomp)
{
    std::vector<std::string> cmds = {"which python", "ls -l" }; 
    int rc(0); 

    for(unsigned i=0 ; i < cmds.size() ; i++)
    {
        const char* cmd = cmds[i].c_str() ; 
        LOG(info) << " cmd " << cmd  <<  " chomp " << chomp ;  
        std::string out = SSys::POpen(cmd, chomp, rc ) ; 

        LOG(info) 
            << " out " << out 
            << " rc " << rc
            ; 
    }
}


void test_POpen2(bool chomp)
{
    std::vector<std::string> cmds = {"which", "python", "ls", "-l" }; 
    int rc(0); 

    for(unsigned i=0 ; i < cmds.size()/2 ; i++)
    {
        const char* cmda = cmds[i*2+0].c_str() ; 
        const char* cmdb = cmds[i*2+1].c_str() ; 
        LOG(info) 
           << " cmda " << cmda  
           << " cmdb " << cmdb  
           << " chomp " << chomp
           ;  
        std::string out = SSys::POpen(cmda, cmdb, chomp, rc ) ; 
        LOG(info) 
            << " out " << out 
            << " rc " << rc
            ; 

    }
}

void test_Which()
{
    std::vector<std::string> scripts = {"python", "nonexisting" }; 

    for(unsigned i=0 ; i < scripts.size() ; i++)
    {
        const char* script = scripts[i].c_str(); 
        std::string path = SSys::Which( script ); 
        LOG(info) 
           << " script " << script 
           << " path " << path 
           << ( path.empty() ? " EMPTY PATH " : "" )
           ;  

    }
}



void test_hexlify()
{
    unsigned u = 0x12abcdef ; 
    std::cout << std::hex << u << " std::hex " << std::endl ; 
    std::cout << SSys::hexlify(&u,4,true) << " SSys::hexlify reverse=true " << std::endl ; 
    std::cout << SSys::hexlify(&u,4,false) << " SSys::hexlify reverse=false " << std::endl ; 
}

void test_getenvintvec()
{
    LOG(info); 
    std::vector<int> ivec ; 

    std::vector<int> ivals = {42,43,-44,1,2,3} ; 
    std::stringstream ss ; 
    for(unsigned i=0 ; i < ivals.size() ; i++)
    {
       ss << ivals[i] << ( i < ivals.size() -1 ? "," : "" ) ;   
    }
    std::string s = ss.str(); 

    const char* key = "SSYSTEST_GETENVINTVEC" ; 
    bool overwrite = true ; 
    SSys::setenvvar(key, s.c_str(), overwrite); 

    std::vector<int> ivals2 ; 
    unsigned n = SSys::getenvintvec(key, ivals2);
    assert( n == ivals.size() ); 
    assert( n == ivals2.size() ); 

    for(unsigned i=0 ; i < n ; i++)
    {
        std::cout << i << " " << ivals[i] <<  " " << ivals2[i] << std::endl ; 
        assert( ivals[i] == ivals2[i] );   
    } 
}

void test_getenvintvec_non()
{
    std::vector<int> m_skip_gencode ; 
    LOG(info) << " m_skip_gencode.size " << m_skip_gencode.size() ; 

    SSys::getenvintvec("OPTICKS_SKIP_GENCODE", m_skip_gencode, ',', ""); 
    LOG(info) << " m_skip_gencode.size " << m_skip_gencode.size() ; 

    for(unsigned i=0 ; i < m_skip_gencode.size() ; i++) std::cout << m_skip_gencode[i] << std::endl ; 
}



void test_getenvintvec_ptr()
{
    LOG(info); 
    const char* key = "SSYSTEST_IVEC" ; 
    std::vector<int>* ivec = SSys::getenvintvec(key); 

    LOG(info) 
        << " key " << key
        << " ivec " << ivec 
        << " ivec.size " << ( ivec ? ivec->size() : 0 )
        ; 
   
    if(ivec) for(unsigned i=0 ; i < ivec->size() ; i++ ) std::cout << (*ivec)[i] << std::endl ; 
}






void test_atof()
{
   const char* s = "45.2" ; 
   float f = SSys::atof_(s) ; 
   LOG(info) 
       << " s " << s 
       << " f " << f 
       ;

}

void test_getenvfloat()
{
    LOG(info); 
    const char* key = "SSYSTEST_GETENVFLOAT" ; 

    float f = 42.5f ; 
    std::stringstream ss ; 
    ss << std::fixed << f ;  
    std::string s = ss.str(); 

    bool overwrite = true ; 
    SSys::setenvvar(key, s.c_str(), overwrite); 

    float f2 = SSys::getenvfloat(key); 
    LOG(info) 
        << " f " << f 
        << " s " << s 
        << " f2 " << f2 
        ; 
    assert( f2 == f ); 
}

void test_OS()
{
    LOG(info) << SSys::OS ; 

}

void test_getenvfloatvec()
{
    const char* ekey = "CE_OFFSET" ; 
    const char* ceo = "0.0,-666.6,0.0" ; 
    SSys::setenvvar(ekey, ceo ); 

    std::vector<float>* fvec = SSys::getenvfloatvec(ekey);
    std::cout << SSys::Desc(fvec) << std::endl ; 
}


void test_getenvunsigned_fallback_max()
{
    unsigned pidx = SSys::getenvunsigned_fallback_max("PIDX"); 
    LOG(info) << " pidx " << pidx << " 0x" << std::hex << pidx << std::dec; 
}
void test_getenvunsigned()
{
    unsigned num = SSys::getenvunsigned("NUM", 0u); 
    LOG(info) << " NUM " << num ; 
}



int main(int argc , char** argv )
{
    OPTICKS_LOG(argc, argv);

    int rc(0) ;

    /** 
    rc = test_tpmt();
    rc = test_RC(77);
    LOG(info) << argv[0] << " rc " << rc ; 
    test_DumpEnv();
    test_IsNegativeZero(); 
    test_hostname();
    test_POpen(true);
    test_POpen(false);
    test_POpen2(true);
    test_POpen2(false);
    test_Which(); 
    test_hexlify();  
    test_getenvintvec(); 
    test_getenvintvec_ptr(); 
    test_getenvintvec_non(); 
    test_getenvfloat(); 
    test_atof(); 
    test_RunPythonScript(); 
    test_OS(); 
    test_getenvfloatvec(); 
    test_getenvunsigned_fallback_max(); 
    **/

    test_getenvunsigned(); 



    return rc  ; 
}

