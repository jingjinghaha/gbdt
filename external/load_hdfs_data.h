/**
* Copyright (C) 2014-2015 All rights reserved.
* @file load_hdfs_data.h 
* @brief Used to load hdfs data into channel 
* @details according to the hdfs part num, uniformity put the part to each node
* when hdfs part is unchanged and node num is unchanged, the node read the same content
* when thread num became more will cause the not full block num became larger, because every part
* can not divid mini-batch size exactly
* so when the trainning task is sensitive to the not full block num, you should be carefully setting
* the read thread num
* @author Weidong Huang (huangweidong01@baidu.com)
* @version 1.0
* @date 2015-10-08 20:50
*/
#ifndef BDL_LUNA_HDFS_LOAD_HDFS_DATA_H
#define BDL_LUNA_HDFS_LOAD_HDFS_DATA_H

#include <thread>
#include "common/wait_group.h"
#include "hdfs_file.h"

namespace bdl {
namespace luna {
/**
* @class LoadHdfsData load_hdfs_data.cpp "hdfs/load_hdfs_data.cpp"
* @brief Used to read from and write into hdfs 
* @details user can set the block num that user want to read one block, and will
* unblock to read block data , put into the channel that you want to read
*/
class LoadHdfsData {
public:
/**                                                                                                     
* @defgroup Hdfs 
* @brief Used to read from and write into hdfs
* @{                                                    
*/
    
    /*
     * @brief the constructor of loadHdfsData 
     * @details  
     * @param[in] process_data_channel  put the hdfs content into channel, and there will be a
     * process waiting to read the content of channel
     * @param[in] is_distribute 0 means every node will read the whole hdfs part; 1 means oppositly, every node read
     * hdfs part together
     * */
    LoadHdfsData(UnboundedChannel<std::vector<std::string> >* process_data_channel
                , int32_t is_distribute)
                : _process_data_channel(process_data_channel),
                _is_distribute(is_distribute){

    }
    
    /*
     * @brief load hdfs data 
     * @details every node read hdfs part together 
     * @param[in] process_data_channel  put the hdfs content into channel, and there will be a
     * process waiting to read the content of channel
     * */
    LoadHdfsData(UnboundedChannel<std::vector<std::string> >* process_data_channel)
                : _process_data_channel(process_data_channel),
                _is_distribute(1){

    }

    ~LoadHdfsData() {
    }

    void filter_empty_file(std::deque<FileInfo>* p_file_deque);
    /**
     * @brief init hdfs file and based on client id to get the hdfs file list
     * @param[in] input_dir is the hdfs input dir
     * @param[in] clientId is the node id just for read hdfs data, not include server node, id begin at
     * 0
     * @param[in] client_num is the total num that read hdfs data
     * @return PS_SUCCESS  
     * @return PS_INIT_FAILED 
     * */
    int32_t init_load_data(const std::string& input_dir, 
                           const int32_t client_id, 
                           const int32_t client_num,
                           const int32_t read_thread_num,
                           const int32_t block_per_line,
                           const std::string& fs_name,
                           const std::string& fs_ugi,
                           const std::string& log_file,
                           int32_t retry_num);
    
    /**
     * @brief read hdfs data, put it into channel
     * @param[in] block_per_line how many lines that vector have, when channel read a object, means the
     * size of object
     * @return PS_SUCCESS load data thread start successfully
     * */
    int32_t start_load_data(const int32_t block_per_line);
/**@}*/

private:
    /**
     * @brief finish loading hdfs data
     * @details will waiting for the channel empty, then will close the channel and finish loading
     * data 
     * @return PS_SUCCESS load data thread start successfully
     * */
    void finish_computing() {
        std::lock_guard<std::mutex> guard(_deque_mtx);
        VLOG(0) << "wait process channel empty";
        
        while (!_process_data_channel->empty()) {
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
        }
        if (!_process_data_channel->is_closed()) {
            _process_data_channel->close();
        }
        
        //channel must empty or will cause bug
        _start_flag = 0;
        
        VLOG(0) << "finish computing load hdfs";
    }

    /**
     * @brief read data thread
     * @details read data multithread, every node have the hdfs part list waiting to read, each
     * thread will compete to read the hdfs part
     * */
    void thread_read_func(const int32_t thread_id);

    uint32_t _read_index;
    int32_t _read_thread_num;
    int32_t _block_per_line = 10;
    int32_t _start_flag;
    int32_t _client_id = 0;
    
    std::shared_ptr<HdfsFile> _hdfs_file;
    //std::shared_ptr<UnboundedChannel<std::vector<std::string>> > _process_data_channel;
    UnboundedChannel<std::vector<std::string>>*  _process_data_channel;
    
    int32_t _is_distribute;
    int32_t _load_mode;
    
    WaitGroup _thread_wait_group;
    std::deque<FileInfo> _node_file_list;
    std::mutex _deque_mtx;
};

}//bdl
}//luna

#endif /* BDL_LUNA_HDFS_LOAD_HDFS_DATA_H_ */
