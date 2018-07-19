#include "Config.h"

namespace HieraParser
{


Config::Config(){ 
       // set usage
        SetUsage(
                 "***********************************************************************\n"
                 "                          HieraParser v2.0                             \n"
                 "A fast Top-down Bracketing Transduction Grammar parser for preordering,\n" 
                 "based on Top-Down BTG-based Preorderer, extended to support multi-threading.\n"
                 "***********************************************************************\n"
                 " Copyright (C) 2017 Hao WANG, Waseda University.                       \n"
                 "                                                                       \n"
                 "***********************************************************************\n");
          
        
};
    
Config::~Config(){ };



int Config::ReadMainArgs(int argc, char **argv)
{
    
    int i = 0;
    while (i < argc)
    {
        if (argv[i][0] == '-')
        {
            std::string mainArgName(argv[i] + 1);
            if (argc<1)
            {
                return 1;
            }
            else if (!mainArgName.compare("h") || !mainArgName.compare("help"))
            {
                return 1;
            }
            auto it = mainArgs_.find(mainArgName);
            if (it == mainArgs_.end()){
                 HELP_MESSAGE("illegal argument: " + mainArgName);
                 return 1;
            } 
            if (i == argc - 1 || argv[i + 1][0] == '-')
                it->second.first = "true";
            else
                it->second.first = argv[++i];
        }
        i++;
    }
    return 0;
}

void Config::SetUsage(const std::string &str)
{
    usage_ = str;  
}

void Config::LoadConfigFile(const std::string &cfg_file)
{
    std::ifstream cfg_in(cfg_file.c_str());
    if (!cfg_in){
        THROW_ERROR("Could not open config file or file does not exist " + cfg_file);
    } 
    std::string line;
    while (getline(cfg_in, line))
    {
        if (line.size() == 0 || line[0] == '#')
            continue;
        if (line[0] == '[' || line[line.length() - 1] == ']')
        {
            std::string section = line.substr(1, line.size() - 2);
            std::stringstream tmp;
            while (getline(cfg_in, line) && line.size() != 0)
            {
                std::vector<std::string> parts = StringSplit(line, "=");
                //&& 
                 if(parts.size() == 2){
                     if(!section.compare("weights")){
                        SetString(parts[0], parts[1]);
                     }
                     else{
                         if  (!tmp.str().empty())
                                 tmp << "|"; 
                         tmp << line; 
                     }
                } 
                else if (parts.size() == 1)
                {
                    SetString(section, parts[0]);
                }
            }
            if (!(tmp.str().empty()) && section.compare("weights"))
            {
                SetString(section, tmp.str());
                tmp.str("");
            }
        }
    }
}

void Config::PrintUsage() const
{
    // print  usage
    std::cerr << usage_ << std::endl;
    std::cerr << "[main arguments]:" << std::endl;
    for (auto it = mainArgs_.begin(); it != mainArgs_.end(); it++)
        std::cerr << "\t"
                  << "-" << it->first << ": " << it->second.second << std::endl; 
}

void Config::PrintConfig() const
{
    // print  configuration
    std::cerr << "[configuration arguments]:" << std::endl;
    for (auto it = configArgs_.begin(); it != configArgs_.end(); it++)
    {
        if (it->second.first.length()!= 0)
            std::cerr  << it->first << ": " << it->second.second << "\n\t["<< it->second.first <<"]"<< std::endl;
    }
}

void Config::AddConfigEntry(const std::string &key, const std::string &val, const std::string &detail, const bool &is_main)
{
    std::pair<std::string, std::pair<std::string, std::string>> entry(key, std::pair<std::string, std::string>(val, detail));
    if (is_main)
        mainArgs_.insert(entry);
    else
        configArgs_.insert(entry);
}

// Getter functions

std::string Config::GetString(const std::string &key) const
{
    ConfigMap::const_iterator mit = mainArgs_.find(key);
    if (mit != mainArgs_.end())
        return mit->second.first;
    ConfigMap::const_iterator cit = configArgs_.find(key);
    if (cit != configArgs_.end())
        return cit->second.first;
    return "";
    // THROW_ERROR("Requesting bad argument "+ key + " from configuration");
}



int Config::GetInt(const std::string &key) const
{
    std::string str = GetString(key);
    int ret = std::stoi(str);
    // CHECK((ret != 0), 
    //                     "Value '" + str + "' for argument " + key + " was not an integer");
    return ret;
}

float Config::GetFloat(const std::string &key) const
{
    std::string str = GetString(key);
    float ret = std::stof(str);
    CHECK( (ret != 0 ), 
                      "Value '" + str + "' for argument " + key + " was not float");
    return ret;
}
bool Config::GetBool(const std::string &key) const
{
    std::string str = GetString(key);
    if (str == "true")
        return true;
    else if (str == "false")
        return false;
    else if (str == "")
        return true;
    THROW_ERROR("Value '" + str + "' for argument " + key + " was not boolean");
    return false;
}

bool Config::Contains(const std::string &key) const
{
    std::string str = GetString(key);
    if (str == "")
        return false;
    else
        return true;
}

// Setter functions
void Config::SetString(const std::string &key, const std::string &val)
{
    ConfigMap::iterator it = configArgs_.find(key);
    CHECK( (it != configArgs_.end()), "No such an argument [" + key + "] in configuration");
    it->second.first = val;
}

// const string &ConfigBase::GetMainArg(int id) const
// {
//     if (id >= (int)mainArgs_.size())
//         throw std::runtime_error("Argument request is out of bounds");
//     return mainArgs_[id];
// }

void Config::SetInt(const std::string &key, int val)
{
    std::ostringstream oss;
    oss << val;
    SetString(key, oss.str());
}

void Config::SetDouble(const std::string &key, double val)
{
    std::ostringstream oss;
    oss << val;
    SetString(key, oss.str());
}

void Config::SetBool(const std::string &key, bool val)
{
    std::ostringstream oss;
    oss << val;
    SetString(key, oss.str());
}

std::vector<std::string> Config::GetStringArray(const std::string &key) const
{
    ConfigMap::const_iterator it = configArgs_.find(key);
    std::vector<std::string> ret;
    CHECK(it != configArgs_.end(), "Requesting bad argument " + key + " from configuration");
    if (it->second.first.length() != 0)
        ret = StringSplit(it->second.first, " ");
    return ret;
}


std::vector<float> Config::GetFloatArray(const std::string &key) const
{
    std::vector<std::string>  stringVector = GetStringArray(key);
    std::vector<float> ret(stringVector.size()); 
    std::transform(stringVector.begin(), stringVector.end(), ret.begin(), 
        [](const std::string& val)
        {
            return std::stof(val);
        }); 
    return ret;
}

std::vector<std::pair<std::string,std::string>> Config::GetStringPairArray(const std::string &key) const
{
    std::vector<std::string>  stringVector = StringSplit(GetString(key), "|");
    std::vector<std::pair<std::string,std::string>> ret(stringVector.size()); 
    std::transform(stringVector.begin(), stringVector.end(), ret.begin(), 
        [](const std::string& val)
        {
            std::vector<std::string>  parts = StringSplit(val, "=");
            return std::make_pair(parts[0], parts[1]);
        }); 
    return ret;
}
}