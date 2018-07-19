#include "Constraint.h"
namespace HieraParser
{

inline bool LessOrEqualAlignment(const std::set<int> &lhs,
                                 const std::set<int> &rhs)
{
    for (auto &x : lhs)
        if (rhs.find(x) == rhs.end())
            for (auto &y : rhs)
                if (x > y)
                    return false;
    for (auto &y : rhs)
        if (lhs.find(y) == lhs.end())
            for (auto &x : lhs)
                if (x > y)
                    return false;
    return true;
}

Constraint *Constraint::CreateFromString(const std::string &line)
{
    Alignment *ret = Alignment::CreateFromString(line);
    if (ret == nullptr)
        return nullptr;
    const Alignment &alignment = *ret;
    Constraint *constraint = new Constraint(alignment.size());

    std::vector<std::vector<int>> sorted_indices;
    for (size_t i = 0; i < alignment.size(); i++)
    {
        if (alignment[i].empty())
            continue;
        bool eq = false;
        size_t j;
        for (j = 0; j < sorted_indices.size(); j++)
        {
            const bool le = LessOrEqualAlignment(
                alignment[i], alignment[sorted_indices[j].front()]);
            const bool ge = LessOrEqualAlignment(
                alignment[sorted_indices[j].front()], alignment[i]);
            if (!le && !ge)
                return nullptr;
            eq = (le && ge);
            if (le)
                break;
        }
        if (!eq)
        {
            sorted_indices.insert(sorted_indices.begin() + j, std::vector<int>());
        }
        sorted_indices[j].push_back(i);
    }
    for (size_t i = 0; i < sorted_indices.size(); i++)
    {
        for (auto j : sorted_indices[i])
        {
            (*constraint)[j] = i;
        }
    }
    // Push the number of target-side tokens at the end of the vector.
    constraint->push_back(sorted_indices.size());
    if (constraint->CheckBTGParsable())
        return constraint;
    return nullptr;
}

bool Constraint::CheckBTGParsable()
{
    if (this->empty())
        return false;
    std::vector<int> lmin(this->size() - 1);
    std::vector<int> lmax(this->size() - 1);
    std::vector<int> rmin(this->size() - 1);
    std::vector<int> rmax(this->size() - 1);
    std::vector<std::pair<int, int>> stack;

    std::vector<int> positions;
    for (size_t i = 0; i < this->size() - 1; i++)
    {
        if ((*this)[i] >= 0)
            positions.push_back((*this)[i]);
    }

    stack.push_back({0, positions.size()});
    while (!stack.empty())
    {
        const auto span = stack.back();
        stack.pop_back();
        for (int i = 0; i < span.second - span.first; i++)
        {
            const int p = positions[span.first + i];
            if (i == 0)
            {
                lmin[i] = p;
                lmax[i] = p;
            }
            else
            {
                lmin[i] = std::min(lmin[i - 1], p);
                lmax[i] = std::max(lmax[i - 1], p);
            }
        }
        for (int i = span.second - span.first - 1; i >= 0; i--)
        {
            const int p = positions[span.first + i];
            if (i == span.second - span.first - 1)
            {
                rmin[i] = p;
                rmax[i] = p;
            }
            else
            {
                rmin[i] = std::min(rmin[i + 1], p);
                ;
                rmax[i] = std::max(rmax[i + 1], p);
                ;
            }
        }
        int split = -1;
        for (int i = 1; i < span.second - span.first; i++)
        {
            if (lmax[i - 1] <= rmin[i] || rmax[i] <= lmin[i - 1])
            {
                split = i;
                break;
            }
        }
        if (split < 0)
            return false;
        const int third = span.first + split;
        if (third - span.first > 1)
        {
            stack.push_back({span.first, third});
        }
        if (span.second - third > 1)
        {
            stack.push_back({third, span.second});
        }
    }
    return true;
}
} // namespace HieraParser
