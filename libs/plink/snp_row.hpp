#ifndef __SNP_ROW_H__
#define __SNP_ROW_H__

#include <string>
#include <vector>

class snp_row
{
public:
    /**
     * Constructor.
     */
    snp_row();

    /**
     * Resizes the row to be able to hold the given size.
     *
     * @param new_size The new size of the row.
     */
    void resize(size_t new_size);

    /**
     * Returns the length of the row.
     *
     * @return The length of the row.
     */
    size_t size() const;

    /**
     * Access operator, not checked for bounds.
     *
     * @param index Index of the SNP to retrive.
     * 
     * @return Return the SNP at the given index.
     */
    unsigned char operator[](size_t index) const;

    /**
     * Access operator for assignment.
     *
     * @param index Index of the SNP to modify.
     * @param value The value to assign.
     */
    void assign(size_t index, unsigned char value);

private:
    /**
     * Size of the row.
     */

    size_t m_size;
    /**
     * Internal data structure, as a vector.
     */
    std::vector<unsigned int> m_genotypes;
};

#endif /* End of __SNP_ROW_H__ */
