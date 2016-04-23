/**
 ******************************************************************************
 *
 * @file       qtwindowingsystem.h
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

#ifndef QTWINDOWINGSYSTEM_H
#define QTWINDOWINGSYSTEM_H

#include "osgearth_global.h"

#include <osg/GraphicsContext>

class QtWindowingSystem : public osg::GraphicsContext::WindowingSystemInterface {
public:

    QtWindowingSystem();
    ~QtWindowingSystem();

    // Access the Qt windowing system through this singleton class.
    static QtWindowingSystem *getInterface()
    {
        static QtWindowingSystem *qtInterface = new QtWindowingSystem;

        return qtInterface;
    }

    // Return the number of screens present in the system
    virtual unsigned int getNumScreens(const osg::GraphicsContext::ScreenIdentifier & /*si*/);

    // Return the resolution of specified screen
    // (0,0) is returned if screen is unknown
    virtual void getScreenSettings(const osg::GraphicsContext::ScreenIdentifier & /*si*/, osg::GraphicsContext::ScreenSettings & /*resolution*/);

    // Set the resolution for given screen
    virtual bool setScreenSettings(const osg::GraphicsContext::ScreenIdentifier & /*si*/, const osg::GraphicsContext::ScreenSettings & /*resolution*/);

    // Enumerates available resolutions
    virtual void enumerateScreenSettings(const osg::GraphicsContext::ScreenIdentifier & /*screenIdentifier*/, osg::GraphicsContext::ScreenSettingsList & /*resolution*/);

    // Create a graphics context with given traits
    virtual osg::GraphicsContext *createGraphicsContext(osg::GraphicsContext::Traits *traits);

private:

    // No implementation for these
    QtWindowingSystem(const QtWindowingSystem &);
    QtWindowingSystem & operator=(const QtWindowingSystem &);
};

#endif // QTWINDOWINGSYSTEM_H
