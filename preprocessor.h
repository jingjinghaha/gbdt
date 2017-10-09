#ifndef IDL_DFGBDT_PREPROCESSOR_H
#define IDL_DFGBDT_PREPROCESSOR_H
    
#include "data.h"
#include "hdfs_io.h"

namespace idl {
namespace dfgbdt {

class DataElement
{
public:
    myfloat_t _x;  // 某个样本在某个特征维度上的原始值

    myfloat_t _y;  // 某个样本的标签

    myfloat_t _w;  // 某个样本的权重
    
    bool operator <(const DataElement& b) const 
    {
        return _x < b._x;
    }
    
    DataElement(myfloat_t x, myfloat_t y, myfloat_t w) : _x(x), _y(y), _w(w) {}
};

class Bucket
{
public:
    index_t _begin_idx;

    index_t _end_idx;  // 该buckt包含了index为[_begin_idx, _end_idx]的区域

    index_t _cut_idx;  // index为_cut的样本被分到左边

    myfloat_t _gain;  // 在_cut上进行划分的增益
    
    bool operator <(const Bucket& b) const 
    {
        return _gain < b._gain;
    }
    
    Bucket(const std::vector<DataElement>& elems, const std::vector<double>& y_sum,
           const std::vector<double>& w_sum, index_t begin, index_t end);
};

class DiscretizationModel
{
public:
    // 离散化切分点
    std::vector<std::vector<myfloat_t> > _cut_points;
    // 特征的local_id和global_id之间的映射关系
    u_map<int32_t, int32_t> _feat_id_global_to_local;

    u_map<int32_t, int32_t> _feat_id_local_to_global;

    inline keyid_t sign_feat_key(int32_t feat_id, myfloat_t x);

    myfloat_t trans_cut_keyid_to_float(int32_t feat_id, keyid_t cut_keyid) const;

    void save_model_to_file(const std::string& file_name);

    void clear() 
    {
        _cut_points.clear();
        _feat_id_global_to_local.clear();
        _feat_id_local_to_global.clear();
        return;
    }
    
    static DiscretizationModel* get_disc_model_ptr() 
    {
        if (nullptr == _s_p_instance) {
            _s_p_instance = new DiscretizationModel();
        }
        return _s_p_instance;
    }

private:
    static DiscretizationModel* _s_p_instance;

    DiscretizationModel() {}
};

/** 读取数据，并对数据进行离散化 ，存入TrainDataSet中 **/
class Preprocessor
{
public:
    std::vector<std::map<int32_t, datafloat_t> > _raw_data;  // 原始数据
    // 读取特征信息，目前也就是特征的名称
    void load_feat_info();
    // 读取训练数据
    void load_raw_train_data(int32_t global_feat_id_start, int32_t global_feat_id_end);
    // 计算离散化的切分点，生成disc_model。内部调用get_feat_elements()和find_feat_cut_points()
    void train_disc_model();

    void write_disc_model_to_file(const std::string& file_name);

    void load_disc_model();
    // 用计算好的离散化模型（disc_model）对数据进行离散化，离散化后的结果存入_train_data_set->_signed_data
    void apply_disc_model();

    void load_raw_test_data();

    void clear_raw_train_data();

    Preprocessor() 
    {
        _p_train_data = TrainDataSet::get_data_ptr();
        _p_test_data = TestDataSet::get_data_ptr();
        // disc_model可以训练产生，也可以从文件中load
        _p_disc_model = nullptr;
    }

    ~Preprocessor() 
    {
        delete _p_disc_model;
    }

private:
    TrainDataSet* _p_train_data;

    TestDataSet* _p_test_data;

    DiscretizationModel* _p_disc_model;

    void parse_train_line(const std::string& line, int32_t begin_id, int32_t end_id,
                          datafloat_t& line_tag, std::map<int32_t, datafloat_t>& line_feat_value_map);

    void parse_test_line(const std::string& line, datafloat_t& line_tag,
                         u_map<int32_t, datafloat_t>& line_feat_value_map);

    inline void parse_feat_id_value(const std::string& id_value, int32_t& feat_id, datafloat_t& feat_value);
    // 从_raw_data中抽取出feat_id这一特征上的elems
    void get_feat_elements(int32_t global_feat_id, myfloat_t total_y_sum, 
                           myfloat_t total_w_sum, index_t feat_occurrence_num,
                           std::vector<DataElement>& elems);
    // 计算特征feat_id上的切分点，并返回离散化的增益
    myfloat_t find_feat_cut_points(std::vector<DataElement>& elems, std::vector<myfloat_t>& cuts);
};

inline void Preprocessor::parse_feat_id_value(const std::string& id_value, 
                                              int32_t& feat_id, datafloat_t& feat_value)
{
    int32_t sepa_idx = 0;
    int32_t str_len = id_value.size();
    while (sepa_idx < str_len && id_value[sepa_idx] != ':') {
        ++sepa_idx;
    }
    CHECK(sepa_idx < str_len) 
        << "Can not parse feature id:value struct: " << id_value;
    feat_id = atoi((id_value.substr(0, sepa_idx)).c_str());
    feat_value = atof((id_value.substr(sepa_idx + 1)).c_str());
    return;
}
    
} // namespace dfgbdt
} // namespace idl

#endif
