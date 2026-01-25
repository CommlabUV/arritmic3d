/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */

#ifndef DEF_H
#define DEF_H

#include <vector>
#include <limits>
#include <eigen3/Eigen/Dense>

constexpr float ALMOST_ZERO = 1e-8;
constexpr float MAX_TIME = std::numeric_limits<float>::max();

typedef unsigned short CellType; ///< @brief Cell type
constexpr CellType CELL_TYPE_VOID = 0;

#endif // DEF_H
