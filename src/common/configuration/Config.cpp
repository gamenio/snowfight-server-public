#include "Config.h"

#include <algorithm>
#include <mutex>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "logging/Log.h"

using namespace boost::property_tree;

bool ConfigMgr::loadInitial(std::string const& file, std::string& error)
{
    std::lock_guard<std::mutex> lock(m_configMutex);

    m_filename = file;

    try
    {
        ptree fullTree;
        ini_parser::read_ini(file, fullTree);

        if (fullTree.empty())
        {
            error = "empty file (" + file + ")";
            return false;
        }

        // Since we're using only one section per config file, we skip the section and have direct property access
        m_config = fullTree.begin()->second;
    }
    catch (ini_parser::ini_parser_error const& e)
    {
        if (e.line() == 0)
            error = e.message() + " (" + e.filename() + ")";
        else
            error = e.message() + " (" + e.filename() + ":" + std::to_string(e.line()) + ")";
        return false;
    }

    return true;
}

ConfigMgr* ConfigMgr::instance()
{
    static ConfigMgr instance;
    return &instance;
}

bool ConfigMgr::reload(std::string& error)
{
    return loadInitial(m_filename, error);
}

template<class T>
T ConfigMgr::getValueDefault(std::string const& name, T def) const
{
    try
    {
        return m_config.get<T>(ptree::path_type(name, '/'));
    }
    catch (boost::property_tree::ptree_bad_path)
    {
        NS_LOG_WARN("server.loading", "Missing name %s in config file %s, add \"%s = %s\" to this file",
            name.c_str(), m_filename.c_str(), name.c_str(), std::to_string(def).c_str());
    }
    catch (boost::property_tree::ptree_bad_data)
    {
		NS_LOG_ERROR("server.loading", "Bad value defined for name %s in config file %s, going to use %s instead",
            name.c_str(), m_filename.c_str(), std::to_string(def).c_str());
    }

    return def;
}

template<>
std::string ConfigMgr::getValueDefault<std::string>(std::string const& name, std::string def) const
{
    try
    {
        return m_config.get<std::string>(ptree::path_type(name, '/'));
    }
    catch (boost::property_tree::ptree_bad_path)
    {
		NS_LOG_WARN("server.loading", "Missing name %s in config file %s, add \"%s = %s\" to this file",
            name.c_str(), m_filename.c_str(), name.c_str(), def.c_str());
    }
    catch (boost::property_tree::ptree_bad_data)
    {
		NS_LOG_ERROR("server.loading", "Bad value defined for name %s in config file %s, going to use %s instead",
            name.c_str(), m_filename.c_str(), def.c_str());
    }

    return def;
}

std::string ConfigMgr::getStringDefault(std::string const& name, const std::string& def) const
{
    std::string val = getValueDefault(name, def);
    val.erase(std::remove(val.begin(), val.end(), '"'), val.end());
    return val;
}

bool ConfigMgr::getBoolDefault(std::string const& name, bool def) const
{
    std::string val = getValueDefault(name, std::string(def ? "1" : "0"));
    val.erase(std::remove(val.begin(), val.end(), '"'), val.end());
    return (val == "1" || val == "true" || val == "TRUE" || val == "yes" || val == "YES");
}

int ConfigMgr::getIntDefault(std::string const& name, int def) const
{
    return getValueDefault(name, def);
}

float ConfigMgr::getFloatDefault(std::string const& name, float def) const
{
    return getValueDefault(name, def);
}

std::string const& ConfigMgr::getFilename()
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_filename;
}

std::list<std::string> ConfigMgr::getKeysByString(std::string const& name)
{
    std::lock_guard<std::mutex> lock(m_configMutex);

    std::list<std::string> keys;

    for (const ptree::value_type& child : m_config)
        if (child.first.compare(0, name.length(), name) == 0)
            keys.push_back(child.first);

    return keys;
}
