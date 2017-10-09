#include <limits>
#include <boost/algorithm/string.hpp>
#include "preprocessor.h"

namespace idl {
namespace dfgbdt {

DiscretizationModel* DiscretizationModel::_s_p_instance = nullptr;

Bucket::Bucket(const std::vector<DataElement>& elems, const std::vector<double>& y_sum,
               const std::vector<double>& w_sum, index_t begin, index_t end)
{
    _begin_idx = begin;
    _end_idx = end;
    _cut_idx = end;
    _gain = 0.0;
    for (index_t cut = _begin_idx; cut < _end_idx; ++cut) {
        if (cut < _end_idx - 1 && elems[cut]._x >= elems[cut + 1]._x) {
            // 不在同样的x之间进行划分
            continue;
        }
        myfloat_t left_y_sum = y_sum[cut + 1] - y_sum[begin];
        myfloat_t left_w_sum = w_sum[cut + 1] - w_sum[begin];

        myfloat_t right_y_sum = y_sum[end + 1] - y_sum[cut + 1];
        myfloat_t right_w_sum = w_sum[end + 1] - w_sum[cut + 1];

        if (left_w_sum + Params::_epsilon < Params::_min_bucket_weight ||
            right_w_sum + Params::_epsilon < Params::_min_bucket_weight) {
            // 保证划分后两边都有足够的权重
            continue;
        }
        myfloat_t left_prediction = left_y_sum / left_w_sum;
        myfloat_t right_prediction = right_y_sum / right_w_sum;
        myfloat_t total_prediction = (left_y_sum + right_y_sum) / (left_w_sum + right_w_sum);

        myfloat_t left_diff = left_prediction - total_prediction;
        myfloat_t right_diff = right_prediction - total_prediction;

        myfloat_t cur_gain = (left_diff * left_diff * (left_w_sum - Params::_disc_lam_l2) + 
                           right_diff * right_diff * (right_w_sum - Params::_disc_lam_l2));

        if (cur_gain > _gain) {
            _gain = cur_gain;
            _cut_idx = cut;
        }
    }
}

keyid_t DiscretizationModel::sign_feat_key(int32_t feat_id, myfloat_t x)
{
    const std::vector<myfloat_t>& cuts = _cut_points[feat_id];
    // 二分搜索
    keyid_t start = 0;
    keyid_t end = cuts.size() - 1;
    if (end < 0 || x > cuts[end]) {
        return end + 1;
    }
    while (end > start) {
        keyid_t mid = start + ((end - start) >> 1);
        if (x <= cuts[mid]) {
            end = mid;
        } else {
            start = mid + 1;
        }
    }
    return start;
}

myfloat_t DiscretizationModel::trans_cut_keyid_to_float(int32_t feat_id, keyid_t cut_keyid) const
{
    return _cut_points[feat_id][cut_keyid];
}

void Preprocessor::load_feat_info()
{
    if (Params::_feat_info_file.empty()) {
        return;
    }
    HdfsReader feat_info_reader(Params::_feat_info_file);
    feat_info_reader.begin_read();
    while (true) {
        std::vector<std::string> block_data;
        if (feat_info_reader.get_block_data(block_data)) {
            for (const std::string& line : block_data) {
                _p_train_data->_feat_names.push_back(line);
            }
        } else {
            break;
        }
    }
    return;
}

void DiscretizationModel::save_model_to_file(const std::string& file_name)
{
    std::vector<std::string> output_vec;
    for (int32_t feat_id = 0; feat_id < (int32_t)_cut_points.size(); ++feat_id) {
        std::ostringstream oss;
        oss << feat_id << " " << _feat_id_local_to_global.at(feat_id);
        std::string cut_vec = Util::trans_vector_to_string(_cut_points[feat_id], ' ');
        if (!cut_vec.empty()) {
            oss << " " << cut_vec;
        }
        output_vec.push_back(oss.str());
    }
    
    HdfsWriter disc_model_writer(file_name);
    disc_model_writer.begin_write();
    disc_model_writer.write_block_data(output_vec);
    disc_model_writer.finish_write();
    return;
}

void Preprocessor::load_raw_train_data(int32_t global_feat_id_start, 
                                       int32_t global_feat_id_end)
{
    LOG(INFO) << "Begin loading raw train data...";
    _raw_data.reserve(100000);
    HdfsReader raw_data_reader(Params::_train_data_file);
    raw_data_reader.begin_read();
    while (true) {
        std::vector<std::string> block_data;
        if (raw_data_reader.get_block_data(block_data)) {
            std::vector<std::map<int32_t, datafloat_t> > block_feat_value_map(block_data.size());
            std::vector<datafloat_t> block_tags(block_data.size());
            std::vector<datafloat_t> block_probs(block_data.size());
#pragma omp parallel for num_threads(Params::_thread_num)
            for (int32_t i = 0; i < (int32_t)block_data.size(); ++i) {
                parse_train_line(block_data[i], global_feat_id_start, global_feat_id_end,
                                 block_tags[i], block_feat_value_map[i]);
            }
            _p_train_data->_tags.insert(_p_train_data->_tags.end(), block_tags.begin(), block_tags.end());
            _p_train_data->_probs.insert(_p_train_data->_probs.end(), block_probs.begin(), block_probs.end());
            _raw_data.insert(_raw_data.end(), block_feat_value_map.begin(), block_feat_value_map.end());
        } else {
            break;
        }
    }
    _raw_data.shrink_to_fit();
    LOG(INFO) << "Finish loading raw train data. Train data size: " << _raw_data.size();
    return;
}

// 此时文件内容已经全部读入，并存储于_raw_data和_train_data_set->_tags和_data_set->_weights中
void Preprocessor::train_disc_model()
{
    LOG(INFO) << "Enter Preprocessor::train_disc_model()";
    if (_raw_data.empty() || nullptr == _p_train_data || _p_train_data->_tags.empty()) {
        LOG(FATAL) << "FATAL ERROR in Preprocessor::train_disc_model(): "
                   << "train data missing.";
    }
    int32_t feat_id = 0;
	std::map<int32_t, index_t> global_feat_id_counter;
    // 扫描每一个样本，统计各个特征出现的次数，以及所有样本y w的和
    LOG(INFO) << "Begin scanning records and counting feature...";
    myfloat_t total_y_sum = 0.0;
    myfloat_t total_w_sum = 0.0;
    for (index_t idx = 0; idx < (index_t)_raw_data.size(); ++idx) {
        myfloat_t weight = _p_train_data->_weights[idx];
        total_y_sum += _p_train_data->_tags[idx] * weight;
        total_w_sum += weight;
        for (const auto& p : _raw_data[idx]) {
            ++global_feat_id_counter[p.first];
        }
    }
    LOG(INFO) << "Finish Scanning records.";
    // 剔除出现次数过少的特征
    std::vector<int32_t> to_remove;
    for (const auto& p : global_feat_id_counter) {
        if (p.second < Params::_min_feat_occur_threshold) {
            to_remove.push_back(p.first);
        }
    }
    //std::string to_remove_str = Util::trans_vector_to_string(to_remove, ',');
    //LOG(INFO) << "Feature with global id: " << to_remove_str << " will be removed.";
    for (const int32_t remove_id : to_remove) {
        global_feat_id_counter.erase(remove_id);
    }
    // 给剩下的global_feat_id赋予local_feat_id
    u_map<int32_t, int32_t> local_to_global_mapper;
    u_map<int32_t, int32_t> global_to_local_mapper;
    feat_id = 0;
    for (const auto& p : global_feat_id_counter) {
        local_to_global_mapper[feat_id] = p.first;
        global_to_local_mapper[p.first] = feat_id;
        ++feat_id;
    }
    int32_t feat_num = (int32_t)global_feat_id_counter.size();
    LOG(INFO) << "Begin calculating cut points...";
    // first:离散化增益  second:feat_id
    std::vector<std::pair<myfloat_t, int32_t> > feat_disc_gains(feat_num, std::pair<myfloat_t, int32_t>()); 
    std::vector<std::vector<myfloat_t> > feat_cuts(feat_num, std::vector<myfloat_t>());
#pragma omp parallel for num_threads(Params::_thread_num)
    for (int32_t feat_id = 0; feat_id < feat_num; ++feat_id) {
        int32_t global_feat_id = local_to_global_mapper.at(feat_id);
        index_t feat_occurrence_num = global_feat_id_counter.at(global_feat_id);
        std::vector<DataElement> feat_elems;
        get_feat_elements(global_feat_id, total_y_sum, total_w_sum,
                          feat_occurrence_num, feat_elems);
        myfloat_t disc_gain = find_feat_cut_points(feat_elems, feat_cuts[feat_id]);
        feat_disc_gains[feat_id].first = disc_gain;
        feat_disc_gains[feat_id].second = feat_id;
    }
    LOG(INFO) << "Finish calculating cut points.";
    // 如果特征数量大于最大特征数量阈值的话，只保留离散化增益最大的那几个特征
    LOG(INFO) << "Begin removing features with low discretization gain...";
    if (feat_num - 1 > Params::_max_feat_num) {
        LOG(INFO) << "feat_num - 1 == " << feat_num - 1 << ", is greater than Params::_max_feat_num";
        std::sort(feat_disc_gains.begin(), feat_disc_gains.end(), 
                  [](const std::pair<myfloat_t, int32_t>& a, const std::pair<myfloat_t, int32_t>& b) {
                        return a.first > b.first;
                  });
    }
    int32_t final_feat_num = std::min(feat_num - 1, Params::_max_feat_num);
    while (final_feat_num > 0) {
        if (feat_disc_gains[final_feat_num - 1].first > 0.0) {
            break;
        }
        --final_feat_num;
    }
    feat_disc_gains.resize(final_feat_num);
    LOG(INFO) << "Finish removing features with low discretization gain.";
    // 将结果保存到DiscretizationModel中
    if (nullptr == _p_disc_model) {
        _p_disc_model = DiscretizationModel::get_disc_model_ptr();
        _p_disc_model->clear();
    }
    _p_disc_model->_cut_points.resize(final_feat_num);
    // 生成的final_feat_id一定是从0开始连续增长的
    for (int32_t final_feat_id = 0; final_feat_id < final_feat_num; ++final_feat_id) {
        int32_t src_feat_id = feat_disc_gains[final_feat_id].second;
        _p_disc_model->_cut_points[final_feat_id].reserve(feat_cuts[src_feat_id].size());
        for (const myfloat_t& cut : feat_cuts[src_feat_id]) {
            _p_disc_model->_cut_points[final_feat_id].push_back(cut);
        }
        int32_t global_feat_id = local_to_global_mapper.at(src_feat_id);
        _p_disc_model->_feat_id_global_to_local[global_feat_id] = final_feat_id;
        _p_disc_model->_feat_id_local_to_global[final_feat_id] = global_feat_id;
    }
    return;
}

void Preprocessor::write_disc_model_to_file(const std::string& file_name)
{
    CHECK(_p_disc_model) << "FATAL ERROR in Preprocessor::save_disc_model_to_file(): "
                         << "_p_disc_model is nullptr!";
    _p_disc_model->save_model_to_file(file_name);
    return;
}

void Preprocessor::apply_disc_model()
{
    LOG(INFO) << "Begin applying disc model on train data...";
    CHECK(_p_disc_model && !_p_disc_model->_cut_points.empty() && 
          !_p_disc_model->_feat_id_global_to_local.empty() &&
          !_p_disc_model->_feat_id_local_to_global.empty())
             << "FATAL ERROR occurs in Preprocessor::sign_data(): "
             << "Error deteted in discretization model!";
    // 生成_p_train_data->_signed_data
    index_t rcd_num = _raw_data.size();
    std::vector<u_map<int32_t, keyid_t> >& signed_data = _p_train_data->_signed_data;
    signed_data.resize(rcd_num);
#pragma omp parallel for num_threads(Params::_thread_num)
    for (index_t idx = 0; idx < rcd_num; ++idx) {
        signed_data[idx].reserve(_raw_data[idx].size());
        for (const auto& p : _raw_data[idx]) {
            int32_t global_feat_id = p.first;
            if (!_p_disc_model->_feat_id_global_to_local.count(global_feat_id)) {
                continue;
            }
            myfloat_t x = p.second;
            int32_t feat_id = _p_disc_model->_feat_id_global_to_local.at(global_feat_id);
            keyid_t key_id = _p_disc_model->sign_feat_key(feat_id, x);
            if (key_id > 0) {
                signed_data[idx][feat_id] = key_id;
            }
        }
    }
    LOG(INFO) << "Finish disc train data.";
    return;
}

void Preprocessor::load_raw_test_data()
{
    LOG(INFO) << "Begin loading raw test data...";
    HdfsReader test_data_reader(Params::_test_data_file);
    test_data_reader.begin_read();
    while (true) {
        std::vector<std::string> block_data;
        if (test_data_reader.get_block_data(block_data)) {
            std::vector<u_map<int32_t, datafloat_t> > block_feat_value_map(block_data.size());
            std::vector<datafloat_t> block_tags(block_data.size());
#pragma omp parallel for num_threads(Params::_thread_num)
            for (int32_t i = 0; i < (int32_t)block_data.size(); ++i) {
                parse_test_line(block_data[i], block_tags[i], block_feat_value_map[i]);
            }
            _p_test_data->_raw_data.insert(_p_test_data->_raw_data.end(), 
                                           block_feat_value_map.begin(),
                                           block_feat_value_map.end());
            _p_test_data->_tags.insert(_p_test_data->_tags.end(), block_tags.begin(),
                                       block_tags.end());
        } else {
            break;
        }
    }
    _p_test_data->_begin_idx = 0;
    _p_test_data->_end_idx = _p_test_data->size() - 1;
    LOG(INFO) << "Finish loading raw test data";
    return;
}

void Preprocessor::clear_raw_train_data()
{
    _raw_data.clear();
    std::vector<std::map<int32_t, datafloat_t> >(_raw_data).swap(_raw_data);
    //_raw_data.shrink_to_fit();
    return;
}

void Preprocessor::parse_train_line(const std::string& line, int32_t  begin_id, int32_t end_id,
                                    datafloat_t& line_tag, std::map<int32_t, datafloat_t>& line_feat_value_map)
{
    std::vector<std::string> splited_line;
    boost::split(splited_line, line, boost::is_any_of(" "));
    if (splited_line.back().empty()) {
        splited_line.pop_back();
    }
    CHECK(splited_line[0] == "0" || splited_line[0] == "1") 
          << "FATAL ERROR occurs in Preprocessor::parse_train_line(): "
          << "Can not find record label！";
    // 数据第一列为tag
    line_tag = ((datafloat_t)atof(splited_line[0].c_str()));
    // 其余列为特征数据
    line_feat_value_map.clear();
    for (int32_t i = 1; i < (int32_t)splited_line.size(); ++i) {
        int32_t feat_id;
        datafloat_t feat_value;
        parse_feat_id_value(splited_line[i], feat_id, feat_value);
        CHECK(feat_id <= std::numeric_limits<int32_t>::max() && 
              feat_id >= std::numeric_limits<int32_t>::min()) << "Feature id is out of range!";
        if (feat_id >= begin_id && feat_id <= end_id) {
            feat_value = std::min(std::numeric_limits<datafloat_t>::max(), feat_value);
            feat_value = std::max(std::numeric_limits<datafloat_t>::min(), feat_value);
            line_feat_value_map[feat_id] = feat_value;
        }
    }
    return;
}

void Preprocessor::parse_test_line(const std::string& line, datafloat_t& line_tag,
                                   u_map<int32_t, datafloat_t>& line_feat_value_map)
{
    std::vector<std::string> splited_line;
    boost::split(splited_line, line, boost::is_any_of(" "));
    if (splited_line.back().empty()) {
        splited_line.pop_back();
    }
    CHECK(splited_line[0] == "0" || splited_line[0] == "1")
          << "FATAL ERROR occurs in Preprocessor::parse_test_line(): "
          << "Label must be '1' or '0'！";
    // 数据第一列为tag
    line_tag = (datafloat_t)atof(splited_line[0].c_str());
    // 其余列为特征数据
    line_feat_value_map.clear();
    for (int32_t i = 1; i < (int32_t)splited_line.size(); ++i) {
        int32_t feat_id;
        datafloat_t feat_value;
        parse_feat_id_value(splited_line[i], feat_id, feat_value);
        line_feat_value_map[feat_id] = feat_value;
    }
    return;
}

void Preprocessor::get_feat_elements(int32_t global_feat_id, myfloat_t total_y_sum, 
                                     myfloat_t total_w_sum, index_t feat_occurrence_num,
                                     std::vector<DataElement>& elems)
{
    elems.reserve(feat_occurrence_num + 1);
    // 为缺失值预留0位置
    elems.push_back(DataElement(std::numeric_limits<datafloat_t>::min(), 0.0, 0.0));

    myfloat_t nomissing_y_sum = 0.0;
    myfloat_t nomissing_w_sum = 0.0;
    for (index_t idx = 0; idx < (index_t)_raw_data.size(); ++idx) {
        const auto& rcd = _raw_data[idx];
        if (rcd.count(global_feat_id)) {
            myfloat_t weight = _p_train_data->_weights[idx];
            myfloat_t y = _p_train_data->_tags[idx] * weight;
            elems.push_back(DataElement(rcd.at(global_feat_id), y, weight));
            nomissing_y_sum += y;
            nomissing_w_sum += weight;
        }
    }
    if (total_w_sum > nomissing_w_sum) {
        elems[0]._y = (total_y_sum - nomissing_y_sum) / (total_w_sum - nomissing_w_sum);
        elems[0]._w = total_w_sum - nomissing_w_sum;
    }
    return;
}

myfloat_t Preprocessor::find_feat_cut_points(std::vector<DataElement>& elems, 
                                             std::vector<myfloat_t>& cuts)
{
    index_t len = elems.size();
    std::sort(elems.begin(), elems.end());

    std::vector<double> y_sum(len + 1, 0.0);
    std::vector<double> w_sum(len + 1, 0.0);

    double cur_y_sum = 0.0;
    double cur_w_sum = 0.0;
    for (index_t i = 0; i < len; ++i) {
        // 此处按照fgbdt原版实现，但实际上_y中的内容已经是tag与weight相乘的结果
        cur_y_sum += elems[i]._y * elems[i]._w;
        cur_w_sum += elems[i]._w;
        y_sum[i + 1] = cur_y_sum;
        w_sum[i + 1] = cur_w_sum;
    }

    std::priority_queue<Bucket> bucket_queue;
    myfloat_t total_gain = 0.0;
    bucket_queue.push(Bucket(elems, y_sum, w_sum, 0, len - 1));
    int32_t bucket_num = 1;
    while (bucket_num < Params::_max_bucket_num && !bucket_queue.empty()) {
        Bucket b = bucket_queue.top();
        bucket_queue.pop();
        if (b._cut_idx >= len - 2 || b._gain <= 0.0) {
            continue;
        }

        cuts.push_back(0.5 * (elems[b._cut_idx]._x + elems[b._cut_idx + 1]._x));

        total_gain += b._gain;
        ++bucket_num;
        
        bucket_queue.push(Bucket(elems, y_sum, w_sum, b._begin_idx, b._cut_idx));
        bucket_queue.push(Bucket(elems, y_sum, w_sum, b._cut_idx + 1, b._end_idx));
    }
    std::sort(cuts.begin(), cuts.end());
    return total_gain;
}

}  // namespace dfgsbt
}  // namespace idl
