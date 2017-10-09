#ifndef IDL_DFGBDT_DATA_H
#define IDL_DFGBDT_DATA_H

#include "dfgbdt_util.h"

namespace idl {
namespace dfgbdt {

/** 在slave上保存经过离散化后的训练数据，特征并行，单例模式 **/
class TrainDataSet
{
public:
    std::vector<u_map<int32_t, keyid_t> > _signed_data;  // 离散化后的特征

    std::vector<datafloat_t> _tags;  // 每条样本的标签

    std::vector<datafloat_t> _probs;
    std::vector<std::vector<index_t> > _orders;  // 数据在各个特征上的排序，TODO：很可能根本就用不到？
    
    std::vector<std::string> _feat_names;  // 各个特征的名称，暂时没有使用

    index_t size() 
    {
        return _signed_data.size();
    }

    static TrainDataSet* get_data_ptr() 
    {
        if (nullptr == _s_p_instance) {
            _s_p_instance = new TrainDataSet();
        }
        return _s_p_instance;
    }
    
    class Weights
    {
    public:
        void resize(const size_t& sz) 
        {
            _w.resize(sz);
            return;
        }

        void reserve(const size_t& sz) 
        {
            _w.reserve(sz);
            return;
        }

        void push_back(const myfloat_t& w) 
        {
            _w.push_back(w);
            return;
        }

        bool empty() 
        {
            return _w.empty();
        }

        myfloat_t operator [](const size_t idx) 
        {
            return _w.empty() ? 1.0 : _w[idx];
        }

    private:
        std::vector<datafloat_t> _w;
    };

    Weights _weights;  // 样本的权重，如果文件中未定义，则为空

    void save_signed_data_to_file(const std::string& file_name);

private:
    static TrainDataSet* _s_p_instance;
    TrainDataSet() {}
};

/** 在slave上保存原始的测试数据，数据并行，单例模式 **/
class TestDataSet
{
public:
    std::vector<u_map<int32_t, datafloat_t> > _raw_data;  // 测试数据为数据并行，因此每一维特征都有

    std::vector<datafloat_t> _tags;

    std::vector<myfloat_t> _predictions;  // 模型对样本的预测值

    index_t _begin_idx;  // 第一条数据的index

    index_t _end_idx;  // 最后一条数据的index

    index_t size()
    {
        return _raw_data.size();
    }
    
    static TestDataSet* get_data_ptr() 
    {
        if (nullptr == _s_p_instance) {
            _s_p_instance = new TestDataSet();
        }
        return _s_p_instance;
    }

private:
    static TestDataSet* _s_p_instance;

    TestDataSet() {}
};

}
}

#endif
