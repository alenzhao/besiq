#ifndef __CASEONLY_METHOD_H__
#define __CASEONLY_METHOD_H__

#include <string>
#include <vector>

#include <armadillo>

#include <bayesic/method/method.hpp>
#include <bayesic/stats/log_scale.hpp>

/**
 * This class is responsible for intializing and repeatedly
 * executing the case-only test by Lewinger et al.
 */
class caseonly_method
: public method_type
{
public:
    /**
     * Constructor.
     *
     * @param data Additional data required by all methods, such as
     *             covariates.
     */
    caseonly_method(method_data_ptr data);
    
    /**
     * @see method_type::init.
     */
    virtual void init(std::ostream &output);

    /**
     * Returns the number of usable samples.
     *
     * @param row1 The first snp.
     * @param row2 The second snp.
     * @param phenotype The phenotype.
     * 
     * @return The number of usable samples.
     */    
    unsigned int num_ok_samples(const snp_row &row1, const snp_row &row2, const arma::vec &phenotype);

    /**
     * @see method_type::run.
     */
    virtual void run(const snp_row &row1, const snp_row &row2, std::ostream &output);

private: 
    /**
     * A weight > 0 associated with each sample, that allows for
     * covariate adjustment.
     */
    arma::vec m_weight;
};

#endif /* End of __CASEONLY_METHOD_H__ */
