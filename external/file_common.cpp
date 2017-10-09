/**
* Copyright (C) 2014-2015 All rights reserved.
* @file file_common.cpp 
* @brief read and write from stream  
* @details read and write one by one block, when read a full block will put block into channel  
* @author Weidong Huang (huangweidong01@baidu.com)
* @version 1.0
* @date 2015-10-08 19:13
*/
#include <limits>
#include "file_common.h"

namespace bdl {
namespace luna {

const size_t kApproximateBufferSize = LARGE_BUFFER_SIZE;

int32_t read_istream(std::istream* input_stream,
                     std::vector<std::string>& file_vec) {
    std::string line;
    while (getline(*input_stream, line)) {
        file_vec.push_back(line);
    }
    return PS_SUCCESS;
}

int32_t read_istream_in_block(std::istream* input_stream,
                           const int32_t line_per_block,
                           std::vector<std::string>& file_block,
                           UnboundedChannel<std::vector<std::string>>* process_data_channel) {
    CHECK_GT(line_per_block, 0);
    std::string line;
    while (getline(*input_stream, line)) {
        file_block.push_back(line);
        if (file_block.size() >= static_cast<size_t>(line_per_block)) {
            process_data_channel->write(std::move(file_block));
            //file_block.clear();
        }
    }
    return PS_SUCCESS;
}

int32_t write_ostream_in_block(const char* file_name_with_path,
                           const std::vector<std::string>& data_block,
                           std::ostream* output_stream) { 
    for (auto it = data_block.begin(); it != data_block.end(); it++) {
        (*output_stream) << *it << "\n";
        CHECK(output_stream->good()) << "failed to write hdfs file" << file_name_with_path;
    }
    return PS_SUCCESS;
}

int32_t write_ostream_appro_block(const char* file_name_with_path,
                                  UnboundedChannel<std::vector<std::string>>* chan,
                                  const int32_t approximate_per_line,
                                  std::ostream* output_stream) { 
    uint32_t total_size = 0;
    std::vector<std::string> block_data;
    while (total_size < approximate_per_line && chan->read(block_data)) { 
        for (auto & line : block_data) {
            (*output_stream) << line << "\n";
            CHECK(output_stream->good()) << "failed to write hdfs file" << file_name_with_path;
        }
        total_size += block_data.size();
    }
    
    if (total_size >= approximate_per_line) {
        return 1;
    }
    return 0;
}
} // namespace luna
} // namespace bdl
