#include <iostream>

#include <armadillo>

#include <cpp-argparse/OptionParser.h>

#include <besiq/method/wald_method.hpp>
#include <besiq/method/wald_lm_method.hpp>
#include <besiq/method/wald_separate_method.hpp>
#include <besiq/method/method.hpp>

#include <dcdflib/libdcdf.hpp>

#include "common_options.hpp"

using namespace arma;
using namespace optparse;

const std::string USAGE = "besiq-meta [OPTIONS] pairs genotype_prefix1 [genotype_prefix2 ...]";
const std::string DESCRIPTION = "Meta analysis using wald tests.";
const std::string VERSION = "Bayesic 0.5.9";
const std::string EPILOG = "This command assumes that input data has been cleaned and alleles have been flipped consistently.";

std::vector<plink_file_ptr> open_plink_file(const std::vector<std::string> &prefix)
{
    std::vector<plink_file_ptr> plink_files;
    // First index is pair file
    for(int i = 1; i < prefix.size( ); i++)
    {
        plink_files.push_back( open_plink_file( prefix[ i ], false ) );
    }

    return plink_files;
}

std::vector<genotype_matrix_ptr> create_genotype_matrices(std::vector<plink_file_ptr> &plink_files)
{
    std::vector<genotype_matrix_ptr> genotypes;
    for(int i = 0; i < plink_files.size( ); i++)
    {
        genotypes.push_back( create_genotype_matrix( plink_files[ i ] ) );
    }

    return genotypes;
}

OptionParser
create_options()
{
    OptionParser parser = OptionParser( ).usage( USAGE )
                                         .version( VERSION )
                                         .description( DESCRIPTION )
                                         .epilog( EPILOG );
    
    char const* const model_choices[] = { "binomial", "normal" };
    parser.add_option( "-m", "--model" ).choices( &model_choices[ 0 ], &model_choices[ 2 ] ).metavar( "model" ).help( "The model to use for the phenotype, 'binomial' or 'normal', default = 'binomial'." ).set_default( "binomial" );
    parser.add_option( "-o", "--out" ).help( "The output file that will contain the results (binary)." );
    parser.add_option( "-t", "--threshold" ).help( "Only output pairs with a p-value less than this." ).set_default( -9 );
    parser.add_option( "--split" ).help( "Runs the analysis on a part of the pair file, and this is part X of 1-<num_splits> parts (default = 1)." ).set_default( 1 );
    parser.add_option( "--num-splits" ).help( "Sets the number of parts to split the pair file in (default = 1)." ).set_default( 1 );
    
    return parser;
}

int
main(int argc, char *argv[])
{
    OptionParser parser = create_options( );
    
    Values options = parser.parse_args( argc, argv );
    if( parser.args( ).size( ) < 2 )
    {
        parser.print_help( );
        exit( 1 );
    }
    
    std::vector<plink_file_ptr> plink_files = open_plink_file( parser.args( ) );
    std::vector<genotype_matrix_ptr> genotypes = create_genotype_matrices( plink_files );
    
    /** 
     * Create pair iterator 
     */
    size_t split = (size_t) options.get( "split" );
    size_t num_splits = (size_t) options.get( "num_splits" );
    if( split > num_splits || split == 0 || num_splits == 0 )
    {
        std::cerr << "besiq: error: Num splits and split must be > 0, and split <= num_splits." << std::endl;
        exit( 1 );
    }
    
    pairfile *pairs = open_pair_file( parser.args( )[ 0 ].c_str( ), plink_files[ 0 ]->get_locus_names( ) );
    if( pairs == NULL || !pairs->open( split, num_splits ) )
    {
        std::cerr << "besiq: error: Could not open pair file." << std::endl;
        exit( 1 );
    }

    double threshold = (double) options.get( "threshold" );

    /**
     * Set up method
     */
    std::vector<wald_method *> methods;
    for(int i = 0; i < plink_files.size( ); i++)
    {
        method_data_ptr data( new method_data( ) );
        data->missing = zeros<uvec>( plink_files[ i ]->get_samples( ).size( ) );
        data->phenotype = create_phenotype_vector( plink_files[ i ]->get_samples( ), data->missing );
        wald_method *m = NULL;
        if( options[ "model" ] == "binomial" )
        {
            m = new wald_method( data );
        }
        else if( options[ "model" ] == "normal" )
        {
            //m = new wald_lm_method( data, false );
        }

        methods.push_back( m );
    }

    /**
     * Open results.
     */
    resultfile *result = NULL;
    if( options.is_set( "out" ) )
    {
        result = new bresultfile( options[ "out" ], plink_files[ 0 ]->get_locus_names( ) );
    }
    else
    {
        std::ios_base::sync_with_stdio( false );
        result = new tresultfile( "-", "w" );
    }
    if( result == NULL || !result->open( ) )
    {
        std::cerr << "besiq: error: Can not open result file." << std::endl;
        exit( 1 );
    }
    std::vector<std::string> header;
    header.push_back( "W" );
    header.push_back( "P" );
    header.push_back( "N" );
    result->set_header( header );

    /**
     * Run analysis
     */
    std::pair<std::string, std::string> pair;
    float *output = new float[ methods[ 0 ]->init( ).size( ) ];
    float meta_output[ header.size( ) ];
    while( pairs->read( pair ) )
    {
        /* Compute betas and covariances */
        std::vector<arma::vec> betas( methods.size( ), arma::zeros<arma::vec>( 4 ) );
        std::vector<arma::mat> covs( methods.size( ), arma::zeros<arma::mat>( 4, 4 ) );
        size_t N = 0;
        bool all_valid = true;
        for(int i = 0; i < methods.size( ); i++)
        {
            snp_row const *row1 = genotypes[ i ]->get_row( pair.first );
            snp_row const *row2 = genotypes[ i ]->get_row( pair.second );

            methods[ i ]->run( *row1, *row2, output );

            betas[ i ] = methods[ i ]->get_last_beta( );
            covs[ i ] = methods[ i ]->get_last_C( );

            if( covs[ i ].n_cols != 4 || covs[ i ].n_rows != 4 )
            {
                all_valid = false;
            }

            N += methods[ i ]->num_ok_samples( *row1, *row2 );
        }

        if( !all_valid )
        {
            continue;
        }

        /* Computed weighted average of betas (fixed effect assumption) */
        arma::mat Csum = arma::zeros<arma::mat>( 4, 4 );
        for(int i = 0; i < covs.size( ); i++)
        {
            Csum += covs[ i ];
        }
        arma::mat Csum_inv;
        if( !arma::inv( Csum_inv, Csum ) )
        {
            continue;
        }

        arma::vec final_beta = arma::zeros<arma::vec>( 4 );
        arma::mat final_C = arma::zeros<arma::mat>( 4, 4 );
        for(int i = 0; i < covs.size( ); i++)
        {
            final_beta += covs[ i ] * betas[ i ];
            final_C += covs[ i ] * covs[ i ] * covs[ i ];
        }
        final_beta = Csum_inv * final_beta;
        final_C = Csum_inv * final_C * Csum_inv;

        arma::mat final_C_inv;
        if( !arma::inv( final_C_inv, final_C ) )
        {
            continue;
        }

        double chi = dot( final_beta, final_C_inv * final_beta );
        double final_p = 1.0 - chi_square_cdf( chi, 4 );

        if( threshold != -9 && final_p > threshold )
        {
            continue;
        }

        meta_output[ 0 ] = chi;
        meta_output[ 1 ] = final_p;
        meta_output[ 2 ] = N;
        
        result->write( pair, meta_output );
    }

    result->close( );

    /* Delete allocated stuff */
    
    return 0;
}
