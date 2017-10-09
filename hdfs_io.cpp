#include "hdfs_io.h"

namespace idl {
namespace dfgbdt {

int32_t HdfsReader::initialize(const std::string& file_name)
{
    _p_load_data = std::make_shared<bdl::luna::LoadHdfsData>(&_data_channel, 0);
    if (_p_load_data->init_load_data(file_name, 
                                     HdfsParams::_node_id, 
                                     HdfsParams::_node_num,
                                     HdfsParams::_read_thread_num,
                                     HdfsParams::_block_per_line,
                                     HdfsParams::_fs_name,
                                     HdfsParams::_fs_ugi,
                                     HdfsParams::_log_file,
                                     HdfsParams::_retry_num) < 0) {
        LOG(FATAL) << "FATAL ERROR in HdfsReader::initialize(): "
                   << "Fail to initialize _p_load_data";
    }
    return FuncState::_SUCCESS;
}

void HdfsReader::begin_read()
{
    _p_load_data->start_load_data(HdfsParams::_block_per_line);
    return;
}

bool HdfsReader::get_block_data(std::vector<std::string>& block_data)
{
    return _data_channel.read(block_data) ? true : false;
}

int32_t HdfsWriter::initialize(const std::string& file_name)
{
    _p_sink_data = std::make_shared<bdl::luna::SinkHdfsData>(&_write_channel);
    if (_p_sink_data->init_sink_data(file_name, 
                                     HdfsParams::_node_id, 
                                     HdfsParams::_node_num,
                                     HdfsParams::_write_thread_num,
                                     HdfsParams::_fs_name,
                                     HdfsParams::_fs_ugi,
                                     HdfsParams::_log_file,
                                     HdfsParams::_retry_num) < 0) {
        LOG(FATAL) << "FATAL ERROR in HdfsWriter::initialize(): "
                   << "Fail to initialize _p_sink_data";
    }
    return FuncState::_SUCCESS;
}

void HdfsWriter::begin_write()
{
    _p_sink_data->start_sink_data();
    return;
}

void HdfsWriter::write_block_data(const std::vector<std::string>& block_data)
{
    _write_channel.write(std::move(block_data));
    return;
}

void HdfsWriter::finish_write()
{
    while (!_write_channel.empty()) {
        LOG(INFO) << "Wait channel empty...";
        std::this_thread::sleep_for(std::chrono::microseconds(200000));
    }
    if (!_write_channel.is_closed()) {
        _write_channel.close();
    }
    while (!_p_sink_data->is_finish()) {
        LOG(INFO) << "Wait hdfs sink finish...";
        std::this_thread::sleep_for(std::chrono::microseconds(200000));
    }
    return;
}

}  // namespace idl
}  // namespace dfgbdt
