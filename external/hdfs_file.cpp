/**
* Copyright (C) 2014-2015 All rights reserved.
* @file hdfs_file.cpp 
* @brief hdfs operation function  
* @author Weidong Huang (huangweidong01@baidu.com)
* @version 1.0
* @date 2015-10-08 20:48
*/
#include "hdfs_file.h"
#include <signal.h>
#include <stdlib.h>
#include <boost/algorithm/string.hpp>
#include "common/system.h"
#include "common/config.h"
#include "common/flags.h"
//#include "common/channel.h"
#include "file_common.h"

namespace bdl {
namespace luna {

const int32_t READBOCKSIZE = LARGE_BUFFER_SIZE;

HdfsFile::HdfsFile(const int32_t retry_num, const std::string& fs_name, const std::string& ugi,
                   const std::string& hdfs_log_file)
        : _retry_num(retry_num), _fs_name(fs_name), _ugi(ugi), _log_file(hdfs_log_file) {
    _codec = FileCompressionCodecs::DEFAULT_CODEC;
    inner_init();
}

HdfsFile::HdfsFile(const int32_t block_per_line,
                   const std::string& fs_name,
                   const std::string& fs_ugi,
                   const std::string& log_file,
                   int32_t retry_num) {
    _codec = FileCompressionCodecs::DEFAULT_CODEC;

    _block_per_line = block_per_line;
    inner_init(fs_name, fs_ugi, log_file, retry_num);
}

HdfsFile::HdfsFile(TwoTierConfigure* conf, FileCompressionCodecs codec)
        : _p_conf(conf), _codec(codec) {
    inner_init();
}

void HdfsFile::inner_init() {
    Configure& conf = Configure::singleton();
    CHECK(0 == conf.get_string_by_name("hdfs_fsName", _fs_name))
            << "failed to get config [hdfs_fsName] from config file";
    CHECK(0 == conf.get_string_by_name("hdfs_fsUgi", _ugi))
            << "failed to get config [hdfs_fsUgi] from config file";
    _retry_num = 10;
    conf.get_int_by_name("hdfs_defaultRetryNum", _retry_num);
    _log_file = FLAGS_log_dir;
    _log_file.append("/hdfs_err.log");
    conf.get_string_by_name("hdfs_logerrFile", _log_file);
 
    _hdfs_cmd_prefix.assign("hadoop fs -Dfs.default.name=").append(_fs_name);
    _hdfs_cmd_prefix.append(" -Dhadoop.job.ugi=").append(_ugi).append(" ");
    _hdfs_cmd_postfix.assign(" 2>>").append(_log_file);
    int32_t cmd_ret_val = 0;
    std::string cmd("hadoop version ");
    cmd.append(_hdfs_cmd_postfix);
    auto exec_ret = system_exec_no_output(cmd, _retry_num, cmd_ret_val);
    CHECK(exec_ret == 0) << "get hadoop version failed.";
    if (cmd_ret_val != 0) {
        VLOG(0) << "hadoop version command execute failed!";
    }
}

void HdfsFile::inner_init(const std::string& fs_name, const std::string& fs_ugi,
                          const std::string& log_file, int32_t retry_num) {
    //Configure& conf = Configure::singleton();
    //CHECK(0 == conf.get_string_by_name("hdfs_fsName", _fs_name))
    //        << "failed to get config [hdfs_fsName] from config file";
    //CHECK(0 == conf.get_string_by_name("hdfs_fsUgi", _ugi))
    //        << "failed to get config [hdfs_fsUgi] from config file";
    _fs_name = fs_name;
    _ugi = fs_ugi;
    _retry_num = 10;
    //conf.get_int_by_name("hdfs_defaultRetryNum", _retry_num);
    _retry_num = retry_num;
    _log_file = FLAGS_log_dir;
    _log_file.append("/hdfs_err.log");
    //conf.get_string_by_name("hdfs_logerrFile", _log_file);
    _log_file = retry_num;
 
    _hdfs_cmd_prefix.assign("hadoop fs -Dfs.default.name=").append(_fs_name);
    _hdfs_cmd_prefix.append(" -Dhadoop.job.ugi=").append(_ugi).append(" ");
    _hdfs_cmd_postfix.assign(" 2>>").append(_log_file);
    int32_t cmd_ret_val = 0;
    std::string cmd("hadoop version ");
    cmd.append(_hdfs_cmd_postfix);
    auto exec_ret = system_exec_no_output(cmd, _retry_num, cmd_ret_val);
    CHECK(exec_ret == 0) << "get hadoop version failed.";
    if (cmd_ret_val != 0) {
        VLOG(0) << "hadoop version command execute failed!";
    }
}

int32_t HdfsFile::list_directory(const char* path, std::deque<FileInfo>& files) {
    CHECK(path) << "input parameter path must not be null";
    files.clear();
    std::string cmd(_hdfs_cmd_prefix);
    cmd.append(" -ls ").append(path).append(_hdfs_cmd_postfix);
    char* cmd_buf = nullptr;
    size_t in_buf_len = 0;
    size_t out_buf_len = 0;
    int32_t cmd_ret = 0;
    int32_t exec_ret = system_exec(cmd, _retry_num, true, cmd_buf, in_buf_len, out_buf_len, cmd_ret);

    if (exec_ret < 0 || 0 != cmd_ret) {
        free(cmd_buf);
        return PS_OPERATE_FAILED;
    }

    std::string cmd_buf_str(cmd_buf);
    std::vector<std::string> tmp_record_vec;
    boost::split(tmp_record_vec, cmd_buf_str, boost::is_any_of("\n"), boost::token_compress_on);

    for (auto tmp_record : tmp_record_vec) {
        std::vector<std::string> tmp_field_vec;
        boost::split(tmp_field_vec, tmp_record, boost::is_any_of(" "), boost::token_compress_on);

        if (tmp_field_vec.size() < 4) {
            continue;
        }

        size_t file_name_pos = tmp_field_vec.size() - 1;
        size_t file_size_pos = tmp_field_vec.size() - 4;
        files.push_back({tmp_field_vec[file_name_pos], std::stoll(tmp_field_vec[file_size_pos])});
    }

    free(cmd_buf);

    return PS_SUCCESS;
}

int32_t HdfsFile::get_file_size(const char* path, uint64_t file_size) {
    CHECK(path) << "input parameter path must not be null";
    std::string cmd(_hdfs_cmd_prefix);
    cmd.append(" -ls ").append(path).append(_hdfs_cmd_postfix);
    char* cmd_buf = nullptr;
    size_t in_buf_len = 0;
    size_t out_buf_len = 0;
    int32_t cmd_ret = 0;
    int32_t exec_ret = system_exec(cmd, _retry_num, true, cmd_buf, in_buf_len, out_buf_len, cmd_ret);

    if (exec_ret < 0 || 0 != cmd_ret) {
        free(cmd_buf);
        return PS_OPERATE_FAILED;
    }

    std::string cmd_buf_str(cmd_buf);
    std::vector<std::string> tmp_record_vec;
    boost::split(tmp_record_vec, cmd_buf_str, boost::is_any_of("\n"), boost::token_compress_on);

    for (auto tmp_record : tmp_record_vec) {
        std::vector<std::string> tmp_field_vec;
        boost::split(tmp_field_vec, tmp_record, boost::is_any_of(" "), boost::token_compress_on);
       
        if (tmp_field_vec.size() < 5) {
            continue;
        }

        file_size = boost::lexical_cast<uint64_t>(tmp_field_vec.at(4));
        VLOG(1) << "dus directory path:" << path << "file size:" << file_size;
    }

    free(cmd_buf);

    return PS_SUCCESS;
}

int32_t HdfsFile::dus_directory(const char* path, uint64_t dir_size) {
    CHECK(path) << "input parameter path must not be null";
    std::string cmd(_hdfs_cmd_prefix);
    cmd.append(" -dus ").append(path).append(_hdfs_cmd_postfix);
    char* cmd_buf = nullptr;
    size_t in_buf_len = 0;
    size_t out_buf_len = 0;
    int32_t cmd_ret = 0;
    int32_t exec_ret = system_exec(cmd, _retry_num, true, cmd_buf, in_buf_len, out_buf_len, cmd_ret);


    if (exec_ret < 0 || 0 != cmd_ret) {
        free(cmd_buf);
        return PS_OPERATE_FAILED;
    }

    std::string cmd_buf_str(cmd_buf);
    std::vector<std::string> tmp_record_vec;
    boost::split(tmp_record_vec, cmd_buf_str, boost::is_any_of("\n"), boost::token_compress_on);

    for (auto tmp_record : tmp_record_vec) {
        std::vector<std::string> tmp_field_vec;
        boost::split(tmp_field_vec, tmp_record, boost::is_any_of(" "), boost::token_compress_on);
        
        dir_size = boost::lexical_cast<uint64_t>(tmp_field_vec.at(0));
        VLOG(1) << "dus directory path:" << path << "dir size:" << dir_size;
    }

    free(cmd_buf);

    return PS_SUCCESS;
}

int32_t HdfsFile::delete_directory(const char* path) {
    CHECK(path) << "input parameter path must not be null";
    std::string cmd(_hdfs_cmd_prefix);
    cmd.append(" -rmr ").append(path).append(_hdfs_cmd_postfix);
    int32_t cmd_ret_val = 0;
    int32_t exec_ret = system_exec_no_output(cmd, _retry_num, cmd_ret_val);

    if (exec_ret < 0) {
        return PS_OPERATE_FAILED;
    }

    switch (cmd_ret_val) {
    case 0:
        return PS_SUCCESS;
        break;

    case 255:
        return PS_SUCCESS;
        break;

    default:
        return PS_OPERATE_FAILED;
        break;
    }
}

int32_t HdfsFile::make_directory(const char* path) {
    CHECK(path) << "input parameter path must not be null";
    std::string cmd(_hdfs_cmd_prefix);
    cmd.append(" -mkdir ").append(path).append(_hdfs_cmd_postfix);
    int32_t cmd_ret_val = 0;
    int32_t exec_ret = system_exec_no_output(cmd, _retry_num, cmd_ret_val);

    if (exec_ret < 0) {
        return PS_OPERATE_FAILED;
    }

    switch (cmd_ret_val) {
    case 0:
        return PS_SUCCESS;
        break;

    case 255:
        return PS_SUCCESS;
        break;

    default:
        return PS_OPERATE_FAILED;
        break;
    }
}

int32_t HdfsFile::exist(const char* path) {
    CHECK(path) << "input parameter path must not be null";
    std::string cmd(_hdfs_cmd_prefix);
    cmd.append(" -test -e ").append(path).append(_hdfs_cmd_postfix);
    int32_t cmd_ret_val = 0;
    int32_t exec_ret = system_exec_no_output(cmd, _retry_num, cmd_ret_val);

    if (exec_ret < 0) {
        return PS_OPERATE_FAILED;
    }

    switch (cmd_ret_val) {
    case 0:
        return PS_SUCCESS;
    case 1:
        return PS_SUCCESS;
    default:
        return PS_OPERATE_FAILED;
    }
}

int32_t HdfsFile::read_data(const char* file_name_with_path,
                                  std::vector<std::string>& file_vec) {
    CHECK(file_name_with_path) << "input parameter file_name_with_path must not be null";
    std::string cmd(_hdfs_cmd_prefix);
    cmd.append(" -text ").append(file_name_with_path).append(_hdfs_cmd_postfix);
    if (_codec == FileCompressionCodecs::BZIP2_CODEC) {
        cmd.append(" | bzip2 -d ");
    }
    FILE* fd = popen_safe(cmd.c_str(), "r");
    CHECK(fd != nullptr) << "erro when called popen with: " << cmd;
    namespace bio = boost::iostreams;
    bio::stream_buffer<bio::file_descriptor_source> fpstream(fileno(fd), bio::never_close_handle);
    std::istream input_stream(&fpstream);
    CHECK(input_stream.good()) << "open file: " << file_name_with_path << " failed";
    VLOG(2) << "start to read file path" << file_name_with_path;
    read_istream(&input_stream, file_vec);
    VLOG(2) << "finish to read file path" << file_name_with_path;
    if (fd == NULL) {
        VLOG(0) << "find the null point of fd";
    }
    VLOG(2) << "fd address: " << fd;
    return pclose_safe(fd);
}

int32_t HdfsFile::read_block_data(const char* file_name_with_path,
                                  int32_t line_per_block,
                                  std::vector<std::string>& file_block,
                                  UnboundedChannel<std::vector<std::string> >* process_data_channel) {
    CHECK(file_name_with_path) << "input parameter file_name_with_path must not be null";
    std::string cmd(_hdfs_cmd_prefix);
    cmd.append(" -text ").append(file_name_with_path).append(_hdfs_cmd_postfix);
    if (_codec == FileCompressionCodecs::BZIP2_CODEC) {
        cmd.append(" | bzip2 -d ");
    }
    FILE* fd = popen_safe(cmd.c_str(), "r");
    CHECK(fd != nullptr) << "erro when called popen with: " << cmd;
    namespace bio = boost::iostreams;
    bio::stream_buffer<bio::file_descriptor_source> fpstream(fileno(fd), bio::never_close_handle);
    std::istream input_stream(&fpstream);
    CHECK(input_stream.good()) << "open file: " << file_name_with_path << " failed";
    VLOG(2) << "start to read file path" << file_name_with_path;
    read_istream_in_block(&input_stream, line_per_block, file_block, process_data_channel);
    VLOG(2) << "finish to read file path" << file_name_with_path;
    if (fd == NULL) {
        VLOG(0) << "find the null point of fd";
    }
    VLOG(2) << "fd address: " << fd;
    return pclose_safe(fd);
}

int32_t HdfsFile::write_appro_block_data(const char* file_name_with_path,
                                         UnboundedChannel<std::vector<std::string>>* chan,
                                         const int32_t approximate_per_line,
                                         const bool overwrite) {
    CHECK(file_name_with_path) << "input parameter file_name_with_path must not be null";

    std::string cmd = "";
    int exec_ret = 0;

    if (overwrite) {
        cmd.assign(_hdfs_cmd_prefix).append(" -rmr ");
        cmd.append(file_name_with_path).append(_hdfs_cmd_postfix);
        exec_ret = system_exec_no_output(cmd, _retry_num, exec_ret);

        if (exec_ret < 0) {
            return -1;
        }
    }

    cmd = "";
    if (_codec == FileCompressionCodecs::GZIP_CODEC) {
        cmd = "gzip -fc - | ";
    } else if (_codec == FileCompressionCodecs::BZIP2_CODEC) {
        cmd = "bzip2 -fc - | ";
    }
    cmd.append(_hdfs_cmd_prefix);
    cmd.append(" -put - ").append(file_name_with_path);
    cmd.append(_hdfs_cmd_postfix);
    FILE* fd = popen_safe(cmd.c_str(), "w");
    CHECK(nullptr != fd) << "error popen with command: " << cmd;
    namespace bio = boost::iostreams;
    bio::stream_buffer<bio::file_descriptor_sink> fpstream(fileno(fd), bio::never_close_handle);
    std::ostream output_stream(&fpstream);
    CHECK(output_stream.good()) << "open file: " << file_name_with_path << " failed";
    int ret = write_ostream_appro_block(file_name_with_path,
                                     chan,
                                     approximate_per_line,
                                     &output_stream);
    output_stream.flush();
    CHECK_EQ(fflush(fd), 0);
    pclose_safe(fd);
    return ret;
}

int32_t HdfsFile::write_file_in_block(const char* file_name_with_path,
                                      const std::vector<std::string>& data_block,
                                      const bool overwrite) {
    CHECK(file_name_with_path) << "input parameter file_name_with_path must not be null";

    std::string cmd = "";
    int exec_ret = 0;

    if (overwrite) {
        cmd.assign(_hdfs_cmd_prefix).append(" -rmr ");
        cmd.append(file_name_with_path).append(_hdfs_cmd_postfix);
        exec_ret = system_exec_no_output(cmd, _retry_num, exec_ret);

        if (exec_ret < 0) {
            return -1;
        }
    }

    cmd = "";
    if (_codec == FileCompressionCodecs::GZIP_CODEC) {
        cmd = "gzip -fc - | ";
    } else if (_codec == FileCompressionCodecs::BZIP2_CODEC) {
        cmd = "bzip2 -fc - | ";
    }
    cmd.append(_hdfs_cmd_prefix);
    cmd.append(" -put - ").append(file_name_with_path);
    cmd.append(_hdfs_cmd_postfix);
    FILE* fd = popen_safe(cmd.c_str(), "w");
    CHECK(nullptr != fd) << "error popen with command: " << cmd;
    namespace bio = boost::iostreams;
    bio::stream_buffer<bio::file_descriptor_sink> fpstream(fileno(fd), bio::never_close_handle);
    std::ostream output_stream(&fpstream);
    CHECK(output_stream.good()) << "open file: " << file_name_with_path << " failed";
    int ret = write_ostream_in_block(file_name_with_path, data_block, &output_stream);
    output_stream.flush();
    CHECK_EQ(fflush(fd), 0);
    pclose_safe(fd);
    return ret;
}

} // namespace elf
} // namespace baidu
