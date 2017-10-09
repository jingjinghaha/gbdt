/**
* Copyright (C) 2014-2015 All rights reserved.
* @file flags.cpp 
* @brief gflags define the global variables
* @author Weidong Huang (huangweidong01@baidu.com)
* @version 1.0
* @date 2015-10-09 23:20
*/
#include "flags.h"
#include <memory>
#include <boost/filesystem.hpp>

namespace bdl {
namespace luna {

DEFINE_string(config, "config.yaml", "config file name");
//DEFINE_string(log_dir, "./log", "log path");
DEFINE_string(coordinator_host, "tcp://localhost:9158", "coordinator host");
DEFINE_int32(node_rank, -1, "node rank");
DEFINE_int32(node_num, 0, "node num");
DEFINE_int32(debug_level, 0, "debug level");
DEFINE_string(redis_ip, "10.222.21.37:16379", "redis ip string");
DEFINE_string(redis_key, "ELF_Program", "reids key string");
DEFINE_int32(communication_secret, 931614275, "communication secret");
DEFINE_bool(enable_performance_moniter, false, "Enable performance moniter");
DEFINE_bool(enable_web_moniter, false, "Enable web moniter");
DEFINE_bool(enable_mock, false, "Enable to mock the code");
DEFINE_string(plan_dag_json_file, "web/static/plan_dag.json", "Plan DAG json dump file");

void gflags_and_glog_init(int argc, char* argv[]) {
    //FLAGS_logtostderr = false;
    FLAGS_logtostderr = true;
    FLAGS_log_dir = "./log/luna_INFO";
    std::string ps_version("ps ");
    ps_version.append(LUNA_VERSION);
    ps_version.append(" Program.");
    google::SetVersionString(ps_version);
    // This is to avoid gflags to rearrange the arguments in argv.
    char** copy_argv = new char* [argc + 1];

    for (int i = 0; i <= argc; ++i) {
        copy_argv[i] = argv[i];
    }

    google::AllowCommandLineReparsing();
    google::ParseCommandLineFlags(&argc, &copy_argv, false);
    delete [] copy_argv;
    copy_argv = NULL;

    boost::filesystem::path p(FLAGS_log_dir);

    if (!boost::filesystem::exists(p)) {
        boost::filesystem::create_directory(p);
        CHECK(boost::filesystem::exists(p))
                << "create log directory: " << FLAGS_log_dir << " failed.";
    }

    google::InstallFailureSignalHandler();
    google::InitGoogleLogging(argv[0]);
}

} // namespace luna
} // namespace bdl
