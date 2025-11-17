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

#ifndef ACTION_POTENTIAL_RS_H
#define ACTION_POTENTIAL_RS_H

//#include "membrane_potential.h"
#include "spline2D.h"
#include "node.h"

/**
 * @brief Action potential model based on APD restitution models.
 */
class ActionPotentialRestSurface
{
public:
    /**
     * @brief Default constructor.
     *
     */
    ActionPotentialRestSurface()
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
     * @param corrfc_ Correction factor for restitution models.
     */
    ActionPotentialRestSurface(CellType type, float apd_, float t0_, float di_ = 0.0, float corrfc_ = 1.0)
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
     * @param corrfc_ Correction factor for restitution models.
     */
    void Init(CellType type, float apd_, float t0_, float di_ = 0.0, float corrfc_ = 1.0)
    {
        SetRestitutionModel(type);
        this->correction_factor = corrfc_;
        if (di_ > 0.0)
        {
            this->last_di = di_;
            // The next call can return NaN
            float new_apd = restitution_model->getValue(this->last_di,apd_)*this->correction_factor;
            // Check invalid value
            if(! restitution_model->is_novalue(new_apd) )
                this->apd = new_apd;
            else
                this->apd = apd_;
        }
        else
        {
            this->last_di = 100.0; /// @todo Should be a reverse mapping from apd_ to di_
            this->apd = apd_;
        }
        this->ta = t0_ - (this->apd + this->last_di); ///< @todo Check if this is correct, this->apd is future
    };

    /**
     * @brief Set the restitution model.
     *
     * @param type Cell type.
     */
    void SetRestitutionModel(CellType type)
    {
        this->restitution_model = splines.getSpline(type);
        if(this->restitution_model == nullptr and type != CELL_TYPE_VOID)
            throw std::runtime_error("action_potential_rc.h: : no APD restitution model found for cell type " + std::to_string(static_cast<int>(type)));
    };

    /**
     * @brief Recompute the action potential after an activation.
     *
     * @param new_ta New activation time.
     */
    bool Activate(float new_ta)
    {
        bool activated = false;
        float di = new_ta -(this->ta + this->apd);
        if(di < 0.0)
            return false;

        //float new_apd = restitution_model->getValue(di,this->apd)*this->correction_factor;
        float new_apd = restitution_model->getValue(this->apd, di)*this->correction_factor;
        if (! restitution_model->is_novalue(new_apd) )
        {
            this->last_di = di;
            this->delta_apd = fabs(new_apd - this->apd);    // Calculated without electrotonic effect !!
            this->apd = new_apd;
            this->ta = new_ta;
            activated = true;
        }
        return activated;
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

    float getERP() const
    {
        return this->apd + restitution_model->GetLabelNoValue(0, this->apd);
        //return this->apd;
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
     * @brief Get the variation in APD due to restitution models (without electrotonic effect).
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
    float delta_apd; ///< Change in APD due to restitution models (without electrotonic effect).
    float correction_factor; /**< Correction factor for restitution models. */

    Spline2D * restitution_model; /**< APD restitution model. */
    static SplineContainer2D splines; /**< Container of APD restitution models. */

    static constexpr bool normalized_potential = false; /**< Whether the potential is normalized. */
    static constexpr float resting_potential = -80.0; // 0.0; /**< Resting potential. */
    static constexpr float peak_potential = 40.0; // 1.0;  /**< Peak potential. */
    static std::string config_file; /**< Configuration file for the model. */

};

std::string ActionPotentialRestSurface::config_file = "";

SplineContainer2D ActionPotentialRestSurface::splines;

#endif
