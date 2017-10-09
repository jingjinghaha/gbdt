/**
* Copyright (C) 2014-2015 All rights reserved.
* @file hdfs_file.h 
* @brief Supply apis about hdfs operation 
* @details hdfs inner_init cmd write_file_in_block read_block_data 
* @author Weidong Huang (huangweidong01@baidu.com)
* @version 1.0
* @date 2015-10-08 20:32
*/
#ifndef BDL_LUNA_HDFS_HDFS_FILE_H
#define BDL_LUNA_HDFS_HDFS_FILE_H

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

#include "file_common.h"
#include "common/system.h"
#include "common/struct.h"
#include "common/const.h"
#include "common/config.h"

namespace bdl {
namespace luna {

/*
 * @class HdfsFile hdfs_file.h "hdfs/hdfs_file.h"
 * @brief HdfsFile is a lower class used to read/write hdfs files. Users also can use this class for
 * convinent.
 */
class HdfsFile {
public:

/**                                                                                                     
* @defgroup Hdfs  
* @brief Used to read from and write into hdfs
* @{ 
*/ 
    /**
     * @brief Constructor of HdfsFile class
     * @param[in] retry_num Retry nums for each hdfs operation
     * @param[in] fs_name hdfs file name with the format host:port
     * @param[in] ugi hdfs ugi with the format user,password
     * @param[in] hdfs_log_file Log file to save hdfs stderr.
     */
    HdfsFile(const int32_t retry_num, const std::string& fs_name, const std::string& ugi,
             const std::string& hdfs_log_file);

    /**
     * @brief Constructor of HdfsFile class, but use the retry_num, fs_name, ugi, hdfs_log_file
     * with the value in user configure file.
     */
    HdfsFile();
    
    /**
    * @brief Constructor of HdfsFile class 
    * @details user can define block's size 
    * @param[in] block_per_line the block size 
    */
    HdfsFile(const int32_t block_per_line,
             const std::string& fs_name,
             const std::string& fs_ugi,
             const std::string& log_file,
             int32_t retry_num);

    /**
    * @brief Constructor of HdfsFile class 
    * @details user can define higher priority conf and block's size 
    * @param[in] conf higher prioriy conf 
    * @param[in] codec hdfs store codec  
    */
    HdfsFile(TwoTierConfigure* conf, FileCompressionCodecs codec);

    ~HdfsFile() = default;

    /**
     * @brief Get list of files/directories for a given path, This is a thread-safe function.
     * @param[in] path The path to list.
     * @param[out] files Vector to save the result files/directories FileInfo.
     * @return 0 on success; -1 on error.
     */
    int32_t list_directory(const char* path, std::deque<FileInfo>& files);
    
    /**
    * @brief get file size 
    * @param[in] path The path to list.
    * @param[out] file_size size of the file
    * @return 0 on success; -1 on error 
    */
    int32_t get_file_size(const char* path, uint64_t file_size);
    
    /**
    * @brief dus directory size 
    * @param[in] path The path to list.
    * @param[out] directory size
    * @return 0 on success; -1 on error 
    */
    int32_t dus_directory(const char* path, uint64_t dir_size);
    
    /**
     * @brief Delete hfds directory, This is a thread-safe function.
     * @param[in] path The path to delete.
     * @return -1 on error; 0 when directory not exit; 1 on success.
     */
    int32_t delete_directory(const char* path);

    /**
     * @brief Make new directory, This is a thread-safe function.
     * @param[in] path The path to make.
     * @return -1 on error; 0 when directory not exit; 1 on success.
     */
    int32_t make_directory(const char* path);

    /**
     * @brief Checks if a given path exsits on the fs. This is a thread-safe function.
     * @param[in] path The path to look for.
     * @return -1 : error; 0 : not exit; 1 : exit.
     */
    int32_t exist(const char* path);

    /**
     * @brief to read the file per line and then when the num reach block num and write into channel
     * @param[in] file_name_with_path is the hdfs part file path
     * @param[in] line_per_block means how many line in one block
     * @param[out] file_block store the hdfs line data
     * @param[out] process_data_channel finish reading file block then will put into channel
     * @return -1 : close failed ; 0 : read successfully
     */
    int32_t read_block_data(const char* file_name_with_path,
                            int32_t line_per_block,
                            std::vector<std::string>& file_block, 
                            UnboundedChannel<std::vector<std::string> >* process_data_channel);
    
    /**
     * @brief to read the hdfs file and put into vector
     * @param[in] file_name_with_path is the hdfs part file path
     * @param[out] vector store the hdfs line data
     * @return -1 : close failed ; 0 : read successfully
     */
    int32_t read_data(const char* file_name_with_path,
                            std::vector<std::string>& file_vec);
    
    /**
    * @brief write data 
    * @details write data_block into hdfs path 
    * @param[in] file_name_with_path the hdfs path user want to write 
    * @param[in] chan to store write data
    * @param[in] approximate_per_line user write a hdfs part's line num, not accuracy
    * @param[in] overwrite if the path already exist, whether overwrite
    * @return -1 write file failed; 0 write successfully 
    */
    int32_t write_appro_block_data(const char* file_name_with_path,
                                   UnboundedChannel<std::vector<std::string>>* chan,
                                   const int32_t approximate_per_line,
                                   const bool overwrite);

    /**
    * @brief write data 
    * @details write data_block into hdfs path 
    * @param[in] file_name_with_path the hdfs path user want to write 
    * @param[in] data_block the data user want to write 
    * @param[in] overwrite if the path already have data whether to overwrite the data 
    * @return -1 write file failed; 0 write successfully 
    */
    int32_t write_file_in_block(const char* file_name_with_path,
                                const std::vector<std::string>& data_block,
                                const bool overwrite);
/**@}*/
protected:
    void inner_init();

    void inner_init(const std::string& fs_name, const std::string& fs_ugi,
                    const std::string& log_file, int32_t retry_num);

protected:
    TwoTierConfigure* _p_conf;
    int32_t _retry_num;
    int32_t _block_per_line;
    std::string _fs_name;
    std::string _ugi;
    std::string _log_file;
    std::string _hdfs_cmd_prefix;
    std::string _hdfs_cmd_postfix;
    FileCompressionCodecs _codec;
};

} // namespace luna
} // namespace bdl

#endif // BDL_LUNA_HDFS_HDFS_FILE_H
