#ifndef STR_UTILS_H
#define STR_UTILS_H

  
#include <set>

#include <iostream>
#include <fstream>
#include <cmath>
#include <cassert>
#include <map>
#include <fstream>
#include <algorithm>
#include <limits>
#include <ctime>
#include <cmath>
#include <cfloat>
#include <queue>
#include <stdlib.h>
#include <cstdint>


// handle the hash functions using farmhash (google hash  library).


// handle the string operations, split, joint and replace.
inline std::vector<std::string> StringSplit(const std::string &str, const std::string &delim)
{
        std::vector<std::string> result;
        std::string::size_type p, q;
        for (p = 0; (q = str.find(delim, p)) != std::string::npos;
             p = q + delim.size())
        {
                result.emplace_back(str, p, q - p);
        }
        result.emplace_back(str, p);
        return result;
}
inline std::string StringJoin(const std::vector<std::string> &strs,
                   const std::string &delim)
{
        std::string result;
        if (!strs.empty())
        {
                result.append(strs[0]);
        }
        for (size_t i = 1; i < strs.size(); i++)
        {
                result.append(delim);
                result.append(strs[i]);
        }
        return result;
}

inline void StringReplace(const std::string &src, const std::string &dst,
                    std::string *str)
{
        std::string::size_type pos = 0;
        while ((pos = str->find(src, pos)) != std::string::npos)
        {
                str->replace(pos, src.size(), dst);
                pos += dst.size();
        }
}

#endif