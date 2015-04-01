#include <iostream>
#include <iomanip>

#include <cpp-argparse/OptionParser.h>

#include <bayesic/correct.hpp>
#include <bayesic/io/covariates.hpp>
#include <bayesic/io/metaresult.hpp>
#include <bayesic/io/resultfile.hpp>
#include <plink/plink_file.hpp>

using namespace optparse;

const std::string USAGE = "bayesic-correct result_file [result_file2 ...]";
const std::string VERSION = "Bayesic 0.0.1";
const std::string DESCRIPTION = "A tool for doing multiple testing correction.";
const std::string EPILOG = "";

std::vector<uint64_t> parse_tests(const std::string &num_tests, const std::string &method)
{
    if( method == "adaptive" )
    {
        return std::vector<uint64_t>( 4, 0 );
    }
    
    std::istringstream ss( num_tests );
    std::vector<uint64_t> parsed_tests;
    uint64_t test;
    while( ss >> test )
    {
        parsed_tests.push_back( test );
        ss.get( );
    }
    
    if( (method == "static" && parsed_tests.size( ) != 4) || (method != "static" && parsed_tests.size( ) != 1) )
    {
        std::cerr << "bayesic-correct: error: Bad number of tests, either 4 or 1." << std::endl;
        exit( 1 );
    }

    return parsed_tests;
}

std::vector<float> parse_weight(const std::string &weight_string, float default_value)
{
    if( weight_string.size( ) == 0 )
    {
        return std::vector<float>( 4, default_value );
    }

    std::vector<float> parsed_weights;
    std::istringstream ss( weight_string );
    float weight;
    float sum = 0.0;
    while( ss >> weight )
    {
        parsed_weights.push_back( weight );
        sum += weight;
        ss.get( );
    }

    if( fabs( sum - 1.0 ) > 1e-5 )
    {
        std::cerr << "bayesic-correct: error: Weights do not sum to 1.0." << std::endl;
        exit( 1 );
    }
    else if( parsed_weights.size( ) != 4 )
    {
        std::cerr << "bayesic-correct: error: Need to specify 4 weights." << std::endl;
        exit( 1 );
    }
    else
    {
        return parsed_weights;
    }
}

int
main(int argc, char *argv[])
{
    OptionParser parser = OptionParser( ).usage( USAGE )
                                         .version( VERSION )
                                         .description( DESCRIPTION )
                                         .epilog( EPILOG ); 

    char const * const methods[] = { "bonferroni", "static", "adaptive" };
    parser.add_option( "-m", "--method" ).set_default( "none" ).choices( &methods[ 0 ], &methods[ 3 ] ).help( "The multiple testing correction to use 'bonferroni', 'static' or 'adaptive (default = bonferroni)." );
    parser.add_option( "-b", "--bfile" ).help( "Plink prefix, needed for static and adaptive." );
    parser.add_option( "-p", "--pheno" ).help( "Phenotype file, possibly needed for static and adaptive." );
    parser.add_option( "-a", "--alpha" ).set_default( 0.05 ).help( "The significance threshold." );
    parser.add_option( "-f", "--field" ).set_default( 1 ).help( "For 'bonferroni', the column that contains the p-value, the first column after the snp names is 0 (default = 1)." );
    parser.add_option( "-n", "--num-tests" ).set_default( "0" ).help( "The number of tests to perform, if multiple, separate by ',' and 0 indicates let the program decide, typically the number of pairs." );
    parser.add_option( "-w", "--weight" ).help( "Used in 'static' and 'adaptive', 4 weights that sum to 1 separated by ','." );
    parser.add_option( "-o", "--output-prefix" ).help( "The output prefix, must be set!" );
    
    Values options = parser.parse_args( argc, argv );
    std::vector<std::string> args = parser.args( );
    if( args.size( ) < 1 )
    {
        std::cerr << "bayesic-correct: error: Need at least one result file." << std::endl;
        parser.print_help( );
        exit( 1 );
    }
    
    if( !options.is_set( "bfile" ) )
    {
        std::cerr << "bayesic-correct: error: Need to supply plink file with --bfile." << std::endl;
        exit( 1 );
    }
    plink_file_ptr genotype_file = open_plink_file( options[ "bfile" ] );
    
    correction_options correct;

    std::string method = (std::string) options.get( "method" );
    correct.alpha = (float) options.get( "alpha" );
    size_t field = (size_t) options.get( "field" );
    correct.num_tests = parse_tests( options[ "num_tests" ], method );
    correct.weight = parse_weight( options[ "weight" ], 0.25 );
    std::string output_prefix = (std::string) options.get( "output_prefix" );

    metaresultfile *meta_result_file = open_meta_result_file( args, genotype_file->get_locus_names( ) );
    if( method == "bonferroni" )
    {
        run_bonferroni( meta_result_file, correct.alpha, field, correct.num_tests[ 0 ], output_prefix );
    }
    else
    {
        if( !options.is_set( "output_prefix" ) )
        {
            std::cerr << "bayesic-correct: error: With static and adaptive an output prefix must be set with --output-prefix." << std::endl;
            exit( 1 );
        }

        genotype_matrix_ptr genotypes = create_genotype_matrix( genotype_file );

        method_data_ptr data( new method_data( ) );
        data->missing = arma::zeros<arma::uvec>( genotype_file->get_samples( ).size( ) );
        std::vector<std::string> order = genotype_file->get_sample_iids( );
        if( options.is_set( "pheno" ) )
        {
            std::ifstream phenotype_file( options[ "pheno" ].c_str( ) );
            data->phenotype = parse_phenotypes( phenotype_file, data->missing, order );
        }
        else
        {
            data->phenotype = create_phenotype_vector( genotype_file->get_samples( ), data->missing );
        }

        if( method == "static" )
        {
            run_static( meta_result_file, genotypes, data, correct, output_prefix );
        }
        else
        {
            run_adaptive( meta_result_file, genotypes, data, correct, output_prefix );
        }
    }

    return 0;
}
