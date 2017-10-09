/**
* Copyright (C) 2014-2015 All rights reserved.
* @file load_hdfs_data.cpp 
* @brief load hdfs data  
* @details multithread processing the hdfs data, work node uniformity read hdfs part 
* @author Weidong Huang (huangweidong01@baidu.com)
* @version 1.0
* @date 2015-10-08 20:50
*/
#include "load_hdfs_data.h"

#define DEFAULT_CODEC FileCompressionCodecs::DEFAULT_CODEC;
namespace bdl {
namespace luna {

void LoadHdfsData::filter_empty_file(std::deque<FileInfo>* p_file_deque) {
    std::deque<FileInfo> filter_file_deque;
    for (auto& file : *p_file_deque) {
        if (file.file_size > 0) {
            filter_file_deque.push_back(file);
        }
    }
    *p_file_deque = filter_file_deque;
    return;
}

/**
 * @breif init hdfs file and based on client id to get the hdfs file list
 * @param input_dir is the hdfs input dir
 * @param clientId is the node id just for read hdfs data, not include server node, id begin at 0
 * @param client_num is the total num that read hdfs data
 * @return PS_SUCCESS
 * @return PS_INIT_FAILED 
 */
int32_t LoadHdfsData::init_load_data(const std::string& input_dir,
                                     const int32_t clientId,
                                     const int32_t client_num,
                                     const int32_t read_thread_num,
                                     const int32_t block_per_line,
                                     const std::string& fs_name,
                                     const std::string& fs_ugi,
                                     const std::string& log_file,
                                     int32_t retry_num) {
    VLOG(2) << "Start to init load data";
    _client_id = clientId;
    VLOG(2) << "init load data, client id:" << _client_id;
    //Configure& conf = Configure::singleton();
    _read_thread_num = read_thread_num;
    _block_per_line = block_per_line;
    //conf.get_int_by_name("block_per_line", _block_per_line);

    // init hdfs file
    _hdfs_file = std::make_shared<HdfsFile>(_block_per_line, fs_name, fs_ugi, log_file, retry_num);

    //process_channel
    //_process_data_channel = std::make_shared<UnboundedChannel<std::vector<std::string> > >();

    std::deque<FileInfo> file_list;
    CHECK(_hdfs_file->list_directory(input_dir.c_str(), file_list) >= 0)
            << "failed to list hdfs directory" << input_dir;

    filter_empty_file(&file_list);
    int32_t hdfs_num = file_list.size();
    CHECK(hdfs_num > 0) << "hdfs directory " << input_dir
                        << "is empty";

    int32_t node_per_size = hdfs_num / (client_num);
    int32_t file_size_leave = hdfs_num - node_per_size * client_num;

    int32_t begin_index = 0;
    int32_t end_index = 0;

    if (_is_distribute) {
        if (clientId >= file_size_leave) {
            begin_index = clientId * node_per_size + file_size_leave ;
            end_index = (clientId + 1) * node_per_size + file_size_leave;
        } else {
            begin_index = clientId * node_per_size + clientId;
            end_index = (clientId + 1) * (node_per_size + 1);
        }
    } else {
        end_index = hdfs_num;
    }

    if (begin_index > end_index) {
        VLOG(0) << "Failed to assign node_file_list";
        return PS_INIT_FAILED;
    }
    //should to check if is right
    VLOG(2) << "client_id:" << _client_id << "start to assign:"
            << "file_list size:" << file_list.size() 
            << "begin_index:" << begin_index << "end_index:" << end_index;
    _node_file_list.assign(file_list.begin() + begin_index, file_list.begin() + end_index);

    return PS_SUCCESS;
}

void LoadHdfsData::thread_read_func(const int32_t thread_id) {
    _thread_wait_group.add(1);
    std::vector<std::string> file_block;
    VLOG(2) << "thread_id:" << thread_id << "start to read func";

    while (_start_flag) {
        FileInfo file_info;

        file_block.reserve(_block_per_line);

        VLOG(2) << "thread_id:" << thread_id << "start to get unique lock";
        
        std::unique_lock<std::mutex> deque_lck(_deque_mtx);
        VLOG(2) << "thread_id:" << thread_id << "have get unique lock";
        if (_read_index >= _node_file_list.size()) {
            deque_lck.unlock();
            break;
        }
        
        file_info = _node_file_list.at(_read_index);
        ++_read_index;
        deque_lck.unlock();

        _hdfs_file->read_block_data(file_info.file_name_with_path.c_str(), _block_per_line,
                                    file_block, _process_data_channel);
        VLOG(0) << "[Rank " << _client_id << "] "
                << "Finish reading hdfs path:" << file_info.file_name_with_path;
        
        VLOG(2) << "thread_id:" << thread_id << "finish to read block data";
    }

    if (file_block.size() != 0) {
        _process_data_channel->write(std::move(file_block));
    }

    VLOG(0) << "thread_id:" << thread_id << "finish this thread:" << _thread_wait_group.get_count();
    _thread_wait_group.done();
    VLOG(0) << "thread_id:" << thread_id << "start to wait this thread:" << _thread_wait_group.get_count();
    _thread_wait_group.wait();
    VLOG(0) << "thread_id:" << thread_id << "finish waiting this thread:" << _thread_wait_group.get_count();
    if (0 == thread_id) {
        finish_computing();
    }

    return;
}

int32_t LoadHdfsData::start_load_data(const int32_t block_per_line) {
    _read_index = 0;
    _block_per_line = block_per_line;
    _start_flag = 1;
    
    for (int32_t i = 0; i < _read_thread_num; ++i) {
        auto t = std::thread(&LoadHdfsData::thread_read_func, this, i);
        t.detach();
    }

    return PS_SUCCESS;
}

}//luna
}//bdl
