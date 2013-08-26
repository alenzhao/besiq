#include <stats/bayesic_models.hpp>

using namespace arma;

saturated::saturated(log_double prior)
: m_prior( prior )
{

}

log_double
saturated::prior( )
{
    return m_prior;
}

log_double
saturated::prob(const snp_row &row1, const snp_row &row2, const arma::vec &phenotype, const arma::vec &weight)
{
    mat counts = joint_count( row1, row2, phenotype, weight );
    std::cout << counts << std::endl;

    log_double likelihood = 1.0;
    for(int i = 0; i < counts.n_rows; i++)
    {
        vec row_count = counts.row( i ).t( );
        vec alpha = ones<vec>( 2 );
        likelihood *= log_double::from_log( ldirmult( row_count, alpha ) );
    }

    return likelihood * null::snp_prob( row1, row2, phenotype, weight );
}

null::null(log_double prior)
: m_prior( prior )
{

}

log_double
null::prior( )
{
    return m_prior;
}

log_double
null::prob(const snp_row &row1, const snp_row &row2, const arma::vec &phenotype, const arma::vec &weight)
{
    return snp_prob( row1, row2, phenotype, weight ) * pheno_prob( row1, row2, phenotype, weight );
}

log_double
null::snp_prob(const snp_row &row1, const snp_row &row2, const arma::vec &phenotype, const arma::vec &weight)
{
    vec counts = joint_count( row1, row2 );
    vec alpha = compute_alpha( row1, row2 );

    return log_double::from_log( ldirmult( counts, alpha ) );
}

log_double
null::pheno_prob(const snp_row &row1, const snp_row &row2, const arma::vec &phenotype, const arma::vec &weight)
{
    vec counts = pheno_count( row1, row2, phenotype, weight );
    vec alpha = ones<vec>( 2 );

    return log_double::from_log( ldirmult( counts, alpha ) );
}

vec
null::compute_alpha(const snp_row &row1, const snp_row &row2)
{
    vec maf1 = compute_maf( row1 );
    vec maf2 = compute_maf( row2 );

    vec alpha = zeros<vec>( 9 );
    for(int i = 0; i < 3; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            alpha[ 3 * i + j ] = maf1[ i ] * maf2[ j ];
        }
    }

    return alpha;
}

ld_assoc::ld_assoc(log_double prior, bool is_first)
: m_prior( prior ),
  m_is_first( is_first )
{

}

log_double
ld_assoc::prior( )
{
    return m_prior;
}

log_double
ld_assoc::prob(const snp_row &row1, const snp_row &row2, const arma::vec &phenotype, const arma::vec &weight)
{
    const snp_row &snp1 = m_is_first ? row1 : row2;
    const snp_row &snp2 = m_is_first ? row2 : row1;

    mat counts = single_count( snp1, snp2, phenotype, weight );

    log_double likelihood = 1.0;
    for(int i = 0; i < counts.n_rows; i++)
    {
        vec row_count = counts.row( i ).t( );
        vec alpha = ones<vec>( 2 );
        likelihood *= log_double::from_log( ldirmult( row_count, alpha ) );

    }

    return likelihood * null::snp_prob( snp1, snp2, phenotype, weight );
}
