#include <iostream>
#include <sstream>
#include <string>
#include "Sentence.h"
#include "utils/AssertDef.h"
#include "utils/StringUtils.h"

namespace HieraParser
{
int Sentence::sid = 0;
Sentence* Sentence::CreateFromString(const std::string &line, const size_t kMaxFactorSize)
{
    Sentence *ret = new Sentence();
    ret->clear();
    std::vector<std::string> parts = StringSplit(line, "\t");
    assert(parts.size() == kMaxFactorSize);

    for (size_t i = 0; i < kMaxFactorSize; i++)
    {
        std::vector<std::string> tokens = StringSplit(parts[i], " ");
        assert(tokens.size() > 0);
        if (i == 0)
        {
            if (ret->empty())
                ret->resize(tokens.size());
        }
        for (size_t j = 0; j < tokens.size(); j++)
        {
            if ((*ret)[j].empty())
                (*ret)[j].resize(kMaxFactorSize);
           (*ret)[j][i] = FeatureFingerprint(tokens[j]);
        }
    }
    sid++; 
    return ret;
}

// std::string Sentence::GetString() const
// {
//     if (this->size() == 0)
//     {
//         return "";
//     }
//     std::stringstream ret;
//     for (size_t i = 0; i < this->size(); i++)
//     {
//         const WORD_UNIT &w = words_[i];
//         if (i == 0)
//             ret << w.GetString();
//         else
//             ret << " " << w.GetString();
//     }
//     return ret.str();
// }
// std::ostream &operator<<(std::ostream &out, const Sentence &sentence)
// {
//     out << sentence.GetString();
//     return out;
// }
} // namespace HieraParser
