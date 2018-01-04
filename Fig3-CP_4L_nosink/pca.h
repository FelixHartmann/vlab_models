#ifndef PCA_H
#define PCA_H

#include <geometry/geometry.h>

struct PCA3Dresult
{
  geometry::Point3d ev1, ev2, ev3;
  geometry::Point3d eval, center;
};

PCA3Dresult PCA(const std::vector<geometry::Point3d>& points);

#endif // PCA_H

