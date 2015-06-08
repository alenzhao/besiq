#ifndef __WALD_LM_METHOD_H__
#define __WALD_LM_METHOD_H__

#include <string>
#include <vector>

#include <armadillo>

#include <besiq/method/method.hpp>
#include <besiq/stats/log_scale.hpp>

/**
 * This class is responsible for executing the closed form
 * wald test for a logistic regression model.
 */
class wald_lm_method
: public method_type
{
public:
    /**
     * Constructor.
     *
     * @param data Additional data required by all methods, such as
     *             covariates.
     * @param unequal_var If true, estimates a separate variance for each cell.
     */
    wald_lm_method(method_data_ptr data, bool unequal_var = false);
    
    /**
     * @see method_type::init.
     */
    virtual std::vector<std::string> init();
    
    /**
     * @see method_type::run.
     */
    virtual void run(const snp_row &row1, const snp_row &row2, float *output);
private:
    /**
     * Weight for each sample.
     */
    arma::vec m_weight;

    /**
     * Determines whether variances should be estimated separately.
     */
    bool m_unequal_var;
};

#endif /* End of __WALD_LM_METHOD_H__ */
