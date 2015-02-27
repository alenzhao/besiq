#include <dcdflib/libdcdf.hpp>
#include <bayesic/method/wald_lm_method.hpp>
#include <bayesic/stats/snp_count.hpp>

wald_lm_method::wald_lm_method(method_data_ptr data)
: method_type::method_type( data )
{
    m_weight = arma::ones<arma::vec>( data->phenotype.n_elem );
}

void
wald_lm_method::init(std::ostream &output)
{
    output << "LR\tP";
}

void
wald_lm_method::run(const snp_row &row1, const snp_row &row2, std::ostream &output)
{
    arma::mat suf = arma::zeros<arma::mat>( 3, 3 );
    arma::mat suf2 = arma::zeros<arma::mat>( 3, 3 );
    arma::mat n = arma::zeros<arma::mat>( 3, 3 );
    double num_samples = 0.0;

    for(int i = 0; i < row1.size( ); i++)
    {
        if( row1[ i ] == 3 || row2[ i ] == 3 || get_data( )->missing[ i ] == 1 )
        {
            continue;
        }

        double pheno = get_data( )->phenotype[ i ];
        n( row1[ i ], row2[ i ] ) += 1;
        suf( row1[ i ], row2[ i ] ) += pheno;
        suf2( row1[ i ], row2[ i ] ) += pheno * pheno;

        num_samples++;
    }
    
    if( arma::min( arma::min( n ) ) <= 10 )
    {
        output << "NA\tNA";
        return;
    }

    /* Calculate residual and estimate sigma^2 */
    double residual_sum = 0.0;
    arma::mat mu = arma::zeros<arma::mat>( 3, 3 );
    for(int i = 0; i < 3; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            double deviance = ( suf2( i, j ) - suf( i, j ) * suf( i, j ) / n( i, j ) );
            residual_sum += deviance;
            mu( i, j ) = suf( i, j ) / n( i, j );
        }
    }
    double sigma2 = residual_sum / ( num_samples - 9 );

    /* Fisher information and betas */ 
    arma::vec beta = arma::zeros<arma::vec>( 4 );
    arma::mat I( 4, 4 );
    int i_map[] = { 1, 1, 2, 2 };
    int j_map[] = { 1, 2, 1, 2 };
    for(int i = 0; i < 4; i++)
    {
        int c_i = i_map[ i ];
        int c_j = j_map[ i ];
        beta[ i ] = mu( 0, 0 ) - mu( 0, c_j ) - mu( c_i, 0 ) + mu( c_i, c_j );

        for(int j = 0; j < 4; j++)
        {
            int o_i = i_map[ j ];
            int o_j = j_map[ j ];

            int same_row = c_i == o_i;
            int same_col = c_j == o_j;
            int in_cell = i == j;

            I( i, j ) = sigma2 * ( 1 / n( 0, 0 ) + same_col / n( 0, c_j ) + same_row / n( c_i, 0 ) + in_cell / n( c_i, c_j ) );
        }
    }

    arma::mat Iinv( 4, 4 );
    if( !inv( Iinv, I ) )
    {
        output << "NA\tNA";
        return;
    }
    
    /* Test if b != 0 with Wald test */
    double chi = dot( beta, Iinv * beta );
    output << chi << "\t" << 1.0 - chi_square_cdf( chi, 4 );
}
