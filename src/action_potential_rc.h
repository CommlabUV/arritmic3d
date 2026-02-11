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
#include <cmath>

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
        this->last_di = 100.0;
        this->ta = 0.0;
        this->correction_factor = 1.0;
        this->apd = 0.0;
    };

    /**
     * @brief Constructor.
     *
     * @param type Cell type.
     * @param apd_ Action potential duration.
     * @param t0_ Time of the activation.
     * @param di_ Diastolic interval.
     * @param corrfc_ Correction factor for restitution curves.
     */
    ActionPotentialRestCurve(CellType type, float apd_, float t0_, float di_ = 0.0, float corrfc_ = 1.0)
    {
        Init(type, apd_, t0_, di_, corrfc_);
    };

    static void InitModel(const std::string &path)
    {
        splines.Init(path);
    }

    /**
     * @brief Initialize the action potential.
     *
     * @param type Cell type.
     * @param apd_ Action potential duration.
     * @param t0_ Time of the activation.
     * @param di_ Diastolic interval.
     * @param corrfc_ Correction factor for restitution curves.
     */
    void Init(CellType type, float apd_, float t0_, float di_ = 0.0, float corrfc_ = 1.0)
    {
        SetRestitutionCurve(type);
        this->correction_factor = corrfc_;
        if (di_ > 0.0)
        {
            this->last_di = di_;
            this->apd = restitution_curve->getValue(this->last_di)*this->correction_factor;
        }
        else
        {
            this->last_di = 100.0; /// @todo Should be a reverse mapping from apd_ to di_
            this->apd = apd_;
        }
        this->ta = t0_ - (this->apd + this->last_di);
    };

    /**
     * @brief Set the restitution curve.
     *
     * @param type Cell type.
     */
    void SetRestitutionCurve(CellType type)
    {
        this->restitution_curve = splines.getSpline(type);
        if(this->restitution_curve == nullptr && type != CELL_TYPE_VOID)
            throw std::runtime_error("action_potential_rc.h: no APD restitution model found for cell type " + std::to_string(static_cast<int>(type)));
    };

    /**
     * @brief Recompute the action potential after an activation.
     *
     * @todo Consider negative diastolic intervals.
     * @param new_ta New activation time.
     */
    bool Activate(float new_ta)
    {
        float di = new_ta - (this->ta + this->apd);

        float new_apd = restitution_curve->getValue(di)*this->correction_factor;
        if(new_apd <= 0.0)
            return false;

        this->last_di = di;
        this->delta_apd = std::fabs(new_apd - this->apd);    // Calculated without electrotonic effect !!
        this->apd = new_apd;
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
        if( ! Activate(new_ta) )
            return false;
        this->apd = this->apd*(1.0 - e_eff) + avg_apd*e_eff;
        return true;
    };

    /**
     * @brief Get the action potential at a given time.
     *
     * @param t_ Time.
     * @return The action potential at time t.
     */
    float getActionPotential(float t_) const
    {
        if(t_ < this->ta || t_ > this->ta + this->apd)
            return this->resting_potential;
        else
            return this->peak_potential;
    };

    /**
     * @brief Check if the cell is active at time t.
    */
    bool IsActive(float t_) const
    {
        return (t_ >= this->ta && t_ < this->ta + this->apd);
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

    float getERP() const
    {
        //return this->apd + restitution_model->GetLabelNoValue(0, this->apd);
        return this->apd;
    }

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

    /**
     * @brief Get the diastolic interval of the last activation.
     *
     * @return The diastolic interval of the last activation.
     */
    float getLastDI() const
    {
        return this->last_di;
    };

    /**
     * @brief Get the normalized life time of the action potential.
     *
     * LT is a value between 0 and 1 that indicates how long the cell
     * has been active, normalized by its APD.
     *
     * @return The normalized life time of the action potential. It
     * is 0 if the cell is inactive and 1 if the cell has been
     * active for a time equal to its APD.
     */
    float getLife(float t) const
    {
        if(this->apd <= 0.0)
            return 0.0;
        if(t < this->ta)
            return 0.0;
        else if(t >= this->ta + this->apd)
            return 1.0;
        else
            return (t - this->ta) / this->apd;
    };


    /**
     * @brief Get the variation in APD due to restitution curves (without electrotonic effect).
     *
     * @return The variation in APD.
     */
    float getDeltaAPD() const
    {
        return this->delta_apd;
    };

private:
    float apd; /**< Action potential duration. */
    float ta; /**< Time of the activation. */
    float last_di; /**< Last diastolic interval. */
    float delta_apd; ///< Change in APD due to restitution curves (without electrotonic effect).
    float correction_factor; /**< Correction factor for restitution curves. */

    Spline * restitution_curve; /**< APD restitution curve. */
    static SplineContainer splines; /**< Container of APD restitution curves. */

    static constexpr bool normalized_potential = false; /**< Whether the potential is normalized. */
    static constexpr float resting_potential = -80.0; // 0.0; /**< Resting potential. */
    static constexpr float peak_potential = 40.0; // 1.0;  /**< Peak potential. */
    static std::string config_file; /**< Configuration file for the model. */

};

std::string ActionPotentialRestCurve::config_file = "";

SplineContainer ActionPotentialRestCurve::splines;

#endif
