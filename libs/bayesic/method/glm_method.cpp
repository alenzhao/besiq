#include <bayesic/method/glm_method.hpp>

#include <dcdflib/libdcdf.hpp>

glm_method::glm_method(method_data_ptr data, const glm_model &model, model_matrix &model_matrix)
: method_type::method_type( data ),
  m_model( model ),
  m_model_matrix( model_matrix )
{
}

std::vector<std::string>
glm_method::init()
{
    std::vector<std::string> header;
    header.push_back( "LR" );
    header.push_back( "P" );

    return header;
}

void glm_method::run(const snp_row &row1, const snp_row &row2, float *output)
{ 
    arma::uvec missing = get_data( )->missing;

    m_model_matrix.update_matrix( row1, row2, missing );

    irls_info null_info;
    irls( m_model_matrix.get_null( ), get_data( )->phenotype, missing, m_model, null_info );

    irls_info alt_info;
    arma::vec b = irls( m_model_matrix.get_alt( ), get_data( )->phenotype, missing, m_model, alt_info );

    if( null_info.converged && alt_info.converged )
    {
        int LR_pos = 0;
        /*if( get_data( )->print_params )
        {
            output[ 0 ] = b[ m_model_matrix.num_alt( ) - 1 ];
            for(int i = 0; i < m_model_matrix.num_alt( ) - 1; i++)
            {
                output[ i + 1 ] = b[ i ];
            }
            LR_pos = m_model_matrix.num_alt( );
        }*/

        double LR = -2 * ( null_info.logl - alt_info.logl );

        try
        {
            output[ LR_pos ] = LR;
            output[ LR_pos + 1 ] = 1.0 - chi_square_cdf( LR, m_model_matrix.num_df( ) );
        }
        catch(bad_domain_value &e)
        {

        }
    }
}
