/**
* Copyright (C) 2014-2015 All rights reserved.
* @file config.h 
* @brief this file derived from elf 
* @author Weidong Huang (huangweidong01@baidu.com)
* @version 1.0
* @date 2015-10-09 21:36
*/
#ifndef BDL_LUNA_CONFIG_H
#define BDL_LUNA_CONFIG_H

#include <cxxabi.h>
#include <string>
#include <mutex>
#include <yaml-cpp/yaml.h>
#include <glog/logging.h>
#include "utility.h"

namespace bdl {
namespace luna {

/**
* @class Configure config.h "common/config.h" 
* @brief Used to store user's configurations and provide basic APIs for getting and setting 
* their values
* @details configure is singleton class, user can use it conveniently 
* mainly job: parse config file and to store kinds of config parameter
*/
class Configure {
public:
/**
* @defgroup Common 
* @brief Provide some common basic APIs for other modules,e.g. const definition, archive class, 
* configure class, etc
* @{
*/
/**
* @defgroup Configure 
* @brief Used to store user's configurations and provide basic APIs for getting and setting 
* their values 
* @{
*/
    /**
    * @brief read config file 
    * @param[in] path the path of config file 
    * @return false failed to read config file
    * @return true init configure class successfully 
    */
    bool init(const std::string& path);

    /**
    * @brief get config name's value 
    * @param[in] name config name that user want to get value 
    * @param[in] ret object where user assign the config name's value to
    * @return -1 failed to get config name's value
    * @return 0  get config name successfully 
    */
    template<class T>
    int get_by_name(const std::string& name, T& ret) {
        std::lock_guard<std::mutex> lock(_mutex);

        try {
            if (_config[name]) {
                ret = _config[name].as<T>();
                return 0;
            }
        } catch (YAML::Exception& e) {
            LOG(FATAL) << "get config [" << name << "] as type [" << type_name<T>()
                       << "] with error: " << e.msg;
        }

        return -1;
    }

    /**
    * @brief set config name by assign value
    * @param[in] name config name that user want to get value
    * @param[in] val value that user will set config name 
    * @return -1 set config name's value failed 
    * @return 0 set config name's value successfully
    */
    template<class T>
    int set_by_name(const std::string& name, const T& val) {
        std::lock_guard<std::mutex> lock(_mutex);

        try {
            _config[name] = val;
        } catch (YAML::Exception& e) {
            LOG(FATAL) << "set config [" << name << "] as type [" << type_name<T>()
                       << "] with error: " << e.msg;
        }

        return 0;
    }

    /**
    * @brief get bool value by name 
    * @param[in] name config name that user want to get value
    * @param[in] ret config name's value will assign 
    * @return -1 get config name's value failed 
    * @return 0 get config name's value successfully
    */
    int get_bool_by_name(const std::string& name, bool& ret);
    /**
    * @brief get int value by name 
    * @param[in] name config name that user want to get value
    * @param[in] ret config name's value will assign 
    * @return -1 get config name's value failed 
    * @return 0 get config name's value successfully
    */
    int get_int_by_name(const std::string& name, int& ret);
    /**
    * @brief get float value by name 
    * @param[in] name config name that user want to get value
    * @param[in] ret config name's value will assign 
    * @return -1 get config name's value failed 
    * @return 0 get config name's value successfully
    */
    int get_float_by_name(const std::string& name, float& ret);
    /**
    * @brief get double value by name 
    * @param[in] name config name that user want to get value
    * @param[in] ret config name's value will assign 
    * @return -1 get config name's value failed 
    * @return 0 get config name's value successfully
    */
    int get_double_by_name(const std::string& name, double& ret);
    /**
    * @brief get string value by name 
    * @param[in] name config name that user want to get value
    * @param[in] ret config name's value will assign 
    * @return -1 get config name's value failed 
    * @return 0 get config name's value successfully
    */
    int get_string_by_name(const std::string& name, std::string& ret);
    /**
    * @brief get pointer of configure 
    * @return YAML::Node* pointer of configure
    */
    YAML::Node* get_yaml_object();

    /**
    * @brief set bool value by name 
    * @param[in] name config name that user want to set value
    * @param[in] ret will assign value to config name 
    * @return -1 set config name's value failed 
    * @return 0 set config name's value successfully
    */
    int set_bool_by_name(const std::string& name, const bool& val);
    /**
    * @brief set int value by name 
    * @param[in] name config name that user want to set value
    * @param[in] ret will assign value to config name 
    * @return -1 set config name's value failed 
    * @return 0 set config name's value successfully
    */
    int set_int_by_name(const std::string& name, const int& val);
    /**
    * @brief set float value by name 
    * @param[in] name config name that user want to set value
    * @param[in] ret will assign value to config name 
    * @return -1 set config name's value failed 
    * @return 0 set config name's value successfully
    */
    int set_float_by_name(const std::string& name, const float& val);
    /**
    * @brief set double value by name 
    * @param[in] name config name that user want to set value
    * @param[in] ret will assign value to config name 
    * @return -1 set config name's value failed 
    * @return 0 set config name's value successfully
    */
    int set_double_by_name(const std::string& name, const double& val);
    /**
    * @brief set string value by name 
    * @param[in] name config name that user want to set value
    * @param[in] ret will assign value to config name 
    * @return -1 set config name's value failed 
    * @return 0 set config name's value successfully
    */
    int set_string_by_name(const std::string& name, const std::string& val);
    /**
    * @brief dump configure  
    * @return string the result of dump configure 
    */
    std::string dump();

    /**
    * @brief deprecate warning 
    * @param[in] old_config config name which deprecate
    * @param[in] new_config config name which will replace
    */
    void deprecate_warning(const std::string& old_config, const std::string& new_config);
    
    /**
    * @brief get the singleton intance of configure 
    * @return singlenton instance of configure 
    */
    static Configure& singleton() {
    	static Configure conf;
    	return conf;
    }

/**@}*/
/**@}*/

private:
    YAML::Node _config;
    std::mutex _mutex;
};

class TwoTierConfigure {
public:
    explicit TwoTierConfigure() : _higher_conf(nullptr), _lower_conf(nullptr) {}

    void set_higher_conf(std::shared_ptr<Configure> higher_conf) {
        std::lock_guard<std::mutex> lock(_mutex);
        _higher_conf = higher_conf;
    }

    std::shared_ptr<Configure> get_higer_conf() const {
        return _higher_conf;
    }

    void set_lower_conf(std::shared_ptr<Configure> lower_conf) {
        std::lock_guard<std::mutex> lock(_mutex);
        _lower_conf = lower_conf;
    }

    std::shared_ptr<Configure> get_lower_conf() const {
        return _lower_conf;
    }

    template<class T>
    int get_by_name(const std::string& name, T& ret) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_higher_conf != nullptr) {
            if (0 == _higher_conf->get_by_name<T>(name, ret)) {
                return 0;
            }
        }
        if (_lower_conf != nullptr) {
            return _lower_conf->get_by_name<T>(name, ret);
        }
        return -1;
    }

    template<class T>
    int set_by_name(const std::string& name, const T& val) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_higher_conf != nullptr) {
            if (0 == _higher_conf->set_by_name<T>(name, val)) {
                return 0;
            }
        }
        if (_lower_conf != nullptr) {
            return _lower_conf->set_by_name<T>(name, val);
        }
        return -1;
    }

protected:
    std::shared_ptr<Configure> _higher_conf;
    std::shared_ptr<Configure> _lower_conf;
    std::mutex _mutex;
};

} // namespace luna
} // namespace bdl

#endif // BDL_LUNA_CONFIG_H
