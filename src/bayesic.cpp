#include <iostream>

#include <armadillo>

#include <plinkio/plinkio.h>

#include <gzstream/gzutil.hpp>
#include <bayesic/covariates.hpp>
#include <glm/irls.hpp>
#include <glm/models/binomial.hpp>
#include <cpp-argparse/OptionParser.h>

#include <plink/plink_file.hpp>
#include <bayesic/pair_iter.hpp>
#include <bayesic/prior.hpp>
#include <bayesic/method/bayesic_method.hpp>
#include <bayesic/method/bayesic_fine_method.hpp>
#include <bayesic/method/logistic_method.hpp>
#include <bayesic/method/logistic_factor_method.hpp>
#include <bayesic/method/loglinear_method.hpp>
#include <bayesic/method/method.hpp>

using namespace arma;
using namespace optparse;

const std::string USAGE = "bayesic [-c covariates.csv] [-m method] pairs genotype_plink_prefix";
const std::string VERSION = "Bayesic 0.0.1";
const std::string DESCRIPTION = "A tool for inferring genetic interactions.";
const std::string EPILOG = "";

std::vector<snp_row>
create_genotype_matrix(plink_file_ptr genotype_file)
{
    std::vector<snp_row> genotype_matrix;
    snp_row row;
    while( genotype_file->next_row( row ) )
    {
        genotype_matrix.push_back( row );
    }

    return genotype_matrix;
}

vec
create_phenotype_vector(plink_file_ptr genotype_file, uvec &missing)
{
    const std::vector<pio_sample_t> &samples = genotype_file->get_samples( );
    vec phenotype( samples.size( ) );
    for(int i = 0; i < samples.size( ); i++)
    {
        phenotype[ i ] = samples[ i ].phenotype;
        if( samples[ i ].affection == PIO_MISSING )
        {
            missing[ i ] = 1;
        }
    }

    return phenotype;
}

unsigned int
count_interactions(const char *pair_file_path, const std::vector<std::string> &loci)
{
    shared_ptr<std::istream> pair_file = open_possible_gz( pair_file_path );
    pair_iter pairs( *pair_file, loci );
    unsigned int num_interactions = 0;
    std::pair<size_t, size_t> pair;
    while( pairs.get_pair( &pair ) )
    {
        num_interactions++;
    }

    return num_interactions;
}

int
main(int argc, char *argv[])
{
    OptionParser parser = OptionParser( ).usage( USAGE )
                                         .version( VERSION )
                                         .description( DESCRIPTION )
                                         .epilog( EPILOG );
    
    
    char const* const choices[] = { "bayes", "bayes-fine", "logistic", "logistic-factor", "loglinear" };
    parser.add_option( "-m", "--method" ).choices( &choices[ 0 ], &choices[ 5 ] ).metavar( "method" ).help( "Which method to use, one of: 'bayes', 'bayes-fine', 'logistic', 'logistic-factor' or 'loglinear'." );
    parser.add_option( "-p", "--pheno" ).help( "Read phenotypes from this file instead of a plink file." );
    parser.add_option( "-c", "--cov" ).action( "store" ).type( "string" ).metavar( "filename" ).help( "Performs the analysis by including the covariates in this file." );
    
    OptionGroup group = OptionGroup( parser, "Options for bayes", "These options will change the behaviour of bayes and fine." );
    group.add_option( "-n", "--num-interactions" ).type( "int" ).help( "The number of interactions to correct for, this is used in the model prior (default: all)." ).set_default( 1 );
    group.add_option( "-i", "--mc-iterations" ).type( "int" ).help( "The number of monte carlo iterations to use in the fine method (default: %default)." ).set_default( 4000000 );
    group.add_option( "-a", "--beta-prior-param1" ).type( "float" ).help( "First shape parameter of beta prior (default: %default)." ).set_default( 2.0 );
    group.add_option( "-b", "--beta-prior-param2" ).type( "float" ).help( "Second shape parameter of beta prior (default: %default)." ).set_default( 2.0 );
    group.add_option( "-e", "--estimate-prior-params" ).action( "store_true" ).help( "Estimate prior parameters from data by permuting phenotype (default: off)." );
    parser.add_option_group( group );

    Values options = parser.parse_args( argc, argv );
    std::vector<std::string> args = parser.args( );
    if( args.size( ) != 2 )
    {
        std::cerr << "bayesic: error: Pairs or genetypes is missing." << std::endl;
        parser.print_help( );
        exit( 1 );
    }
    else if( !options.is_set( "method" ) )
    {
        std::cerr << "bayesic:error: No method selected." << std::endl;
        exit( 1 );
    }

    /* Read all genotypes */
    plink_file_ptr genotype_file = open_plink_file( args[ 1 ] );
    std::vector<snp_row> genotype_matrix = create_genotype_matrix( genotype_file );
    
    /* Create pair iterator */
    shared_ptr<std::istream> pair_file = open_possible_gz( args[ 0 ].c_str( ) );
    std::vector<std::string> locus_names = genotype_file->get_locus_names( );
    pair_iter pairs( *pair_file, locus_names );
    
    /* Read additional data  */
    method_data_ptr data( new method_data( ) );
    data->missing = zeros<uvec>( genotype_file->get_samples( ).size( ) );
    data->phenotype = create_phenotype_vector( genotype_file, data->missing );
    std::vector<std::string> order = genotype_file->get_sample_iids( );
    if( options.is_set( "pheno" ) )
    {
        std::ifstream phenotype_file( options[ "pheno" ].c_str( ) );
        data->phenotype = parse_phenotypes( phenotype_file, data->missing, order );
    }
    if( options.is_set( "cov" ) )
    {
        std::ifstream covariate_file( options[ "cov" ].c_str( ) );
        data->covariate_matrix = parse_covariate_matrix( covariate_file, data->missing, order );
    }

    /* Read prior parameters */
    arma::vec alpha = arma::ones<arma::vec>( 2 );
    alpha[ 0 ] = (float) options.get( "beta_prior_param1" );
    alpha[ 1 ] = (float) options.get( "beta_prior_param2" );
    if( options.is_set( "estimate_prior_params" ) )
    {
        alpha = estimate_prior_parameters( genotype_matrix, data->phenotype, data->missing, 5000 );
    }

    /* Count the number of interactions to adjust for */
    data->num_interactions = (unsigned int) options.get( "num_interactions" );
    if( !options.is_set( "num_interactions" ) )
    {
        data->num_interactions = count_interactions( args[ 0 ].c_str( ), locus_names );
    }

    /* XXX: Implement proper log file. */
    std::ostream nullstream( 0 );
    arma::set_stream_err2( nullstream );

    /* Run method */
    if( options[ "method" ] == "bayes" )
    {
        bayesic_method bayesic( data, alpha );
        run_method( bayesic, genotype_matrix, locus_names, pairs );
    }
    else if( options[ "method" ] == "bayes-fine" )
    {
        bayesic_fine_method bayesic_fine( data, (int) options.get( "mc_iterations" ), alpha );
        run_method( bayesic_fine, genotype_matrix, locus_names, pairs );
    }
    else if( options[ "method" ] == "logistic" )
    {
        logistic_method logistic( data );
        run_method( logistic, genotype_matrix, locus_names, pairs );
    }
    else if( options[ "method" ] == "logistic-factor" )
    {
        logistic_factor_method logistic( data );
        run_method( logistic, genotype_matrix, locus_names, pairs );
    }
    else if( options[ "method" ] == "loglinear" )
    {
        loglinear_method loglinear( data );
        run_method( loglinear, genotype_matrix, locus_names, pairs );
    }

    return 0;
}
