#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <mutex>

#include <boost/property_tree/ptree.hpp>

#include "Common.h"

class ConfigMgr
{
    ConfigMgr() = default;
    ConfigMgr(ConfigMgr const&) = delete;
    ConfigMgr& operator=(ConfigMgr const&) = delete;
    ~ConfigMgr() = default;

public:
    bool loadInitial(std::string const& file, std::string& error);

    static ConfigMgr* instance();

    bool reload(std::string& error);

    std::string getStringDefault(std::string const& name, const std::string& def) const;
    bool getBoolDefault(std::string const& name, bool def) const;
    int getIntDefault(std::string const& name, int def) const;
    float getFloatDefault(std::string const& name, float def) const;

    std::string const& getFilename();
    std::list<std::string> getKeysByString(std::string const& name);

private:
    std::string m_filename;
    boost::property_tree::ptree m_config;
    std::mutex m_configMutex;

    template<class T>
    T getValueDefault(std::string const& name, T def) const;
};

#define sConfigMgr ConfigMgr::instance()

#endif //__CONFIG_H__
