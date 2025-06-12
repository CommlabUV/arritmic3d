/**
 * ARRITMIC3D
 *
 * (C) CoMMLab-UV 2023
 *
 * @file node_parameters.h
 *
*/

#ifndef NODE_PARAMETERS_H
#define NODE_PARAMETERS_H

#include <eigen3/Eigen/Dense>
#include <vector>
#include <map>


/**
 * @brief Parameters for a Node object.
 * Parameters that determine the Node behaviour.
 * These parameters should not change during the simulation !!!
 */
struct NodeParameters
{
    using Vector3 = Eigen::Vector3f;
    using Vector2 = Eigen::Vector2f;

    bool        isotropic_diffusion = true;       ///< @brief Isotropic diffusion flag.
    TissueRegion   tissue_region = TissueRegion::ENDO;      ///< @brief endo (0) / mid (1) / epi (2)
    unsigned char  sensor = 0;                ///< @brief Sensor flag. 0: no sensor, 1: sensor
    unsigned char  label1 = 0;                ///< @brief user defined label
    //unsigned short   padding = 0;            ///< Padding for alignment

    float       initial_apd = 100.0;     ///< Initial APD @todo CAMBIAR!!

    //float       gray_level = 1.0;                       ///< @brief Grey level in the CT/MR image.
    float       cond_veloc_transversal_reduction = 0.25;    ///< @brief Amount of CV reduction along transversal direction
    float       safety_factor = 1.0;                    ///< @brief Safety factor of the Node
    float       correction_factor_cv_border_zone = 1.0; ///< @brief Correction factor to apply to CV in BZ Nodes
    float       correction_factor_apd = 1.0;            ///< @brief Correction factor to apply to APD in BZ Nodes
    float       electrotonic_effect = 0.85;              ///< @brief Electrotonic effect factor.
    float       min_potential = 0.0;                    ///< @brief Used with safety factor.

    static const std::vector<std::string> names;        ///< @brief Names of the parameters.

    /**
     * @brief Set a parameter.
     * @param i Index of the parameter.
     * @param value Value of the parameter.
    */
    void SetParameter(size_t i, float value)
    {
        switch(i)
        {
            case 0: initial_apd = value; break;
            case 1: cond_veloc_transversal_reduction = value; break;
            case 2: safety_factor = value; break;
            case 3: correction_factor_cv_border_zone = value; break;
            case 4: correction_factor_apd = value; break;
            case 5: electrotonic_effect = value; break;
            case 6: min_potential = value; break;

            // int parameters automatically converted to float
            case 7: sensor = static_cast<unsigned char>(value); break; // sensor is an unsigned char
            case 8: label1 = static_cast<unsigned char>(value); break; // label1 is an unsigned char
            default: std::cerr << "Unknown FLOAT parameter index: " << i << std::endl;
        }
    }

    /**
     * @brief Set an integer parameter.
     * @param i Index of the parameter.
     * @param value Value of the parameter.
    */
    void SetParameter(size_t i, int value)
    {
        switch(i)
        {
            case 7: sensor = value; break;
            case 8: label1 = value; break;
            default: std::cerr << "Unknown INT parameter index: " << i << std::endl;
        }
    }

    /**
     * @brief Return a dictionary with the parameters.
     * int values are converted to float.
     * @return Dictionary with the parameters.
    */
    std::map<std::string, float> GetParameters() const
    {
        std::map<std::string, float> parameters;
        parameters["INITIAL_APD"] = initial_apd;
        parameters["COND_VELOC_TRANSVERSAL_REDUCTION"] = cond_veloc_transversal_reduction;
        parameters["SAFETY_FACTOR"] = safety_factor;
        parameters["CORRECTION_FACTOR_CV_BORDER_ZONE"] = correction_factor_cv_border_zone;
        parameters["CORRECTION_FACTOR_APD"] = correction_factor_apd;
        parameters["ELECTROTONIC_EFFECT"] = electrotonic_effect;
        parameters["MIN_POTENTIAL"] = min_potential;
        parameters["SENSOR"] = static_cast<float>(sensor);
        return parameters;
    }

    /**
     * @brief Compare operator for NodeParameters.
     * It allows the use of NodeParameters as a key in a map.
     * @todo With C++20 we could use the spaceship operator.
    */
    bool operator<(const NodeParameters & p) const
    {
        // We should guarantee that padding uses 0 or is not present.
        return memcmp(this, &p, sizeof(NodeParameters) ) < 0;
    }
};

const std::vector<std::string> NodeParameters::names = {"INITIAL_APD",
                                                  "COND_VELOC_TRANSVERSAL_REDUCTION", "SAFETY_FACTOR",
                                                  "CORRECTION_FACTOR_CV_BORDER_ZONE", "CORRECTION_FACTOR_APD",
                                                  "ELECTROTONIC_EFFECT", "MIN_POTENTIAL", "SENSOR", "LABEL1"};

/**
 * @brief Pool of NodeParameters.
 * Stores only one copy of each NodeParameters.
*/
class ParametersPool
{
public:
    /**
     * One parameter for all nodes.
    */
    void Init(const NodeParameters & p)
    {
        index.insert({p,0});
        pool.push_back(p);
    }

    /**
     * Different parameters for each node.
    */
    void Init(const std::vector<NodeParameters> & vp)
    {
        pool.reserve(vp.size());

        // Insert parameters in the map
        for(auto & p : vp)
            index.insert({p, 0});

        // Put the parameters in the pool and update the index with the position in the pool
        size_t i = 0;
        for(auto & x : index)
        {
            pool.push_back(x.first);
            x.second = i;
            ++i;
        }
    }

    /**
     * @brief Find a NodeParameters in the pool.
     * @pre The NodeParameters must be in the pool.
     *
     * @param p NodeParameters to find.
     * @return Pointer to the NodeParameters in the pool.
    */
    NodeParameters* Find(const NodeParameters & p)
    {
        auto it = index.find(p);
        assert(it != index.end());

        return & pool[it->second];
    }

    std::string Info() const
    {
        std::string info;
        info += "Pool size: " + std::to_string(pool.size()) + " Size of NodeParameters struct: " + std::to_string(sizeof(NodeParameters)) + "\n";
        for(auto & p : pool)
        {
            info += " Region: " + std::to_string((int)p.tissue_region) + " APD: " + std::to_string(p.initial_apd) + "\n";
        }
        return info;
    }



private:
    std::vector<NodeParameters> pool;
    std::map<NodeParameters, size_t> index;
};


#endif // NODE_PARAMETERS_H