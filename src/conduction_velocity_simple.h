/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 *
 * @file conduction_velocity_simple.h
 *
 * @brief Classes to implement the conduction velocity evolution of a cell.
 *
 * Essentially, it is the evolution of the conduction velocity along time.
 * The basic interface includes a method to get the conduction velocity at a given time.
 *
 * */

#ifndef CONDUCTION_VELOCITY_SIMPLE_H
#define CONDUCTION_VELOCITY_SIMPLE_H

#include "node.h"

/**
 * @brief Conduction velocity model based on step function.
 *
 * This model considers constant CV between updates.
 */
class ConductionVelocitySimple
{
public:
    /**
     * @brief Default constructor.
     *
     */
    ConductionVelocitySimple()
    {
        this->cv = 1.0;
    };


    /**
     * @brief Constructor.
     *
     * @param type Cell type.
     */
    ConductionVelocitySimple(CellType type)
    {
        Init(type);
    };


    /**
     * @brief Initialize the conduction velocity.
     *
     * @param type Cell type.
     */
    void Init(CellType type)
    {
        this->cv = 1.0;
    };

    /**
     * @brief Recompute the conduction velocity after an activation.
     *
     * @param di Diastolic interval.
     */
    void Activate(float di)
    {
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

    float cv; /**< Conduction velocity. */

};


#endif // CONDUCTION_VELOCITY_SIMPLE_H