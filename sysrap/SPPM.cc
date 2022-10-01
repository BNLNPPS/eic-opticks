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


#include <iostream>
#include <fstream>

#include "SLOG.hh"
#include "SStr.hh"
#include "SColor.hh"
#include "SPPM.hh"

const plog::Severity SPPM::LEVEL = SLOG::EnvLevel("SPPM", "DEBUG")  ; 

SPPM::SPPM()
    :   
    pixels(NULL),
    pwidth(0),
    pheight(0),
    pscale(0),
    yflip(true)
{
}

std::string SPPM::desc() const 
{
    std::stringstream ss ; 
    ss << " SPPM " 
       << " pwidth " << pwidth 
       << " pheight " << pheight
       << " pscale " << pscale
       << " yflip " << yflip
        ;
    return ss.str(); 
}

void SPPM::resize( int width, int height, int scale )
{ 
    bool changed_size = !(width == pwidth && height == pheight && scale == pscale) ; 
    if( pixels == NULL || changed_size )
    {   
        delete [] pixels ;
        pixels = NULL ; 
        pwidth = width ; 
        pheight = height ; 
        pscale = scale ; 
        int size = 4 * pwidth * pscale * pheight * pscale ;
        pixels = new unsigned char[size];
        LOG(LEVEL) << "creating resized pixels buffer " << desc() ; 
    }   
}

void SPPM::save(const char* path)
{
    if(path == NULL ) path = "/tmp/SPPM.ppm" ; 
    save(path, pwidth*pscale, pheight*pscale, pixels, yflip );
    LOG(fatal) 
        << " path " << path 
        << " desc " << desc()
        ; 
}

void SPPM::save(const char* path, int width, int height, const unsigned char* image, bool yflip)
{
    //LOG(info) << "saving to " << path ; 
    std::cout << "SPPM::save " << path << std::endl ;  

    FILE * fp;
    fp = fopen(path, "wb");
    if(!fp) LOG(fatal) << "FAILED to open for writing " << path ; 
    assert(fp); 

    int ncomp = 4;
    fprintf(fp, "P6\n%d %d\n%d\n", width, height, 255);

    unsigned size = height*width*3 ; 
    unsigned char* data = new unsigned char[size] ;
 
    for( int h=0 ; h < height ; h++ ) 
    {
        int y = yflip ? height - 1 - h : h ;  

        for( int x=0; x < width ; ++x )
        {
            *(data + (y*width+x)*3+0) = image[(h*width+x)*ncomp+0] ;
            *(data + (y*width+x)*3+1) = image[(h*width+x)*ncomp+1] ;
            *(data + (y*width+x)*3+2) = image[(h*width+x)*ncomp+2] ;
        }
    }
    fwrite(data, sizeof(unsigned char)*size, 1, fp);
    fclose(fp);
    delete[] data;
}


/**
SPPM::snap
------------

Invokes download from subclass, eg oglrap/Pix
and then saves.

**/


void SPPM::snap(const char* path)
{
    download(); 
    save(path); 
}


void SPPM::write( const char* filename, const float* image, int width, int height, int ncomp, bool yflip )
{
    std::ofstream out( filename, std::ios::out | std::ios::binary );
    if( !out ) 
    {
        std::cerr << "Cannot open file " << filename << "'" << std::endl;
        return;
    }

    out << "P6\n" << width << " " << height << "\n255" << std::endl;

    for( int h=0; h < height ; h++ ) // flip vertically
    {   
        int y = yflip ? height - 1 - h : h ; 

        for( int x = 0; x < width*ncomp; ++x ) 
        {   
            float val = image[y*width*ncomp + x];    // double flip ?
            unsigned char cval = val < 0.0f ? 0u : val > 1.0f ? 255u : static_cast<unsigned char>( val*255.0f );
            out.put( cval );
        }   
    }
    LOG(LEVEL) << "Wrote file (float*)" << filename ;
}

/**
SPPM::write
-------------

Note the intermediary array. This allows four component image data to be written into 
PPM format which is by definition 3 component and also allows for vertical flipping
as conventions vary in this regard resulting in a common need to yflip upside down images.

**/

void SPPM::write( const char* filename, const unsigned char* image, int width, int height, int ncomp, bool yflip )
{
    FILE * fp;
    fp = fopen(filename, "wb");
    if(!fp) LOG(fatal) << "FAILED to open for writing " << filename ;  
    assert(fp); 
    fprintf(fp, "P6\n%d %d\n%d\n", width, height, 255);


    unsigned size = height*width*3 ; 
    unsigned char* data = new unsigned char[size] ; 

    for( int h=0; h < height ; h++ ) 
    {   
        int y = yflip ? height - 1 - h : h ;   // flip vertically

        for( int x=0; x < width ; ++x ) 
        {   
            *(data + (y*width+x)*3+0) = image[(h*width+x)*ncomp+0] ;   
            *(data + (y*width+x)*3+1) = image[(h*width+x)*ncomp+1] ;   
            *(data + (y*width+x)*3+2) = image[(h*width+x)*ncomp+2] ;   
        }
    } 

    fwrite(data, sizeof(unsigned char)*size, 1, fp);
    fclose(fp);  
    LOG(LEVEL) << "Wrote file (unsigned char*) " << filename  ;
    delete[] data;
}



/**
SPPM::read
------------

* https://en.wikipedia.org/wiki/Netpbm

**/

void SPPM::dumpHeader( const char* path )
{
    unsigned width(0); 
    unsigned height(0); 
    unsigned mode(0); 
    unsigned bits(0); 

    int rc = readHeader(path, width, height, mode, bits ); 

    LOG(info)
        << " path " << path 
        << " width " << width
        << " height " << height
        << " mode " << mode
        << " bits " << bits
        << " rc " << rc 
        ;
}

int SPPM::readHeader( const char* path, unsigned& width, unsigned& height, unsigned& mode, unsigned& bits )
{
    assert(SStr::EndsWith(path, ".ppm")); 
    std::ifstream f(path, std::ios::binary);
    if(f.fail())
    {
        LOG(fatal) << "Could not open path: " << path ;
        return 1 ;
    }

    mode = 0;
    std::string s;
    f >> s;
    if (s == "P3") mode = 3;
    else if (s == "P6") mode = 6;
    
    f >> width ;
    f >> height ;
    f >> bits;

    f.close();
    return 0 ; 
}


/**
SPPM::read
-----------

* http://netpbm.sourceforge.net/doc/ppm.html

A PPM file consists of a sequence of one or more PPM images. There are no data, delimiters, or padding before, after, or between images.

Each PPM image consists of the following:

1. A "magic number" for identifying the file type. A ppm image's magic number is the two characters "P6".
2. Whitespace (blanks, TABs, CRs, LFs).
3. A *Width*, formatted as ASCII characters in decimal.
4. Whitespace.
5. A *Height*, again in ASCII decimal.
6. Whitespace.
7. The maximum color value (Maxval), again in ASCII decimal. Must be less than 65536 and more than zero.
8. A single whitespace character (usually a newline).
9. A raster of *Height* rows, in order from top to bottom. 

   * Each row consists of *Width* pixels, in order from left to right. 
   * Each pixel is a triplet of red, green, and blue samples, in that order. 
   * Each sample is represented in pure binary by either 1 or 2 bytes. If the Maxval is less than 256, it is 1 byte. Otherwise, it is 2 bytes. 
     The most significant byte is first.

A row of an image is horizontal. A column is vertical. The pixels in the image are square and contiguous.

**/


int SPPM::read( const char* path, std::vector<unsigned char>& img, unsigned& width, unsigned& height, const unsigned ncomp, const bool yflip )
{
    assert(SStr::EndsWith(path, ".ppm")); 

    std::ifstream f(path, std::ios::binary);
    if(f.fail())
    {
        std::cout << "Could not open path: " << path << std::endl;
        return 1 ;
    }

    int mode = 0;
    std::string s;
    f >> s;
    if (s == "P3") mode = 3;
    else if (s == "P6") mode = 6;
    assert( mode == 6 ); 
    
    f >> width ;
    f >> height ;

    int bits = 0;
    f >> bits;
    assert( bits == 255 ); 
    f.get();

    unsigned filesize = width*height*3 ; 
    unsigned arraysize = ncomp == 3 ? filesize : width*height*ncomp  ; 

    img.clear(); 
    img.resize(arraysize);
    unsigned char* imgdata = img.data(); 

    if( ncomp == 3 && yflip == false ) // slurp straight into the vector when no shuffling needed 
    {
        f.read((char*)imgdata, filesize);
    }
    else
    {
        // read into tmp and then reorder into imgdata
        // NB assuming "row-major" layout 
  
        unsigned char* tmp = new unsigned char[filesize] ; 
        f.read( (char*)tmp, filesize);

        for( int h=0; h < int(height) ; h++ ) 
        {   
            int y = yflip ? height - 1 - h : h ;   // flip vertically
            for( int x=0; x < int(width) ; ++x ) 
            {   
                for( int k=0 ; k < int(ncomp) ; k++ )
                { 
                    imgdata[ (h*width+x)*ncomp + k] = k < 3 ? tmp[(y*width+x)*3+k] : 0xff ;        
                }
            }
        } 
        delete [] tmp ; 
    }
    f.close();
    return 0  ; 
}


void SPPM::AddBorder( std::vector<unsigned char>& img, const int width, const int height, const int ncomp, const bool yflip )
{
    int size = width*height*ncomp ; 
    assert( int(img.size()) == size ); 
    unsigned char* imgdata = img.data();  
    AddBorder( imgdata, width, height, ncomp, yflip ); 
}


void SPPM::AddBorder( unsigned char* imgdata, const int width, const int height, const int ncomp, const bool yflip )
{
    SColor border_color = SColors::red ; 

    int margin = 16 ; 

    for( int h=0; h < height ; h++ ) 
    {   
        int y = yflip ? height - 1 - h : h ;   // flip vertically

        bool y_border = y < margin || height - 1 - y < margin ; 

        for( int x=0; x < width ; ++x ) 
        {   
            bool x_border = x < margin || width - x < margin ;  

            if( y_border || x_border )            
            for( int k=0 ; k < ncomp ; k++ ) imgdata[ (h*width+x)*ncomp + k] = k < 3 ? border_color.get(k) : 0xff ;        

        }
    } 
}



void SPPM::AddMidline( std::vector<unsigned char>& img, const int width, const int height, const int ncomp, const bool yflip )
{
    int size = width*height*ncomp ; 
    assert( int(img.size()) == size ); 
    unsigned char* imgdata = img.data();  
    AddMidline( imgdata, width, height, ncomp, yflip ); 
}


void SPPM::AddMidline( unsigned char* imgdata, const int width, const int height, const int ncomp, const bool yflip )
{
    SColor midline_color = SColors::green ; 

    int margin = 8 ; 

    for( int h=0; h < height ; h++ ) 
    {   
        int y = yflip ? height - 1 - h : h ;   // flip vertically

        bool y_mid = std::abs(y - height/2) < margin ; 

        for( int x=0; x < width ; ++x ) 
        {   
            bool x_mid = std::abs( x - width/2 ) < margin ; 

            if( y_mid || x_mid )            
            for( int k=0 ; k < ncomp ; k++ ) imgdata[ (h*width+x)*ncomp + k] = k < 3 ? midline_color.get(k) : 0xff ;        

        }
    } 
}



void SPPM::AddQuadline( std::vector<unsigned char>& img, const int width, const int height, const int ncomp, const bool yflip )
{
    int size = width*height*ncomp ; 
    assert( int(img.size()) == size ); 
    unsigned char* imgdata = img.data();  
    AddQuadline( imgdata, width, height, ncomp, yflip ); 
}


void SPPM::AddQuadline( unsigned char* imgdata, const int width, const int height, const int ncomp, const bool yflip )
{
    SColor quadline_color = SColors::blue ; 

    int margin = 8 ; 

    for( int h=0; h < height ; h++ ) 
    {   
        int y = yflip ? height - 1 - h : h ;   // flip vertically

        bool y_quad0 = std::abs(y - height/4) < margin ; 
        bool y_quad1 = std::abs(y - 3*height/4) < margin ; 

        for( int x=0; x < width ; ++x ) 
        {   
            bool x_quad0 = std::abs( x - width/4 ) < margin ; 
            bool x_quad1 = std::abs( x - 3*width/4 ) < margin ; 

            if( y_quad0 || y_quad1 || x_quad0 || x_quad1   )            
            for( int k=0 ; k < ncomp ; k++ ) imgdata[ (h*width+x)*ncomp + k] = k < 3 ? quadline_color.get(k) : 0xff ;        
        }
    } 
}












unsigned char* SPPM::MakeTestImage(const int width, const int height, const int ncomp, const bool yflip, const char* config)
{
    assert( ncomp == 3 ); 

    int size = width*height*ncomp ; 
    unsigned char* imgdata = new unsigned char[size] ;  
    
    for(int i=0 ; i < height ; i++){
    for(int j=0 ; j < width  ; j++){

        unsigned idx = i*width + j ;
        unsigned mi = i % 32 ; 
        unsigned mj = j % 32 ; 

        float fi = float(i)/float(height) ; 
        float fj = float(j)/float(width) ; 
  
        unsigned char ii = (1.-fi)*255.f ;   
        unsigned char jj = (1.-fj)*255.f ;   

        SColor col = SColors::white ; 
        if( SStr::Contains(config, "checkerboard") )
        {
            if( mi < 4 ) col = SColors::black ; 
            else if (mj < 4 ) col = SColors::red ; 
            else col = { jj, jj, jj } ; 
        }
        else if( SStr::Contains(config, "horizontal_gradient") )
        {
            col = { jj , jj, jj } ; 
        }    
        else if( SStr::Contains(config, "vertical_gradient") )
        {
            col = { ii , ii, ii } ; 
        }    

        imgdata[idx*ncomp+0] = col.r ; 
        imgdata[idx*ncomp+1] = col.g ; 
        imgdata[idx*ncomp+2] = col.b ; 
    }
    }


    bool add_border = SStr::Contains(config, "add_border") ; 
    bool add_midline = SStr::Contains(config, "add_midline") ; 
    bool add_quadline = SStr::Contains(config, "add_quadline") ; 

    if(add_border)  SPPM::AddBorder(imgdata, width, height, ncomp, yflip);  
    if(add_midline) SPPM::AddMidline(imgdata, width, height, ncomp, yflip);  
    if(add_quadline) SPPM::AddQuadline(imgdata, width, height, ncomp, yflip);  

    return imgdata ; 
}


unsigned SPPM::ImageCompare(const int width, const int height, const int ncomp, const unsigned char* imgdata, const unsigned char* imgdata2 )
{
    unsigned count(0); 
    unsigned mismatch(0); 
    for(int h=0 ; h < height ; h++){
    for(int w=0 ; w < width  ; w++){

        unsigned idx = h*width + w ; 
        assert( idx*ncomp == count ); 
        count += ncomp ;  

        unsigned r = idx*ncomp+0 ; 
        unsigned g = idx*ncomp+1 ; 
        unsigned b = idx*ncomp+2 ; 

        bool match = 
               imgdata[r] == imgdata2[r] && 
               imgdata[g] == imgdata2[g] && 
               imgdata[b] == imgdata2[b] ; 

        if(!match) 
        {
            mismatch++ ; 
            std::cout 
                << " h " << std::setw(3) << h 
                << " w " << std::setw(3) << w 
                << " idx " << idx
                << " mismatch " << mismatch
                << " imgdata[rgb] "
                << "(" 
                << std::setw(3) << unsigned(imgdata[r]) 
                << " "
                << std::setw(3) << unsigned(imgdata[g]) 
                << " "
                << std::setw(3) << unsigned(imgdata[b]) 
                << ")"
                << " imgdata2[rgb] "
                << "(" 
                << std::setw(3) << unsigned(imgdata2[r]) 
                << " "
                << std::setw(3) << unsigned(imgdata2[g]) 
                << " "
                << std::setw(3) << unsigned(imgdata2[b]) 
                << ")"
                << std::endl
                ;
        }
    } 
    }
    return mismatch ; 
} 



