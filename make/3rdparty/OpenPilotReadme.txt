OpenSceneGraph and OSGEarth libs build for use with the OpenPilot project, normally we would include these libs as source inside the OpenPilot git as we have done with other libs such as Qwt, GLCLib etc, this is the aim eventually for OSG and OSGEarth and we are working towards that goal. However, as OSG has several dependencies this needs time, due to the hobby nature of the project we prefer to get the interesting stuff released to people rather than work on complicated cross platform build scripts and solving dependencies for people. To that end we are using these compiled libs in the short term to make it easier to build the OpenPilot GCS until we can integrate the source in to our own source tree. OSG and OSGEarth libs in this package were build using vanilla and unchanged code for their respective SCRs.

 

Exact source unmodified source used to compile these libs can be found: 
 

http://github.com/openscenegraph/osg and git://github.com/gwaldron/osgearth.git

 

Important Note: These libs include OpenPilot's license for Sundog's Silverlining OSGEarth Plugin, hence these libs can only be used to compile and link against OpenPilot's code base and no other, do not pirate this Sundog license by using these libs for any other project or purpose. This issue is simple to resolved by a recompile of your own version of these libs anyway. As stated, in time we will also integrate OSG and OSGEarth in to our own codebase to be built with the OpenPilot GCS where the Silverlining license can be changed or removed, until that time these pre-compiled libs are only licensed to be used with the OpenPIlot project.