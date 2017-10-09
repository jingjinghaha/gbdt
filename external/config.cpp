/**
* Copyright (C) 2014-2015 All rights reserved.
* @file config.cpp 
* @brief this file derived from elf   
* @author Weidong Huang (huangweidong01@baidu.com)
* @version 1.0
* @date 2015-10-09 23:26
*/
#include "config.h"

namespace bdl {
namespace luna {

bool Configure::init(const std::string& path) {
    try {
        _config = YAML::LoadFile(path);
        return true;
    } catch (YAML::Exception& e) {
        LOG(FATAL) << "parse yaml config file " << path << " with error: " << e.what();
    }

    return false;
}

int Configure::get_bool_by_name(const std::string& name, bool& ret) {
    return get_by_name<bool>(name, ret);
}

int Configure::get_int_by_name(const std::string& name, int& ret) {
    return get_by_name<int>(name, ret);
}

int Configure::get_float_by_name(const std::string& name, float& ret) {
    return get_by_name<float>(name, ret);
}

int Configure::get_double_by_name(const std::string& name, double& ret) {
    return get_by_name<double>(name, ret);
}

YAML::Node* Configure::get_yaml_object() {
    return &_config;
}

int Configure::get_string_by_name(const std::string& name, std::string& ret) {
    return get_by_name<std::string>(name, ret);
}

int Configure::set_bool_by_name(const std::string& name, const bool& val) {
    return set_by_name<bool>(name, val);
}

int Configure::set_int_by_name(const std::string& name, const int& val) {
    return set_by_name<int>(name, val);
}

int Configure::set_float_by_name(const std::string& name, const float& val) {
    return set_by_name<float>(name, val);
}

int Configure::set_double_by_name(const std::string& name, const double& val) {
    return set_by_name<double>(name, val);
}

int Configure::set_string_by_name(const std::string& name, const std::string& val) {
    return set_by_name<std::string>(name, val);
}

std::string Configure::dump() {
    return YAML::Dump(_config);
}

void Configure::deprecate_warning(const std::string& old_config, const std::string& new_config) {
    LOG(WARNING) << "config [" << old_config << "] is deprecated, please use ["
                 << new_config << "] instead";
}

} // namespace luna
} // namespace bdl
