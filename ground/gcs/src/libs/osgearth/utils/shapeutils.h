#ifndef SHAPE_UTILS_H
#define SHAPE_UTILS_H

#include <osg/Vec4>

namespace osg {
class Node;
class Geode;
class PositionAttitudeTransform;
}

namespace ShapeUtils {
osg::Geode *createCube();
osg::Geode *createSphere(const osg::Vec4 &color, float radius);
osg::PositionAttitudeTransform *createArrow(const osg::Vec4 &color);
osg::Node *create3DAxis();
osg::Node *createOrientatedTorus(float innerRadius, float outerRadius);
osg::Geode *createTorus(float innerRadius, float outerRadius, float sweepCuts, float sphereCuts);
osg::Geode *createRhombicuboctahedron();
}

#endif /* SHAPE_UTILS_H */
