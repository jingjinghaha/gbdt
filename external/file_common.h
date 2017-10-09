/**
* Copyright (C) 2014-2015 All rights reserved.
* @file file_common.h 
* @brief read from and write into stream 
* @details read and write one by one block, when read a full block will put block into channel 
* @author Weidong Huang (huangweidong01@baidu.com)
* @version 1.0
* @date 2015-10-08 20:31
*/
#ifndef BDL_LUNA_HDFS_FILE_COMMON_H
#define BDL_LUNA_HDFS_FILE_COMMON_H

#include <string>
#include <iostream>
#include <vector>

#include "common/const.h"
#include "common/unbounded_channel.h"

namespace bdl {
namespace luna {
/**
* @brief read data from istream 
* @details line by line to read from istream and put into vector
* @param[in] input_stream open hdfs file get istream that user can read line from it 
* @param[out] file_vec user read line put into file vector
* @return PS_SUCCESS read istream successfully 
*/
int32_t read_istream(std::istream* input_stream,         
                           std::vector<std::string>& file_vec);
/**
* @brief read block from istream 
* @details line by line to read from istream, when one block is full, put the block into channel 
* @param[in] input_stream open hdfs file get istream that user can read line from it 
* @param[in] line_per_block how many lines in one block
* @param[out] file_block user read line put into file vector
* @param[out] process_data_channel when block is full user put file_block into channel 
* @return PS_SUCCESS read istream successfully 
*/
int32_t read_istream_in_block(std::istream* input_stream,         
                           const int32_t line_per_block,
                           std::vector<std::string>& file_block,
                           UnboundedChannel<std::vector<std::string>>* process_data_channel);
/**
* @brief write file block into ostream
* @details write whole file vector into ostream 
* @param[in] file_name_with_path the hdfs path user want to write  
* @param[in] data_block store line data  
* @param[out] output_stream output stream user want to write 
* @return PS_SUCCESS write successfully 
*/
int32_t write_ostream_in_block(const char* file_name_with_path,
                           const std::vector<std::string>& data_block,
                           std::ostream* output_stream);
/**
* @brief write data into hdfs approximate size 
* @param[in] file_name_with_path the hdfs path user want to write  
* @param[in] chan store block data 
* @param[in] approximate_per_line the line user write into hdfs per part, but number not
* accuracy 
* @param[out] output_stream output stream user want to write 
* @return 0 write not enough line data into hdfs 
* @return 1 write enough or more line data into hdfs 
*/
int32_t write_ostream_appro_block(const char* file_name_with_path,
                                  UnboundedChannel<std::vector<std::string>>* chan,
                                  const int32_t approximate_per_line,
                                  std::ostream* output_stream);
enum class FileCompressionCodecs {
    DEFAULT_CODEC,
    GZIP_CODEC,
    BZIP2_CODEC
};

} // namespace luna
} // namespace bdl

#endif // BDL_LUNA_HDFS_FILE_COMMON_H
