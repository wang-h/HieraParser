#ifndef BASE_CONFIG_H__
#define BASE_CONFIG_H__

#include <fstream>
#include <sstream>
#include <unordered_map>

#include "utils/AssertDef.h"
#include "utils/TypeDef.h"
#include "utils/StringUtils.h"

namespace HieraParser
{



class Config
{

  private:
    // argument functions
    // details for printing the usage
    std::string usage_; // usage details 
    
    bool initialized_;

    typedef std::unordered_map<std::string, std::pair<std::string, std::string>> ConfigMap;
    ConfigMap mainArgs_; 
    ConfigMap configArgs_; 
      
  public:
    Config();
    ~Config();
    // public interfaces
    void SetUsage(const std::string &str);
    void PrintUsage() const;
    bool IsInitialized() const {return initialized_;};
    int  ReadMainArgs(int argc, char **argv); 
    
    void LoadConfigFile(const std::string &cfg_file);
    void PrintConfig() const;
    void AddConfigEntry(const std::string &key, const std::string &val, const std::string &detail, const bool &isMainArg=false);


    // Get
    
    int    GetInt(const std::string &key) const;
    bool   GetBool(const std::string &key) const;
    bool   Contains(const std::string &key) const;
    float GetFloat(const std::string &key) const; 
    std::string GetString(const std::string &key) const;

    std::vector<std::string> GetStringArray(const std::string &key) const;
    std::vector<float> GetFloatArray(const std::string &key) const;
    std::vector<std::pair<std::string,std::string>> GetStringPairArray(const std::string &key) const; 
    // Set
    
    void SetInt(const std::string &key, int val);
    void SetBool(const std::string &key, bool val);
    void SetDouble(const std::string &key, double val);
    void SetString(const std::string &key, const std::string &val);

    
};
}
#endif
