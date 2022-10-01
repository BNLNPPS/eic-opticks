#include <string>
#include "SLOG.hh"
#include "NPY.hpp"
#include "MockSensorAngularEfficiencyTable.hh"

const plog::Severity MockSensorAngularEfficiencyTable::LEVEL = SLOG::EnvLevel("MockSensorAngularEfficiencyTable", "DEBUG"); 


NPY<float>* MockSensorAngularEfficiencyTable::Make(unsigned num_sensor_cat, unsigned num_theta_steps, unsigned num_phi_steps)  // static 
{
    MockSensorAngularEfficiencyTable tab( num_sensor_cat, num_theta_steps, num_phi_steps );  
    NPY<float>* arr = tab.getArray(); 
    return arr ; 
}


MockSensorAngularEfficiencyTable::MockSensorAngularEfficiencyTable( unsigned num_sensor_cat, unsigned num_theta_steps, unsigned num_phi_steps)
    :
    m_num_sensor_cat(num_sensor_cat),
    m_num_theta_steps(num_theta_steps),
    m_num_phi_steps(num_phi_steps),
    m_multiplicity(1),

    m_theta_min(0.f),
    m_theta_max(180.f),
    m_theta_step((m_theta_max - m_theta_min)/float(m_num_theta_steps)),

    m_phi_min(0.f),
    m_phi_max(360.f),
    m_phi_step((m_phi_max - m_phi_min)/float(m_num_phi_steps)), 

    m_shape(),
    m_values(),
    m_array(NULL)
{
    init();
}

/**
MockSensorAngularEfficiencyTable::getEfficiency
------------------------------------------------

The mocked up efficiency returned is the product of phi_eff and 
theta_eff terms. 

theta_eff
    arbitrary stripes  

phi_eff
    arbitrary cosine variation over phi for categories other than 0 

**/

float MockSensorAngularEfficiencyTable::getEfficiency(unsigned i_cat, unsigned j_theta, unsigned k_phi) const 
{
    float theta =  m_theta_min + j_theta*m_theta_step ;
    float phi = m_phi_min + k_phi*m_phi_step ; 
    const float twopi = 2.f*glm::pi<float>() ; 
           
    float phi_eff = i_cat == 0 ? 1.f : 0.5f + 0.5f*cos(phi*twopi/360.f) ; 
    float theta_eff = int(theta/10.) % 2 == 0 ? 0.f : 1.f ;    

    return phi_eff*theta_eff ; 
}


const std::vector<int> MockSensorAngularEfficiencyTable::getShape() const 
{
    return m_shape ; 
}
const std::vector<float> MockSensorAngularEfficiencyTable::getValues() const 
{
    return m_values ; 
}
NPY<float>* MockSensorAngularEfficiencyTable::getArray() const 
{
    return m_array ; 
}




/**
MockSensorAngularEfficiencyTable::init
-------------------------------------------

Thinking about image dimensions and array serialization find the below
choice of array shape to be more natural::

     (i/category, j/height/theta/0:180/latitude/N-S/polar, k/width/phi/0:360/longitude/E-W/azimuthal) 

With the j:height:theta index incrementing more slowly than the k:width:phi index in the 
serialization. Hence the serialization is composed of "nj" sequences of "nk" width:phi elements.
For the case of no dependency on phi nk=1 giving nominal shape (ncat,180, 1), a tall skinny array/image. 

For an example of spherical mapping of an Earth texture onto the sphere, 
see examples/UseOptiXTextureLayeredOKImgGeo.
**/

void MockSensorAngularEfficiencyTable::init()
{
    unsigned ni = m_num_sensor_cat ;
    unsigned nj = m_num_theta_steps ;
    unsigned nk = m_num_phi_steps ;
    unsigned nl = m_multiplicity ;  // GPU textures typically use 1 or 4 for the element multiplicity
    assert( nl == 1 );              // as last dimension is one there is no need to loop over it 

    m_shape.push_back(ni);
    m_shape.push_back(nj);
    m_shape.push_back(nk);
    m_shape.push_back(nl);

    m_values.resize(ni*nj*nk, 0.f); 

    for(unsigned i=0 ; i < ni ; i++)
    {
        for(unsigned j=0 ; j < nj ; j++)
        {
            for(unsigned k=0 ; k < nk ; k++)
            {
                 unsigned index = i*nj*nk + j*nk + k  ;
                 m_values[index] = getEfficiency(i,j,k) ;  
            }
        }
    }
    std::string metadata = "" ; 
    m_array = new NPY<float>(m_shape, m_values, metadata);
}


