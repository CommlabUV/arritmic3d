/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 * */

#ifndef DEF_H
#define DEF_H

#include <vector>
#include <eigen3/Eigen/Dense>

const float ALMOST_ZERO = 1e-8;

enum class CellType : char { HEALTHY, BORDER_ZONE, CORE}; ///< @brief Cell type

enum class TissueRegion : char { ENDO, MID, EPI}; ///< @brief endo (0) / mid (1) / epi (2)

#endif // DEF_H
