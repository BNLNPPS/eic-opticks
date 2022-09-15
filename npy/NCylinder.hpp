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

#pragma once

#include "NNode.hpp"
#include "NGLM.hpp"
#include "NCylinder.h"

struct npart ;
struct nbbox ; 
struct nuv ; 

#include "NPY_API_EXPORT.hh"


struct NPY_API ncylinder : nnode 
{
    static ncylinder* Create(const nquad& param, const nquad& param1, bool old ); 
    static ncylinder* Create(float radius_, float z1_, float z2_, bool old=false ); 
    static ncylinder* Create(); 
    static ncylinder* Create(float x0, float y0, float z0, float w0, float x1, float y1, float z1, float w1); 


    float operator()(float x, float y, float z) const ;

    nbbox bbox() const;

    int       par_euler() const ; 
    unsigned  par_nsurf() const ; 
    glm::vec3 par_pos_model(const nuv& uv) const  ;
    unsigned  par_nvertices(unsigned nu, unsigned nv) const ; 

    void increase_z2(float dz);
    void decrease_z1(float dz);

    void set_z1(float new_z1); 
    void set_z2(float new_z2); 

    glm::vec3 gseedcenter() const ;
    glm::vec3 gseeddir() const ;

    void pdump(const char* msg="ncylinder::pdump") const ;
 
    glm::vec3  center() const  ; 
    float      radius() const  ; 
    float      x() const ; 
    float      y() const ; 
    float      z() const ; 
    float     z1() const ; 
    float     z2() const ; 
    float     r1() const ; 
    float     r2() const ; 
};




