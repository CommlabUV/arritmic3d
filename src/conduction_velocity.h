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

#include "spline2D.h"
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
     * @param corrfc_ Correction factor for restitution models.
     */
    ConductionVelocity(CellType type, float corrfc_ = 1.0)
    {
        Init(type, corrfc_);
    };


    static void InitModel(const std::string &path)
    {
        splines.Init(path);
    }

    /**
     * @brief Initialize the conduction velocity.
     *
     * @param type Cell type.
     * @param corrfc_ Correction factor for restitution models.
     */
    void Init(CellType type, float corrfc_ = 1.0)
    {
        SetRestitutionModel(type);
        this->correction_factor = corrfc_;
        this->cv = INITIAL_CV;
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
            throw std::runtime_error("conduction_velocity.h: no CV restitution model found for cell type " + std::to_string(static_cast<int>(type)));
    };

    /**
     * @brief Recompute the conduction velocity after an activation.
     *
     * @param di Diastolic interval.
     */
    void Activate(float di,float apd)
    {
        this->cv = this->restitution_model->getValue(di,apd)*this->correction_factor;
    };

   /**
     * @brief Recompute the conduction velocity after an activation taking
     * into account the electrotonic effect.
     *
     * @param di Diastolic interval.
     * @param avg_cv Average conduction velocity.
     * @param e_eff Electrotonic effect.
     */
    void Activate(float di, float apd, float avg_cv, float e_eff = 0.0)
    {
        Activate(di, apd);
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
    float correction_factor; /**< Correction factor for restitution model. */

    Spline2D * restitution_model; ///< APD restitution model.
    static SplineContainer2D splines; ///< Container of APD restitution models.

    static std::string config_file; /**< Configuration file for the model. */

};

std::string ConductionVelocity::config_file = "";

SplineContainer2D ConductionVelocity::splines;

#endif // CONDUCTION_VELOCITY_H