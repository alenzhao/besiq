#ifndef __BAYESIC_MODELS_H__
#define __BAYESIC_MODELS_H__

#include <armadillo>

#include <plink_file.hpp>
#include <stats/dirichlet.hpp>
#include <stats/log_scale.hpp>
#include <stats/snp_count.hpp>

/**
 * This class represents a general model that can compute a
 * model likelihood for two snps.
 */
class model
{
public:
    /**
     * Constructor.
     *
     * @param prior The prior for the model.
     */
    model(log_double prior)
    : m_prior( prior )
    {
    }
    
    /**
     * Returns the prior of the model.
     *
     * @return The prior probability.
     */
    log_double prior( )
    {
        return m_prior;
    }

    /**
     * Computes the likelihood of the snps and phenotypes under this
     * model. 
     *
     * @param row1 The first snp.
     * @param row2 The second snp.
     * @param phenotype The phenotype, discrete 0.0 and 1.0.
     * @param weight A weight for each sample, this will be used instead of 1.0 as a count.
     *
     * @return The likelihood of the snps.
     */
    virtual log_double prob(const snp_row &row1, const snp_row &row2, const arma::vec &phenotype, const arma::vec &weight) = 0;
    
private:
    /**
     * The prior probability for the model.
     */
    log_double m_prior;
};

/**
 * This model represents a saturated model in the sense
 * that each cell in the penetrance table has it is own
 * parameter.
 */
class saturated
: public model
{
public:
    /**
     * Constructor.
     *
     * @param prior The prior probability for the model.
     */
    saturated(log_double prior);
    
    /**
     * @see model::prob.
     */
    virtual log_double prob(const snp_row &row1, const snp_row &row2, const arma::vec &phenotype, const arma::vec &weight);
};

/**
 * This class represents a model where no snp is associated with
 * the phenotype. The snps may however be in ld.
 */
class null
: public model
{
public:
    /**
     * Constructor.
     *
     * @param prior The prior probability for the model.
     */
    null(log_double prior);

    /**
     * Computes a sensible prior alpha based on the minor allele frequencies
     * of both SNPs.
     *
     * @param row1 The first snp.
     * @param row2 The second snp.
     *
     * @return A vector of alphas that reflects the estiamted mafs.
     */
    static arma::vec compute_alpha(const snp_row &row1, const snp_row &row2);

    /**
     * Computes the probability of the snps under the null model.
     *
     * Note: Convenient to use in other models, as they always compute the
     *       null or prior probability of the snps.
     *
     * @param row1 The first snp.
     * @param row2 The second snp.
     * @param phenotype The phenotype, discrete 0.0 and 1.0.
     * @param weight A weight for each sample, this will be used instead of 1.0 as a count.
     *
     * @return the probability of the snps under the null model.
     */
    static log_double snp_prob(const snp_row &row1, const snp_row &row2, const arma::vec &phenotype, const arma::vec &weight);
    
    /**
     * Computes the probability of the phenotype under the null model.
     *
     * @param row1 The first snp.
     * @param row2 The second snp.
     * @param phenotype The phenotype, discrete 0.0 and 1.0.
     * @param weight A weight for each sample, this will be used instead of 1.0 as a count.
     *
     * @return the probability of the phenotype under the null model.
     */
    static log_double pheno_prob(const snp_row &row1, const snp_row &row2, const arma::vec &phenotype, const arma::vec &weight);

    /**
     * @see model::prob.
     */
    virtual log_double prob(const snp_row &row1, const snp_row &row2, const arma::vec &phenotype, const arma::vec &weight);
};

/**
 * This class represents a model where one snp is associated with
 * the phenotype, and the other possibly in ld with the first.
 */
class ld_assoc
: public model
{
public:
    /**
     * Constructor.
     *
     * @param prior The prior probability for the model.
     * @param is_first If true the first snp is associated, otherwise the second.
     */
    ld_assoc(log_double prior, bool is_first);

    /**
     * @see model::prob.
     */
    virtual log_double prob(const snp_row &row1, const snp_row &row2, const arma::vec &phenotype, const arma::vec &weight);

private:
    /**
     * Indicates which snp is associated with the phenotype, if true
     * the first, false the second.
     */
    bool m_is_first;
};

#endif /* End of __BAYESIC_MODELS_H__ */
