#include <Eigen/Dense>

#include "halma.h"

using namespace Eigen;

void halma()
{
    Matrix<float, 3, 3> A;
    A.setZero();
    printf("Test %f %f\n", A(0, 2), A(1, 1));
}
