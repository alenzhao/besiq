#include <dcdflib/libdcdf.hpp>
#include <bayesic/method/loglinear_method.hpp>
#include <bayesic/stats/snp_count.hpp>

loglinear_method::loglinear_method(method_data_ptr data)
: method_type::method_type( data )
{
    m_models.push_back( new full( ) );
    m_models.push_back( new partial( true ) );
    m_models.push_back( new partial( false ) );
    m_models.push_back( new block( ) );

    m_weight = arma::ones<arma::vec>( data->phenotype.size( ) );
}

unsigned int
loglinear_method::num_ok_samples(const snp_row &row1, const snp_row &row2, const arma::vec &phenotype)
{
    return arma::accu( joint_count( row1, row2, get_data( )->phenotype, m_weight ) + 1.0 );
}

std::vector<std::string>
loglinear_method::init()
{
    std::vector<std::string> header;

    header.push_back( "P" );

    return header;
}

void
loglinear_method::run(const snp_row &row1, const snp_row &row2, float *output)
{
    double num_samples = num_ok_samples( row1, row2, get_data( )->phenotype );
    std::vector<log_double> likelihood( m_models.size( ), 0.0 );
    std::vector<double> bic( m_models.size( ), 0.0 );
    bool all_valid = true;
    for(int i = 0; i < m_models.size( ); i++)
    {
        bool is_valid = false;
        likelihood[ i ] = m_models[ i ]->prob( row1, row2, get_data( )->phenotype, m_weight, &is_valid );
        all_valid = all_valid && is_valid;
        bic[ i ] = -2.0 * likelihood[ i ].log_value( ) + m_models[ i ]->num_params( ) * log( num_samples );
    }

    if( all_valid )
    {
        unsigned int best_model = std::distance( bic.begin( ), std::min_element( bic.begin( ) + 1, bic.end( ) ) );
        double LR = -2.0*(likelihood[ best_model ].log_value( ) - likelihood[ 0 ].log_value( ));
    
        try
        {
            double p_value = 1.0 - chi_square_cdf( LR, m_models[ best_model ]->df( ) );
            output[ 0 ] = p_value;
        }
        catch(bad_domain_value &e)
        {
        }
    }
}
