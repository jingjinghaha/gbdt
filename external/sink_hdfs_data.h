/**
* Copyright (C) 2014-2015 All rights reserved.
* @file sink_hdfs_data.h 
* @brief Used to load data into channel and write into hdfs
* @author Weidong Huang (huangweidong01@baidu.com)
* @version 1.0
* @date 2015-10-08 20:50
*/
#ifndef BDL_LUNA_HDFS_SINK_HDFS_DATA_H
#define BDL_LUNA_HDFS_SINK_HDFS_DATA_H

#include <boost/format.hpp>
#include <thread>
#include "common/wait_group.h"
#include "hdfs_file.h"

namespace bdl {
namespace luna {
/**
* @class SinkHdfsData load_hdfs_data.cpp "hdfs/load_hdfs_data.cpp"
* @brief Used to read from and write into hdfs 
* @details user can set the block num that user want to read one block, and will
* unblock to read block data , put into the channel that you want to read
*/
class SinkHdfsData {
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
    SinkHdfsData(UnboundedChannel<std::vector<std::string> >* process_data_channel
                , int32_t is_overwrite)
                : _process_data_channel(process_data_channel),
                _is_overwrite(is_overwrite) {

    }
    
    /*
     * @brief load hdfs data 
     * @details every node read hdfs part together 
     * @param[in] process_data_channel  put the hdfs content into channel, and there will be a
     * process waiting to read the content of channel
     * */
    SinkHdfsData(UnboundedChannel<std::vector<std::string> >* process_data_channel)
                : _process_data_channel(process_data_channel),
                _is_overwrite(1) {

    }

    ~SinkHdfsData() {
    }
   
    /**
     * @brief init hdfs file and based on client id to get the hdfs file list
     * @param[in] input_dir is the hdfs input dir
     * @param[in] clientId is the node id just for read hdfs data, not include server node, id begin at
     * 0
     * @param[in] client_num is the total num that read hdfs data
     * @return PS_SUCCESS  
     * @return PS_INIT_FAILED 
     * */
    int32_t init_sink_data(const std::string& input_dir,
                           const int32_t client_id, 
                           const int32_t client_num,
                           const int32_t write_thread_num,
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
    int32_t start_sink_data(const int32_t block_per_line);
    
    /**
     * @brief read hdfs data, put it into channel
     * @return PS_SUCCESS load data thread start successfully
     * */
    int32_t start_sink_data();
    
    /**
    * @brief hdfs sink is finish 
    * @return 0 not finish 
    * @return 1 finish sink data
    * @note 
    */
    int32_t is_finish();
    /**
    * @brief hdfs sink is finish 
    * @return 0 not finish 
    * @return 1 finish sink data
    * @note 
    */
    void set_finish();
/**@}*/

private:
    /**
     * @brief finish writing hdfs data
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
        _finish_flag = 1;
        
        VLOG(0) << "finish computing load hdfs";
    }

    /**
     * @brief read data thread
     * @details read data multithread, every node have the hdfs part list waiting to read, each
     * thread will compete to read the hdfs part
     * */
    void thread_sink_func(const int32_t thread_id);

    int32_t _write_thread_num;
    int32_t _approximate_per_line;
    int32_t _start_flag;
    int32_t _client_id;
    int32_t _client_num;
    int32_t _local_part_id;
    
    std::shared_ptr<HdfsFile> _hdfs_file;
    //std::shared_ptr<UnboundedChannel<std::vector<std::string>> > _process_data_channel;
    UnboundedChannel<std::vector<std::string>>*  _process_data_channel;

    int32_t _finish_flag = 0;
    int32_t _is_overwrite;
    const int32_t HDFS_PER_LINE = 100000;
    std::string _write_hdfs_dir;
    
    WaitGroup _thread_wait_group;
    std::mutex _deque_mtx;
};

}//bdl
}//luna

#endif /* BDL_LUNA_HDFS_LOAD_HDFS_DATA_H_ */
