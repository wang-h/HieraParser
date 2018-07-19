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

// handle the assert operations.
#define CHECK(condition, message)                                                                     \
        do                                                                                            \
        {                                                                                             \
                if (!(condition))                                                                     \
                {                                                                                     \
                        std::cerr << "ERROR(" << __FILE__ << ":" << __LINE__ << ") " << (message) << std::endl; \
                        abort();                                                                      \
                }                                                                                     \
        } while (0);
;

#define HELP_MESSAGE(message)          \
    do                                                                                            \
        {                                                                                             \
               std::cerr << "ERROR(" << __FILE__ << ":" << __LINE__ << ") " << (message) << std::endl; \
              abort();                                                                       \
        } while (0); 
;

#define THROW_ERROR(message)                       \
    do                                                                                            \
        {                                                                                             \
                 throw std::runtime_error(std::string("Failed: ") + message);\
                 abort(); \
        } while (0);
;