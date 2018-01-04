#include "pca.h"

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_eigen.h>

using geometry::Point3d;
using geometry::Matrix3d;

PCA3Dresult PCA(const std::vector<Point3d>& points)
{
  Matrix3d corr;
  size_t n = points.size();
  Point3d center;
  for(const Point3d& pos: points)
  {
    center += pos;
  }
  center /= n;
  for(const Point3d& pos: points)
  {
    Point3d dp = pos-center;
    corr(0,0) += dp.x()*dp.x();
    corr(1,1) += dp.y()*dp.y();
    corr(2,2) += dp.z()*dp.z();
    corr(1,0) += dp.x()*dp.y();
    corr(2,0) += dp.x()*dp.z();
    corr(2,1) += dp.y()*dp.z();
  }
  corr /= n; // n-1 ???
  gsl_matrix *mat = gsl_matrix_alloc(3, 3);
  for(int i = 0 ; i < 3 ; ++i)
  {
    gsl_matrix_set(mat, i, i, corr(i,i));
    for(int j = 0 ; j < i ; ++j)
    {
      gsl_matrix_set(mat, i, j, corr(i,j));
      gsl_matrix_set(mat, j, i, corr(i,j));
    }
  }
  gsl_vector *eval = gsl_vector_alloc(3);
  gsl_matrix *evec = gsl_matrix_alloc(3,3);
  gsl_eigen_symmv_workspace *w = gsl_eigen_symmv_alloc(3);
  gsl_eigen_symmv(mat, eval, evec, w);
  gsl_eigen_symmv_free(w);
  gsl_eigen_symmv_sort(eval, evec, GSL_EIGEN_SORT_VAL_DESC);

  PCA3Dresult result;

  result.ev1 = Point3d(gsl_matrix_get(evec, 0, 0),
                       gsl_matrix_get(evec, 1, 0),
                       gsl_matrix_get(evec, 2, 0));
  result.ev2 = Point3d(gsl_matrix_get(evec, 0, 1),
                       gsl_matrix_get(evec, 1, 1),
                       gsl_matrix_get(evec, 2, 1));
  result.ev3 = Point3d(gsl_matrix_get(evec, 0, 2),
                       gsl_matrix_get(evec, 1, 2),
                       gsl_matrix_get(evec, 2, 2));
  result.eval = Point3d(gsl_vector_get(eval, 0),
                        gsl_vector_get(eval, 1),
                        gsl_vector_get(eval, 2));
  result.center = center;

  gsl_vector_free(eval);
  gsl_matrix_free(evec);
  gsl_matrix_free(mat);

  return result;
}

