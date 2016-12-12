#include "utils/shapeutils.h"

#include <math.h>

#include <osg/Geode>
#include <osg/Group>
#include <osg/PositionAttitudeTransform>
#include <osg/Geometry>
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/ShadeModel>

namespace ShapeUtils {
osg::Geode *createSphere(const osg::Vec4 &color, float radius)
{
    osg::TessellationHints *sphereHints = new osg::TessellationHints;
    // sphereHints->setDetailRatio(0.1f);
    // sphereHints->setCreateBody(true);
    osg::Sphere *sphere = new osg::Sphere(osg::Vec3(0, 0, 0), radius);
    osg::ShapeDrawable *sphereDrawable = new osg::ShapeDrawable(sphere, sphereHints);

    sphereDrawable->setColor(color);

    osg::Geode *geode = new osg::Geode;
    geode->addDrawable(sphereDrawable);

    return geode;
}

osg::Geode *createCube()
{
    // Declare a group to act as root node of a scene:
    // osg::Group* root = new osg::Group();

    // Declare a box class (derived from shape class) instance
    // This constructor takes an osg::Vec3 to define the center
    // and a float to define the height, width and depth.
    // (an overloaded constructor allows you to specify unique
    // height, width and height values.)
    osg::Box *unitCube = new osg::Box(osg::Vec3(0, 0, 0), 1.0f);

    // Declare an instance of the shape drawable class and initialize
    // it with the unitCube shape we created above.
    // This class is derived from 'drawable' so instances of this
    // class can be added to Geode instances.
    osg::ShapeDrawable *unitCubeDrawable = new osg::ShapeDrawable(unitCube);

    // Declare a instance of the geode class:
    osg::Geode *geode = new osg::Geode();

    // Add the unit cube drawable to the geode:
    geode->addDrawable(unitCubeDrawable);

    // osg::PolygonMode *pm = new osg::PolygonMode();
    // pm->setMode( osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE );
    //
    // osg::StateSet *stateSet = new osg::StateSet();
    // stateSet->setAttributeAndModes( pm,
    // osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON );
    // geode->setStateSet(stateSet);

    // Add the geode to the scene:
    return geode;
}

osg::PositionAttitudeTransform *createArrow(const osg::Vec4 &color)
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;

    double radius = 0.04;

    osg::TessellationHints *cylinderHints = new osg::TessellationHints;

    cylinderHints->setDetailRatio(0.1f);
    cylinderHints->setCreateBody(true);
    // cylinderHints->setCreateFrontFace(false);
    // cylinderHints->setCreateBackFace(false);
    osg::Cylinder *cylinder = new osg::Cylinder(osg::Vec3(0, 0, 0.4), radius, 0.8);
    // cylinder->setRotation(quat);
    osg::ShapeDrawable *cylinderDrawable = new osg::ShapeDrawable(cylinder, cylinderHints);
    cylinderDrawable->setColor(color);
    geode->addDrawable(cylinderDrawable);

    osg::TessellationHints *coneHints = new osg::TessellationHints;
    coneHints->setDetailRatio(0.5f);
    coneHints->setCreateBottom(true);
    osg::Cone *cone = new osg::Cone(osg::Vec3(0, 0, 0.8), radius * 2, 0.2);
    // cone->setRotation(quat);
    osg::ShapeDrawable *coneDrawable = new osg::ShapeDrawable(cone, coneHints);
    coneDrawable->setColor(color);
    geode->addDrawable(coneDrawable);

    osg::PositionAttitudeTransform *transform = new osg::PositionAttitudeTransform();
    transform->addChild(geode);

    return transform;
}

osg::Node *create3DAxis()
{
    osg::PositionAttitudeTransform *xAxis = createArrow(osg::Vec4(1, 0, 0, 1));

    xAxis->setAttitude(osg::Quat(M_PI / 2.0, osg::Vec3(0, 1, 0)));

    osg::PositionAttitudeTransform *yAxis = createArrow(osg::Vec4(0, 1, 0, 1));
    yAxis->setAttitude(osg::Quat(-M_PI / 2.0, osg::Vec3(1, 0, 0)));

    osg::PositionAttitudeTransform *zAxis = createArrow(osg::Vec4(0, 0, 1, 1));

    osg::Node *center = createSphere(osg::Vec4(0.7, 0.7, .7, 1), 0.08);

    osg::Group *group = new osg::Group();
    group->addChild(xAxis);
    group->addChild(yAxis);
    group->addChild(zAxis);
    group->addChild(center);
    return group;
}

osg::Node *createOrientatedTorus(float innerRadius, float outerRadius)
{
    osg::Node *node = createTorus(innerRadius, outerRadius, 64, 32);

    osg::PositionAttitudeTransform *transform = new osg::PositionAttitudeTransform();

    transform->addChild(node);

    osg::Quat q = osg::Quat(
        M_PI / 2, osg::Vec3d(1, 0, 0),
        0, osg::Vec3d(0, 1, 0),
        0, osg::Vec3d(0, 0, 1));
    transform->setAttitude(q);

    // transform->getOrCreateStateSet()->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
    // node->getOrCreateStateSet()->setMode(GL_RESCALE_NORMAL, osg::StateAttribute::ON);

    return transform;
}

osg::Geode *createTorus(float innerRadius, float outerRadius, float sweepCuts, float sphereCuts)
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;

    bool create_body = true;

    if (create_body) {
        float inner = innerRadius;
        float outer = outerRadius;
        float tube_radius = (outer - inner) * 0.5;
        float avg_center  = (inner + outer) * 0.5;

        float start_sweep = 0; // this->getStartSweep();
        float end_sweep   = osg::DegreesToRadians(360.0); // this->getEndSweep();

        int torus_sweeps  = sweepCuts;
        int sphere_sweeps = sphereCuts;

        float dsweep = (end_sweep - start_sweep) / (float)torus_sweeps;
        float dphi   = osg::DegreesToRadians(360.0) / (float)sphere_sweeps;

        for (int j = 0; j < sphere_sweeps; j++) {
            osg::Vec3Array *vertices = new osg::Vec3Array;
            osg::Vec3Array *normals  = new osg::Vec3Array;

            float phi    = dphi * (float)j;
            float cosPhi = cosf(phi);
            float sinPhi = sinf(phi);
            float next_cosPhi = cosf(phi + dphi);
            float next_sinPhi = sinf(phi + dphi);

            float z = tube_radius * sinPhi;
            float yPrime = avg_center + tube_radius * cosPhi;

            float next_z = tube_radius * next_sinPhi;
            float next_yPrime = avg_center + tube_radius * next_cosPhi;

            float old_x  = yPrime * cosf(-dsweep);
            float old_y  = yPrime * sinf(-dsweep);
            float old_z  = z;

            for (int i = 0; i < torus_sweeps; ++i) {
                float sweep    = start_sweep + dsweep * i;
                float cosSweep = cosf(sweep);
                float sinSweep = sinf(sweep);

                float x = yPrime * cosSweep;
                float y = yPrime * sinSweep;

                float next_x = next_yPrime * cosSweep;
                float next_y = next_yPrime * sinSweep;

                vertices->push_back(osg::Vec3(next_x, next_y, next_z));
                vertices->push_back(osg::Vec3(x, y, z));

                // calculate normals
                osg::Vec3 lateral(next_x - x, next_y - y, next_z - z);
                osg::Vec3 longitudinal(x - old_x, y - old_y, z - old_z);
                osg::Vec3 normal = longitudinal ^ lateral; // cross product
                normal.normalize();

                normals->push_back(normal);
                normals->push_back(normal);

                old_x = x;
                old_y = y;
                old_z = z;
            } // end torus loop

            // the last point
            float last_sweep   = start_sweep + end_sweep;
            float cosLastSweep = cosf(last_sweep);
            float sinLastSweep = sinf(last_sweep);

            float x = yPrime * cosLastSweep;
            float y = yPrime * sinLastSweep;

            float next_x = next_yPrime * cosLastSweep;
            float next_y = next_yPrime * sinLastSweep;

            vertices->push_back(osg::Vec3(next_x, next_y, next_z));
            vertices->push_back(osg::Vec3(x, y, z));

            osg::Vec3 lateral(next_x - x, next_y - y, next_z - z);
            osg::Vec3 longitudinal(x - old_x, y - old_y, z - old_z);
            osg::Vec3 norm = longitudinal ^ lateral;
            norm.normalize();

            normals->push_back(norm);
            normals->push_back(norm);

            osg::ShadeModel *shademodel = new osg::ShadeModel;
            shademodel->setMode(osg::ShadeModel::SMOOTH);

            osg::StateSet *stateset     = new osg::StateSet;
            stateset->setAttribute(shademodel);

            osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
            geometry->setStateSet(stateset);

            geometry->setVertexArray(vertices);

            osg::Vec4Array *colors = new osg::Vec4Array;
            colors->push_back(osg::Vec4(1.0, 1.0, 0.0, 1.0)); // this->getColor());
            geometry->setColorArray(colors);
            geometry->setColorBinding(osg::Geometry::BIND_OVERALL);

            geometry->setNormalArray(normals);
            geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);

            geometry->addPrimitiveSet(
                new osg::DrawArrays(osg::PrimitiveSet::QUAD_STRIP, 0,
                                    vertices->size()));
            geode->addDrawable(geometry.get());
        } // end cirle loop
    } // endif create_body

    return geode.release();
}

osg::Geode *createRhombicuboctahedron()
{
    // Top           *
    // Bottom        o
    // Front         ^
    // Stern         v
    // Left          <
    // Right         >
    const double alpha = 1; // 0.65;
    const osg::Vec4 colors[] = {
        osg::Vec4(0.7, 0.5, 0.7, alpha), // TFR
        osg::Vec4(0.9, 0.9, 0.9, alpha), // T
        osg::Vec4(0.5, 0.7, 0.7, alpha), // TFL
        osg::Vec4(1.0, 0.7, 0.7, alpha), // TS
        osg::Vec4(0.7, 0.7, 1.0, alpha), // TF
        osg::Vec4(0.5, 0.0, 0.7, alpha), // BFR
        osg::Vec4(0.5, 0.5, 1.0, alpha), // F
        osg::Vec4(0.1, 0.5, 0.7, alpha), // BFL
        osg::Vec4(0.9, 0.5, 0.9, alpha), // TR
        osg::Vec4(0.5, 0.0, 0.5, alpha), // R
        osg::Vec4(1.0, 0.5, 0.7, alpha), // TSR
        osg::Vec4(0.2, 0.0, 0.2, alpha), // BR
        osg::Vec4(0.2, 0.2, 1.0, alpha), // BF
        osg::Vec4(0.7, 0.0, 0.2, alpha), // BSR
        osg::Vec4(0.1, 0.1, 0.1, alpha), // B
        osg::Vec4(0.5, 0.5, 0.2, alpha), // BSL
        osg::Vec4(0.9, 0.0, 0.4, alpha), // SR
        osg::Vec4(1.0, 0.2, 0.2, alpha), // BS
        osg::Vec4(1.0, 0.5, 0.5, alpha), // S
        osg::Vec4(0.7, 0.7, 0.5, alpha), // SL
        osg::Vec4(0.5, 0.9, 0.5, alpha), // TL
        osg::Vec4(0.5, 0.7, 0.5, alpha), // L
        osg::Vec4(1.0, 1.0, 0.7, alpha), // TSL
        osg::Vec4(0.2, 0.5, 0.2, alpha), // BL
        osg::Vec4(0.5, 0.0, 0.8, alpha), // FR
        osg::Vec4(0.5, 0.7, 1.0, alpha) // FL
    };

    const double no = -1.0;
    const double po = 1.0f;
    const double ps = 1.0 + sqrt(1.0);
    const double ns = -ps;
    const osg::Vec3 vertices[] = {
        osg::Vec3(po, po, ps),
        osg::Vec3(po, no, ps),
        osg::Vec3(no, po, ps),
        osg::Vec3(no, no, ps),

        osg::Vec3(po, ps, po),
        osg::Vec3(po, ps, no),
        osg::Vec3(no, ps, po),
        osg::Vec3(no, ps, no),

        osg::Vec3(ps, po, po),
        osg::Vec3(ps, po, no),
        osg::Vec3(ps, no, po),
        osg::Vec3(ps, no, no),

        osg::Vec3(po, po, ns),
        osg::Vec3(po, no, ns),
        osg::Vec3(no, po, ns),
        osg::Vec3(no, no, ns),

        osg::Vec3(po, ns, po),
        osg::Vec3(po, ns, no),
        osg::Vec3(no, ns, po),
        osg::Vec3(no, ns, no),

        osg::Vec3(ns, po, po),
        osg::Vec3(ns, po, no),
        osg::Vec3(ns, no, po),
        osg::Vec3(ns, no, no),

        // bonus vertices to map unique colors
        // we have only 24 regular vertices, but 26 faces
        osg::Vec3(ps, po, no), // copy from vertex 9
        osg::Vec3(no, ps, no) // copy from vertex 7
    };

    const unsigned int indices[] = {
        // PITCH   ROLL   YAW
        8,  4,  0, // TFR       45      -45

        0,  2,  1, // T1       0       0
        3,  2,  1, // T2

        6,  20, 2, // TFL      45      45

        1,  16, 3, // TS1       -45     0
        18, 16, 3, // TS2

        0,  2,  4, // TF1      45      0
        6,  2,  4, // TF2

        12, 9,  5, // BFR       45      -135

        4,  5,  6, // F1       90      X
        7,  5,  6, // F2

        14, 21, 7, // BFL       45      135

        0,  1,  8, // TR1       0       -45
        10, 1,  8, // TR2

        8,  10, 9, // R1       0       -90
        11, 10, 9, // R2

        1,  16, 10, // TSR       -45     -45
        // PITCH    ROLL   YAW
        13, 12, 11, // BR1       0       -135
        9,  12, 11, // BR2

        5,  7,  12, // BF1       45      +-180
        14, 7,  12, // BF2

        11, 17, 13, // BSR       -45     -135

        12, 13, 14, // B1       0       +-180
        15, 13, 14, // B2

        19, 23, 15, // BSL       -45     135

        11, 17, 16, // SR1       -45     -90
        11, 10, 16, // SR2

        13, 15, 17, // BS1       -45     +-180
        19, 15, 17, // BS2

        16, 17, 18, // S1        -90     X
        19, 17, 18, // S2

        18, 22, 19, // SL        -45     90
        22, 23, 19, // SL
        // PITCH    ROLL   YAW
        2,  3,  20, // TL1       0       45
        22, 3,  20, // TL2

        20, 22, 21, // L1        0       90
        23, 22, 21, // L2

        3,  18, 22, // TSL       -45     45

        14, 15, 23, // BL1       0       135
        14, 21, 23, // BL2

        // last vertex is equal to vertex 9 to map a unique color
        4,  5,  24, // FR        45      -90
        4,  8,  24, // FR

        // last vertex is equal to vertex 7 to map a unique color
        6,  20, 25, // FL        45      90
        21, 20, 25 // FL
    };

    osg::ShadeModel *shademodel = new osg::ShadeModel;

    shademodel->setMode(osg::ShadeModel::FLAT);

    osg::StateSet *stateset = new osg::StateSet;
    stateset->setAttribute(shademodel);

    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;

    geom->setStateSet(stateset);

    unsigned int vertexCount    = sizeof(vertices) / sizeof(vertices[0]);
    osg::Vec3Array *vertexArray = new osg::Vec3Array(vertexCount, const_cast<osg::Vec3 *>(&vertices[0]));
    geom->setVertexArray(vertexArray);

    unsigned int indexCount     = sizeof(indices) / sizeof(indices[0]);
    geom->addPrimitiveSet(new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, indexCount, indices));

    unsigned int colorCount     = sizeof(colors) / sizeof(colors[0]);
    osg::Vec4Array *colorArray  = new osg::Vec4Array(colorCount, const_cast<osg::Vec4 *>(&colors[0]));
    geom->setColorArray(colorArray);
    geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    // geometry->setNormalArray(normals);
    // geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geom.get());

    return geode.release();
}
}
