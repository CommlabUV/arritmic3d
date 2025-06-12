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

#ifndef ACTION_POTENTIAL_H
#define ACTION_POTENTIAL_H


/**
 * @brief Action potential model based on APD restitution curves.
 */
class ActionPotential
{
public:
    /**
     * @brief Default constructor.
     *
     */
    ActionPotential()
    {
        apd = 300.0;
        ta = 0.0;
    };

    /**
     * @brief Constructor.
     *
     * @param apd_ Action potential duration.
     * @param t0_ Time of the activation.
     * @param di_ Diastolic interval.
     */
    ActionPotential(float apd_, float t0_, float di_ = 0.0)
    {
        Init(apd_, t0_, di_);
    };

    /**
     * @brief Initialize the action potential.
     *
     * @param apd_ Action potential duration.
     * @param t0_ Time of the activation.
     * @param di_ Diastolic interval.
     */
    void Init(float apd_, float t0_, float di_ = 0.0)
    {
        this->apd = apd_;
        this->ta = t0_ - (apd_ + di_);
    };

    /**
     * @brief Recompute the action potential after an activation.
     *
     */
    bool Activate(float new_ta)
    {
        float di = new_ta -(this->ta + this->apd);
        if(di < 0.0)
            return false;

        this->apd = 0.7*this->apd + 0.3*(di+30.0); // At least, 30ms of APD
        this->ta = new_ta;
        return true;
    };

    /**
     * @brief Recompute the action potential after an activation taking into account the electrotonic effect.
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
        return (t_ >= this->ta and t_ <= this->ta + this->apd);
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
        return this->ta + this->apd - t;
    };


private:
    float apd; /**< Action potential duration. */
    float ta; /**< Time of the activation. */

    //Spline restitution_curve; /**< APD restitution curve. */

    static constexpr bool normalized_potential = false; /**< Whether the potential is normalized. */
    static constexpr float resting_potential = -80.0; // 0.0; /**< Resting potential. */
    static constexpr float peak_potential = 40.0; // 1.0;  /**< Peak potential. */


};


#endif
