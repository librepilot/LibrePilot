/**
 ******************************************************************************
 *
 * @file       Export.hpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 * @addtogroup
 * @{
 * @addtogroup
 * @{
 * @brief
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _H_OSGQTQUICK_EXPORT_H_
#define _H_OSGQTQUICK_EXPORT_H_ 1

#include <osg/Config>

#if defined(_MSC_VER) && defined(OSG_DISABLE_MSVC_WARNINGS)
    #pragma warning( disable : 4244 )
    #pragma warning( disable : 4251 )
    #pragma warning( disable : 4267 )
    #pragma warning( disable : 4275 )
    #pragma warning( disable : 4290 )
    #pragma warning( disable : 4786 )
    #pragma warning( disable : 4305 )
    #pragma warning( disable : 4996 )
#endif

#if defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BCPLUSPLUS__) || defined(__MWERKS__)
    #  if defined(OSGEARTH_LIBRARY_STATIC)
    #    define OSGQTQUICK_EXPORT
    #  elif defined(OSGEARTH_LIBRARY)
    #    define OSGQTQUICK_EXPORT __declspec(dllexport)
    #  else
    #    define OSGQTQUICK_EXPORT __declspec(dllimport)
    #endif
#else
    #define OSGQTQUICK_EXPORT
#endif

#endif // _H_OSGQTQUICK_EXPORT_H_
