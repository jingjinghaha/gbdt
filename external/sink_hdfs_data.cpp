/**
* Copyright (C) 2014-2015 All rights reserved.
* @file sink_hdfs_data.cpp 
* @brief sink hdfs data  
* @details multithread processing write hdfs data
* @author Weidong Huang (huangweidong01@baidu.com)
* @version 1.0
* @date 2015-10-08 20:50
*/
#include "sink_hdfs_data.h"

#define DEFAULT_CODEC FileCompressionCodecs::DEFAULT_CODEC;
namespace bdl {
namespace luna {
/**
 * @breif init hdfs file and based on client id to get the hdfs file list
 * @param input_dir is the hdfs input dir
 * @param clientId is the node id just for read hdfs data, not include server node, id begin at 0
 * @param client_num is the total num that read hdfs data
 * @return PS_SUCCESS
 * @return PS_INIT_FAILED 
 */
int32_t SinkHdfsData::init_sink_data(const std::string& input_dir,
                                     const int32_t clientId,
                                     const int32_t client_num,
                                     const int32_t write_thread_num,
                                     const std::string& fs_name,
                                     const std::string& fs_ugi,
                                     const std::string& log_file,
                                     int32_t retry_num) {
                                    
    VLOG(2) << "Start to init load data";
    _client_id = clientId;
    _client_num = client_num;
	_local_part_id = 0;
    VLOG(2) << "init load data, client id:" << _client_id;
    //Configure& conf = Configure::singleton();
    //CHECK(0 == conf.get_int_by_name("write_thread_num", _write_thread_num));
    _write_thread_num = write_thread_num;
  
    VLOG(2) << "Start to init load data";
    _client_id = clientId;
    _client_num = client_num;
    VLOG(2) << "init load data, client id:" << _client_id;
    //Configure& conf = Configure::singleton();
    //CHECK(0 == conf.get_int_by_name("write_thread_num", _write_thread_num));
    _write_thread_num = write_thread_num;
  
    _write_hdfs_dir = input_dir;
    _approximate_per_line = HDFS_PER_LINE;
    // init hdfs file
    _hdfs_file = std::make_shared<HdfsFile>(_approximate_per_line, fs_name, fs_ugi, log_file, retry_num);

    if (_is_overwrite) {
        if (_client_id == 0) {      
            if (1 == _hdfs_file->exist(_write_hdfs_dir.c_str())) {  
                CHECK(1 == _hdfs_file->delete_directory(_write_hdfs_dir.c_str())) 
                    << "delete hdfs directory: " << _write_hdfs_dir << " failed.";
            }                                                                               
            CHECK(0 <= _hdfs_file->make_directory(_write_hdfs_dir.c_str()))       
                << "make hdfs directory: " << _write_hdfs_dir << " failed.";      
        }
    }
    return PS_SUCCESS;
}

void SinkHdfsData::thread_sink_func(const int32_t thread_id) {
    _thread_wait_group.add(1);
    VLOG(2) << "thread_id:" << thread_id << "start to read func";

    while (_start_flag && (!_process_data_channel->is_closed())) {
        std::unique_lock<std::mutex> deque_lck(_deque_mtx);
        int32_t part_id = _local_part_id * _client_num + _client_id; 
        ++_local_part_id;
        deque_lck.unlock();
        
        std::string write_file_name = 
                    boost::str(boost::format("%s/part-%05d") % _write_hdfs_dir % part_id);
        _hdfs_file->write_appro_block_data(write_file_name.c_str(),
                                           _process_data_channel,
                                           _approximate_per_line,
                                           _is_overwrite);
        VLOG(0) << "[Rank " << _client_id << "] "
                << "Finish writing hdfs path:" << write_file_name;
    }

    VLOG(0) << "thread_id:" << thread_id << "finish this thread:" << _thread_wait_group.get_count();
    _thread_wait_group.done();
    VLOG(0) << "thread_id:" << thread_id << "start to wait this thread:" << _thread_wait_group.get_count();
    _thread_wait_group.wait();
    VLOG(0) << "thread_id:" << thread_id << "finish waiting this thread:" << _thread_wait_group.get_count();
    finish_computing();

    return;
}

void SinkHdfsData::set_finish() {
    _start_flag = 0;
}

int32_t SinkHdfsData::is_finish() {
    return _finish_flag;
}

int32_t SinkHdfsData::start_sink_data() {
    _start_flag = 1;
    
    for (int32_t i = 0; i < _write_thread_num; ++i) {
        auto t = std::thread(&SinkHdfsData::thread_sink_func, this, i);
        t.detach();
    }

    return PS_SUCCESS;
}

int32_t SinkHdfsData::start_sink_data(const int32_t hdfs_per_line) {
    _approximate_per_line = hdfs_per_line;
    _start_flag = 1;
    
    start_sink_data();

    return PS_SUCCESS;
}

}//luna
}//bdl
