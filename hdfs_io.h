#ifndef IDL_DFGBDT_HDFS_IO_H
#define IDL_DFGBDT_HDFS_IO_H

#include "dfgbdt_util.h"
#include "common/struct.h"
#include "external/load_hdfs_data.h"
#include "external/sink_hdfs_data.h"

namespace idl {
namespace dfgbdt {

class HdfsReader
{
public:
    // 启动新的线程来读取数据，将读取的内容以block为单位放入到_data_channel中
    void begin_read();
    // 从_data_channel中取出一个block的数据
    bool get_block_data(std::vector<std::string>& block_data);

    HdfsReader(const std::string& file_name) 
    {
        initialize(file_name);
    }

private:
    bdl::luna::UnboundedChannel<std::vector<std::string> > _data_channel;

    std::shared_ptr<bdl::luna::LoadHdfsData> _p_load_data;

    int32_t initialize(const std::string& file_name);
};

class HdfsWriter
{
public:
    void begin_write();

    void write_block_data(const std::vector<std::string>& block_data);

    void finish_write();

    HdfsWriter(const std::string& file_name)
    {
        initialize(file_name);
    }

private:
    bdl::luna::UnboundedChannel<std::vector<std::string> > _write_channel;

    std::shared_ptr<bdl::luna::SinkHdfsData> _p_sink_data;

    int32_t initialize(const std::string& file_name);
};

}
}

#endif
