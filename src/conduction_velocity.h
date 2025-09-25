/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 *
 * @file conduction_velocity.h
 *
 * @brief Classes to implement the conduction velocity evolution of a cell.
 *
 * Essentially, it is the evolution of the conduction velocity along time.
 * The basic interface includes a method to get the conduction velocity at a given time.
 *
 * */

#ifndef CONDUCTION_VELOCITY_H
#define CONDUCTION_VELOCITY_H

#include "spline.h"
#include "node.h"

/**
 * @brief Conduction velocity model based on step function.
 *
 * This model considers constant CV between updates.
 */
class ConductionVelocity
{
public:
    /**
     * @brief Default constructor.
     *
     */
    ConductionVelocity()
    {
        this->cv = INITIAL_CV;
        this->correction_factor = 1.0;
    };


    /**
     * @brief Constructor.
     *
     * @param type Cell type.
     * @param region Tissue region.
     * @param corrfc_ Correction factor for restitution curves.
     */
    ConductionVelocity(CellType type, TissueRegion region, float corrfc_ = 1.0)
    {
        Init(type, region);
    };


    /**
     * @brief Initialize the conduction velocity.
     *
     * @param type Cell type.
     * @param region Tissue region.
     * @param corrfc_ Correction factor for restitution curves.
     */
    void Init(CellType type, TissueRegion region, float corrfc_ = 1.0)
    {
        SetRestitutionCurve(type, region);
        this->correction_factor = corrfc_;
        this->cv = INITIAL_CV;
    };

    /**
     * @brief Set the restitution curve.
     *
     * @param type Cell type.
     * @param region Tissue region.
     */
    void SetRestitutionCurve(CellType type, TissueRegion region)
    {
        this->restitution_curve = splines.getSpline(type, region);
    };

    /**
     * @brief Recompute the conduction velocity after an activation.
     *
     * @param di Diastolic interval.
     */
    void Activate(float di)
    {
        this->cv = this->restitution_curve->getValue(di)*this->correction_factor;
    };

   /**
     * @brief Recompute the conduction velocity after an activation taking
     * into account the electrotonic effect.
     *
     * @param di Diastolic interval.
     * @param avg_cv Average conduction velocity.
     * @param e_eff Electrotonic effect.
     */
    void Activate(float di, float avg_cv, float e_eff = 0.0)
    {
        Activate(di);
        this->cv = this->cv*(1.0 - e_eff) + avg_cv*e_eff;
    };

    /**
     * @brief Get the conduction velocity.
     *
     * @return The conduction velocity.
     */
    float getConductionVelocity() const
    {
        return this->cv;
    };

private:

    float cv; ///< Conduction velocity.
    static constexpr float INITIAL_CV = 1.0; ///< Initial conduction velocity.

    Spline * restitution_curve; ///< APD restitution curve.
    static SplineContainer splines; ///< Container of APD restitution curves.

    float correction_factor; /**< Correction factor for restitution curves. */

};

const std::string path_cv = "restitutionCurves/";

SplineContainer ConductionVelocity::splines({{CellType::HEALTHY, TissueRegion::ENDO, path_cv + "RestitutionCurve_Sanas_CV_Endo.csv"},
                                                    {CellType::HEALTHY, TissueRegion::MID, path_cv + "RestitutionCurve_Sanas_CV_Mid.csv"},
                                                   {CellType::HEALTHY, TissueRegion::EPI, path_cv + "RestitutionCurve_Sanas_CV_Epi.csv"},
                                                   {CellType::BORDER_ZONE, TissueRegion::ENDO, path_cv + "RestitutionCurve_BZ_CV_Endo.csv"},
                                                   {CellType::BORDER_ZONE, TissueRegion::MID, path_cv + "RestitutionCurve_BZ_CV_Mid.csv"},
                                                   {CellType::BORDER_ZONE, TissueRegion::EPI, path_cv + "RestitutionCurve_BZ_CV_Epi.csv"}
                                                   });

#endif // CONDUCTION_VELOCITY_H