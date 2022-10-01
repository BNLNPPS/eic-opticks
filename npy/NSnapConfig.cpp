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

#include "BFile.hh"
#include "BConfig.hh"
#include "SLOG.hh"
#include "NSnapConfig.hpp"

const plog::Severity NSnapConfig::LEVEL = SLOG::EnvLevel("NSnapConfig","DEBUG") ; 

const float NSnapConfig::NEGATIVE_ZERO = -0.f ; 

NSnapConfig::NSnapConfig(const char* cfg)  
    :
    bconfig(new BConfig(cfg)),
    verbosity(0),
    steps(10),
    width(5),
    ex0(NEGATIVE_ZERO),   // -ve zero on ex0,ey0,ez0 indicates leave asis, see OpTracer::snap
    ey0(NEGATIVE_ZERO),
    ez0(NEGATIVE_ZERO),
    ex1(NEGATIVE_ZERO),
    ey1(NEGATIVE_ZERO),
    ez1(NEGATIVE_ZERO),
    prefix("snap"),
    ext(".jpg")
{
    LOG(LEVEL)
              << " cfg [" << ( cfg ? cfg : "NULL" ) << "]"
              ;

    bconfig->addInt("verbosity", &verbosity );
    bconfig->addInt("steps", &steps );

    bconfig->addFloat("ex0", &ex0 );
    bconfig->addFloat("ex1", &ex1 );

    bconfig->addFloat("ey0", &ey0 );
    bconfig->addFloat("ey1", &ey1 );

    bconfig->addFloat("ez0", &ez0 );
    bconfig->addFloat("ez1", &ez1 );

    bconfig->addString("prefix", &prefix );
    bconfig->addString("ext",    &ext );   

    bconfig->parse();
}

const char* NSnapConfig::getCfg() const 
{
    return bconfig->cfg ; 
}

void NSnapConfig::dump(const char* msg) const
{
    bconfig->dump(msg);
}

std::string NSnapConfig::getSnapName(int index, const char* nameprefix) const 
{
    const char* pfx = nameprefix ? nameprefix : prefix.c_str()  ; 
    return BFile::MakeName(index, width, pfx, ext.c_str() ); 
}

const char* NSnapConfig::getSnapPath(const char* outdir, int index, const char* nameprefix) const 
{
    std::string name = getSnapName(index, nameprefix) ; 
    bool create = true ; 
    std::string path = BFile::preparePath(outdir, nullptr, name.c_str(), create);  
    return strdup(path.c_str()); 
}

std::string NSnapConfig::desc() const 
{
    return bconfig->desc() ;
}
