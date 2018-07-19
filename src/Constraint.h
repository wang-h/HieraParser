#ifndef BTGCONSTRAINT_H_
#define BTGCONSTRAINT_H_
#include <string>
#include <vector>
#include "Alignment.h"
namespace HieraParser
{

class Constraint: public std::vector<int>
{
      public:
        Constraint(){};
        Constraint(size_t size) {assign(size, -1); };
        ~Constraint(){};

        static Constraint *CreateFromString(const std::string &line);
        bool CheckBTGParsable();
        // void push_back(int val)
        // {
        //         constraint.push_back(val);
        // }
        // int operator[](size_t pos) const
        // {
        //         return constraint[pos];
        // }
        // int &operator[](size_t pos)
        // {
        //         return constraint[pos];
        // }
        
};

} // namespace HieraParser

#endif // BTGCONSTRAINT_H_