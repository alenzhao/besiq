#include <bayesic/method/lm_method.hpp>

#include <dcdflib/libdcdf.hpp>

lm_method::lm_method(method_data_ptr data, model_matrix &model_matrix)
: method_type::method_type( data ),
  m_model_matrix( model_matrix )
{
}

std::vector<std::string>
lm_method::init()
{
    std::vector<std::string> header;
    
    header.push_back( "LR" );
    header.push_back( "P" );
    
    return header;
}

void lm_method::run(const snp_row &row1, const snp_row &row2, float *output)
{
    arma::uvec missing = get_data( )->missing;

    m_model_matrix.update_matrix( row1, row2, missing );
    
    glm_info null_info;
    lm( m_model_matrix.get_null( ), get_data( )->phenotype, missing, null_info );

    glm_info alt_info;
    arma::vec b = lm( m_model_matrix.get_alt( ), get_data( )->phenotype, missing, alt_info );

    set_num_ok_samples( missing.n_elem - sum( missing ) );

    if( null_info.success && alt_info.success )
    {
        int LR_pos = 0;

        double LR = -2 * ( null_info.logl - alt_info.logl );

        try
        {
            double p = 1.0 - chi_square_cdf( LR, m_model_matrix.num_df( ) );
            output[ LR_pos ] = LR;
            output[ LR_pos + 1 ] = p;
        }
        catch(bad_domain_value &e)
        {
        }
    }
}
