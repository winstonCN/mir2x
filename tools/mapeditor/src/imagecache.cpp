/*
 * =====================================================================================
 *
 *       Filename: imagecache.cpp
 *        Created: 02/14/2016 15:40:17
 *    Description:
 *
 *        Version: 1.0
 *       Revision: none
 *       Compiler: gcc
 *
 *         Author: ANHONG
 *          Email: anhonghe@gmail.com
 *   Organization: USTC
 *
 * =====================================================================================
 */

#include "colorf.hpp"
#include "imagedb.hpp"
#include "imagecache.hpp"

extern ImageDB *g_imageDB;
Fl_Image *ImageCache::retrieve(uint32_t imageIndex)
{
    if(auto p = m_cache.find(imageIndex); p != m_cache.end()){
        return p->second.get();
    }

    if(const auto [imgBuf, imgWidth, imgHeight] = g_imageDB->decode(imageIndex, 0XFFFFFFFF, 0XFFFFFFFF, 0XFFFFFFFF); imgBuf){
        // TODO hack the color
        // The RGBA model used in FLTK is actually I called ABGR
        m_convBuf.resize(imgWidth * imgHeight, 0);
        for(size_t i = 0; i < imgWidth * imgHeight; ++i){
            m_convBuf[i] = colorf::RGBA2ABGR(imgBuf[i]);
        }

        // understand the Fl_Image and Fl_RGB_Image:
        // Fl_RGB_Image needs a RGBA buffer as constructor parameter, but it won't create its internal copy
        // caller has to make sure the buffer is available and not changed

        // but Fl_Image doesn't require such a buffer
        // immediately after Fl_RGB_Image::copy() returns the Fl_Image pointer
        // we are good to use the RGBA buffer for other purpose
        return (m_cache[imageIndex] = std::unique_ptr<Fl_Image>(Fl_RGB_Image((const uchar *)(m_convBuf.data()), imgWidth, imgHeight, 4).copy())).get();
    }

    // can't load from imageDB
    // put a place holder, to prevent next load

    m_cache[imageIndex] = {};
    return nullptr;
}