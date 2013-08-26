#include <method/method.hpp>

void run_method(method_type &method, const std::vector<snp_row> &genotype_matrix, const std::vector<pio_locus_t> &loci, pair_iter &pairs)
{
    std::pair<size_t, size_t> pair;
    while( pairs.get_pair( &pair ) )
    {
        const snp_row &row1 = genotype_matrix[ pair.first ];
        const snp_row &row2 = genotype_matrix[ pair.second ];

        std::string name1 = loci[ pair.first ].name;
        std::string name2 = loci[ pair.second ].name;

        method.run( row1, row2, name1, name2 );
    }
}
