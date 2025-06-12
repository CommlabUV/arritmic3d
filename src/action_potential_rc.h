/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 *
 * @file action_potential.h
 *
 * @brief Classes to implement the action potential evolution of a cell.
 *
 * Essentially, it is the evolution of the potential along time.
 * The basic interface includes a method to get the potential at a given time,
 * and a method to recompute future potential after an activation.
 *
 * Several models to implement the action potential are available.
 *
 * */

#ifndef ACTION_POTENTIAL_RC_H
#define ACTION_POTENTIAL_RC_H

//#include "membrane_potential.h"
#include "spline.h"
#include "node.h"

/**
 * @brief Action potential model based on APD restitution curves.
 */
class ActionPotentialRestCurve
{
public:
    /**
     * @brief Default constructor.
     *
     */
    ActionPotentialRestCurve()
    {
        this->apd = 300.0;
        this->ta = 0.0;
    };

    /**
     * @brief Constructor.
     *
     * @param type Cell type.
     * @param region Tissue region.
     * @param apd_ Action potential duration.
     * @param t0_ Time of the activation.
     * @param di_ Diastolic interval.
     */
    ActionPotentialRestCurve(CellType type, TissueRegion region, float apd_, float t0_, float di_ = 0.0)
    {
        Init(type, region, apd_, t0_, di_);
    };

    /**
     * @brief Initialize the action potential.
     *
     * @param type Cell type.
     * @param region Tissue region.
     * @param apd_ Action potential duration.
     * @param t0_ Time of the activation.
     * @param di_ Diastolic interval.
     */
    void Init(CellType type, TissueRegion region, float apd_, float t0_, float di_ = 0.0)
    {
        SetRestitutionCurve(type, region);
        this->apd = apd_;
        this->ta = t0_ - (apd_ + di_);
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
     * @brief Recompute the action potential after an activation.
     *
     * @param new_ta New activation time.
     */
    bool Activate(float new_ta)
    {
        float di = new_ta -(this->ta + this->apd);
        if(di < 0.0)
            return false;


        this->apd = restitution_curve->getValue(di);
        this->ta = new_ta;
        return true;
    };

    /**
     * @brief Recompute the action potential after an activation taking
     * into account the electrotonic effect.
     *
     * @param new_ta New activation time.
     * @param avg_apd Average action potential duration.
     * @param e_eff Electrotonic effect.
     */
    bool Activate(float new_ta, float avg_apd, float e_eff = 0.0)
    {
        bool res = Activate(new_ta);
        this->apd = this->apd*(1.0 - e_eff) + avg_apd*e_eff;
        return res;
    };

    /**
     * @brief Get the action potential at a given time.
     *
     * @param t_ Time.
     * @return The action potential at time t.
     */
    float getActionPotential(float t_) const
    {
        if(t_ < this->ta or t_ > this->ta + this->apd)
            return this->resting_potential;
        else
            return this->peak_potential;
    };

    /**
     * @brief Check if the cell is active at time t.
    */
    bool IsActive(float t_) const
    {
        return (t_ >= this->ta and t_ < this->ta + this->apd);
    };

    /**
     * @brief Get the action potential duration.
     *
     * @return The action potential duration.
     */
    float getAPD() const
    {
        return this->apd;
    };

    /**
     * @brief Get the activation time.
     *
     * @return The activation time.
     */
    float getActivationTime() const
    {
        return this->ta;
    };

    /**
     * @brief Get the diastolic interval at time t
     *
     * @param t Time.
     * @return The diastolic interval at time t.
     */
    float getDI(float t) const
    {
        return t - (this->ta + this->apd);
    };


private:
    float apd; /**< Action potential duration. */
    float ta; /**< Time of the activation. */
    float conduction_velocity; /**< Conduction velocity. */

    Spline * restitution_curve; /**< APD restitution curve. */
    static SplineContainer splines; /**< Container of APD restitution curves. */

    static constexpr bool normalized_potential = false; /**< Whether the potential is normalized. */
    static constexpr float resting_potential = -80.0; // 0.0; /**< Resting potential. */
    static constexpr float peak_potential = 40.0; // 1.0;  /**< Peak potential. */


};

const std::string path = "restitutionCurves/";

SplineContainer ActionPotentialRestCurve::splines({{CellType::HEALTHY, TissueRegion::ENDO, path + "RestitutionCurve_Sanas_APD_Endo.csv"},
                                                    {CellType::HEALTHY, TissueRegion::MID, path + "RestitutionCurve_Sanas_APD_Mid.csv"},
                                                   {CellType::HEALTHY, TissueRegion::EPI, path + "RestitutionCurve_Sanas_APD_Epi.csv"},
                                                   {CellType::BORDER_ZONE, TissueRegion::ENDO, path + "RestitutionCurve_BZ_APD_Endo.csv"},
                                                   {CellType::BORDER_ZONE, TissueRegion::MID, path + "RestitutionCurve_BZ_APD_Mid.csv"},
                                                   {CellType::BORDER_ZONE, TissueRegion::EPI, path + "RestitutionCurve_BZ_APD_Epi.csv"}
                                                   });

#endif
