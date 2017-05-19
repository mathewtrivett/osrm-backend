#ifndef OSRM_UTIL_PERMUTATION_HPP
#define OSRM_UTIL_PERMUTATION_HPP

#include "util/integer_range.hpp"

#include <vector>

namespace osrm
{
namespace util
{

template <typename RandomAccesIterator, typename IndexT>
void inplacePermutation(RandomAccesIterator begin,
                        RandomAccesIterator end,
                        const std::vector<IndexT> &old_to_new)
{
    std::size_t size = std::distance(begin, end);
    BOOST_ASSERT(old_to_new.size() == size);
    // we need a little bit auxililary space since we need to mark
    // replaced elements in a non-destructive way
    std::vector<bool> was_replaced(size, false);
    for (auto index : util::irange<IndexT>(0, size))
    {
        if (was_replaced[index])
            continue;

        // iterate over a cycle in the permutation
        auto buffer = begin[index];
        auto old_index = index;
        auto new_index = old_to_new[old_index];
        for (; new_index != index; old_index = new_index, new_index = old_to_new[new_index])
        {
            was_replaced[old_index] = true;
            std::swap(buffer, begin[new_index]);
        }
        was_replaced[old_index] = true;
        std::swap(buffer, begin[index]);
    }
}
}
}

#endif
