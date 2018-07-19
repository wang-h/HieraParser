#include "Alignment.h"
namespace HieraParser
{

Alignment *Alignment::CreateFromString(const std::string &line)
{
        const std::vector<std::string> &parts = StringSplit(line, " ||| ");
        std::string raw;
        if (parts.size() == 2)
        {
                if (parts[1].size() == 0)
                        return nullptr;
        }
        raw = parts[1];
        const size_t sourceSize = std::stoul(StringSplit(parts[0], "-")[0]);

        const std::vector<std::string> &tokens = StringSplit(raw, " ");
        CHECK(tokens.size() > 0, "#ERROR [Alignment], empty line:" + line);
        CHECK(tokens[0] != "\n", "#ERROR [Alignment], empty line:" + line);
        Alignment *ret = new Alignment(sourceSize);
        for (size_t j = 0; j < tokens.size(); j++)
        {
                const std::vector<std::string> &indices = StringSplit(tokens[j], "-");
                CHECK(indices.size() == 2, "#ERROR [Alignment],  invalid line: " + line);
                int srcId = std::stoi(indices[0]);
                int trgId = std::stoi(indices[1]);
                CHECK((srcId >= 0) && (srcId < static_cast<int>(sourceSize)), "#ERROR [Alignment],  invalid line: " + line);
                (*ret)[srcId].insert(trgId);
        }
        return ret;
}
} // namespace HieraParser
