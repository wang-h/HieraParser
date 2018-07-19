#ifndef ALIGNMENT_H
#define ALIGNMENT_H

#include <vector>
#include <set>
#include <string>
#include "utils/StringUtils.h"
#include "utils/AssertDef.h"
namespace HieraParser
{
typedef std::vector<std::set<int>> AlignmentTemplate;

class Alignment : public AlignmentTemplate
{
        // alignment object does not allow the input line as empty line.
      public:
        Alignment() : valid_(false){};
        Alignment(const size_t &size) : valid_(false) {resize(size);};
        ~Alignment(){};
        static Alignment *CreateFromString(const std::string &line);

      private:
        bool valid_;
};
} // namespace HieraParser
#endif