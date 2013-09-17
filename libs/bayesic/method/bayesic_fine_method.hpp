#ifndef __BAYESIC_FINE_METHOD_H__
#define __BAYESIC_FINE_METHOD_H__

#include <string>
#include <vector>

#include <method/bayesic_method.hpp>
#include <stats/log_scale.hpp>
#include <snp_row.hpp>

/**
 * This class is responsible for intializing and repeatedly
 * executing the bayesic method on pairs of snps.
 */
class bayesic_fine_method
: public bayesic_method
{
public:
    /**
     * Constructor.
     *
     * @param data Additional data required by all methods, such as
     *             covariates.
     * @param num_mc_iterations The number of monte carlo iterations.
     */
    bayesic_fine_method(method_data_ptr data, int num_mc_iterations);
};

#endif /* End of __BAYESIC_FINE_METHOD_H__ */
