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
