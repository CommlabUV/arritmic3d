/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 *
 * @file membrane_potential.h
 *
 * @brief Classes to implement the membrane received_potential evolution around a cell.
 *
 * Essentially, it is the evolution of the received_potential along time.
 * The basic interface includes a method to get the received_potential at a given time.
 *
 * Several models to implement the membrane received_potential are available.
 *
 * */

#ifndef MEMBRANE_POTENTIAL_H
#define MEMBRANE_POTENTIAL_H


/**
 * @brief Membrane received_potential model based on step function.
 *
 * This model only has a resting received_potential and a peak received_potential, plus a time of activation.
 */
class MembranePotentialStep
{
public:
    /**
     * @brief Constructor.
     *
     * @param t0_ Time of the activation.
     * @param normalized_potential_ Whether the received_potential is normalized.
     */
    MembranePotentialStep(double t0_ = 0, bool normalized_potential_ = false)
    : normalized_potential(normalized_potential_), ta(t0_)
    {
        this->setPotentialRange();
    };

    /**
     * @brief Get the membrane received_potential at a given time.
     *
     * @param t_ Time.
     * @return The membrane received_potential at time t.
     */
    double getMembranePotential(double t_) const
    {
        if(t_ < this->ta)
            return this->resting_potential;
        else
            return this->peak_potential;
    };


    /**
     * @brief Activate the membrane received_potential.
     *
     * @param t Time of the activation.
     */
    void Activate(double t)
    {
        this->ta = t;
    };

    /**
     * @brief Set the time of the activation.
     *
     * @param t0_ Time of the activation.
     */
    void setActivationTime(double t0_)
    {
        this->ta = t0_;
    };

    /**
     * @brief Get the time of the activation.
     *
     * @return The time of the activation.
     */
    double getActivationTime() const
    {
        return this->ta;
    };

private:
    /**
     * @brief Set the received_potential range.
     *
     * Set the received_potential range according to the received_potential normalization.
     */
    void setPotentialRange()
    {
        if(this->normalized_potential)
        {
            this->resting_potential = 0.0;
            this->peak_potential = 1.0;
        }
        else
        {
            this->resting_potential = -80.0;
            this->peak_potential = 40.0;
        }
    };

    bool normalized_potential; /**< Whether the received_potential is normalized. */
    double ta; /**< Time of the activation. */
    double resting_potential; /**< Resting received_potential. */
    double peak_potential; /**< Peak received_potential. */

};


#endif // MEMBRANE_POTENTIAL_H