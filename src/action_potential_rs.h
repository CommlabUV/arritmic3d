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

#include "spline2D.h"
#include "node.h"
#include <cmath>

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
        if(type == CELL_TYPE_VOID)
            return;

        this->correction_factor = corrfc_;
        if (di_ > 0.0)
        {
            this->last_di = di_;
            // The next call can return -1, meaning no activation
            float new_apd = restitution_model->getValue(this->last_di,apd_)*this->correction_factor;
            // Check invalid value
            if(! restitution_model->is_novalue(new_apd) )
                this->apd = new_apd;
            else
                this->apd = apd_;
        }
        else
        {
            // Calculate proper di for the given apd
            float new_di = restitution_model->getEquilibrium(0, apd_);
            if(new_di < 0.0)
                this->last_di = 100.0; /// @todo Should be a simulation constant ?
            else
                this->last_di = new_di;
            this->apd = apd_;
        }
        this->ta = t0_ - (this->apd + this->last_di); // We assume that the previous apd is the same as the current one
    };

    /**
     * @brief Set the restitution model.
     *
     * @param type Cell type.
     */
    void SetRestitutionModel(CellType type)
    {
        this->restitution_model = splines.getSpline(type);
        if(this->restitution_model == nullptr && type != CELL_TYPE_VOID)
            throw std::runtime_error("action_potential_rs.h: : no APD restitution model found for cell type " + std::to_string(static_cast<int>(type)));
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

        float new_apd = restitution_model->getValue(this->apd, di)*this->correction_factor;
        if (! restitution_model->is_novalue(new_apd) )
        {
            this->last_di = di;
            this->delta_apd = std::fabs(new_apd - this->apd);    // Calculated without electrotonic effect !!
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
        if(! Activate(new_ta))
            return false;
        this->apd = this->apd*(1.0 - e_eff) + avg_apd*e_eff;
        return true;
    };

    /**
     * @brief Simple polynomial approximation of a Ventricular Cardiomyocyte Action Potential.
     *
     * Optimized for high-performance simulations (no tanh, exp, or pow).
     *
     * @param t Time normalized to the APD (0 to 1).
     * @return The action potential at time t, in mV.
     */
    double pseudoAP(double t) const
    {
        // Clamp t to [0, 1] range to prevent out-of-bounds behavior
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;

        double apRange = this->peak_potential - this->resting_potential;
        double yNorm = 0.0;

        if (t < 0.01)
        {
            // PHASE 0: Fast Depolarization (Na+ influx)
            // Rapid linear upstroke
            yNorm = t / 0.01;
        }
        else if (t < 0.06)
        {
            // PHASE 1: Early Repolarization (Notch / Ito current)
            // Linear drop from peak to plateau level
            double tn = (t - 0.01) / 0.05;
            yNorm = 1.0 - 0.2 * tn;
        }
        else if (t < 0.65)
        {
            // PHASE 2: Plateau (Balance of Ca2+ influx and K+ efflux)
            // Nearly flat with a slight negative slope
            double tn = (t - 0.06) / 0.59;
            yNorm = 0.8 - (0.02 * tn);
        }
        else
        {
            // PHASE 3: Final Repolarization (K+ efflux - Ikr/Iks)
            // We use a cubic term (tn*tn*tn) to create the characteristic curve
            // without the cost of transcendental functions.
            double tn = (t - 0.65) / 0.35;
            double tn3 = tn * tn * tn;
            yNorm = 0.78 * (1.0 - tn3);
        }

        return this->resting_potential + (apRange * yNorm);
    }

    /**
     * @brief Get the action potential at a given time.
     *
     * @param t_ Time.
     * @return The action potential at time t.
     */
    float getActionPotential(float t_) const
    {
        return pseudoAP( getLife(t_) );
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
        return this->apd + restitution_model->GetLabelNoValue(0, this->apd);
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
     * @brief Get the variation in APD due to restitution models (without electrotonic effect).
     *
     * @return The variation in APD.
     */
    float getDeltaAPD() const
    {
        return this->delta_apd;
    };


    /**
     * Save the state of the model to a file.
     * The correct restitution model will be set when loading according to the cell type.
     */
    void SaveState(std::ofstream & f) const
    {
        f.write( (char *) &apd, sizeof(float) );
        f.write( (char *) &ta, sizeof(float) );
        f.write( (char *) &last_di, sizeof(float) );
        f.write( (char *) &delta_apd, sizeof(float) );
        f.write( (char *) &correction_factor, sizeof(float) );
    }

    /**
    * Load the state of the model from a file.
    * The correct restitution model will be set according to the cell type.
    */
    void LoadState(std::ifstream & f, CellType type)
    {
        f.read( (char *) &apd, sizeof(float) );
        f.read( (char *) &ta, sizeof(float) );
        f.read( (char *) &last_di, sizeof(float) );
        f.read( (char *) &delta_apd, sizeof(float) );
        f.read( (char *) &correction_factor, sizeof(float) );

        SetRestitutionModel(type);
    }

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
