#ifndef IDL_DFGBDT_FOREST_H
#define IDL_DFGBDT_FOREST_H

#include "tree.h"

namespace idl {
namespace dfgbdt {

class Predictor;

class Forest
{
public:
    friend class SlaveForestTrainer;
    // 加入新的训练完的树，同时更新训练样本的预测值
    void add_tree(Tree* p_new_tree, Loss* p_loss);
    // 调整某一棵树某一个节点的预测值，供fully_corrective使用
    void adjust_tree_node_prediction(int32_t tree_id, int32_t node_id, myfloat_t pred_diff);
    
    void set_rcd_prediction(index_t index, myfloat_t new_prediction);

    myfloat_t get_rcd_prediction(index_t index);

    void write_forest_to_file(const std::string& file_name);

    myfloat_t predict_record(const u_map<int32_t, datafloat_t>& record);

    int32_t get_tree_num()
    {
        return _trees.size();
    }
    
    Forest()
    {
        index_t rcd_num = TrainDataSet::get_data_ptr()->size();
        _rcd_predictions.resize(rcd_num);
        for (index_t rcd_idx = 0; rcd_idx < rcd_num; ++rcd_idx) {
            _rcd_predictions[rcd_idx] = 0.0;
        }
    }

    ~Forest()
    {
        for (Tree*& ptr : _trees) {
            delete ptr;
            ptr = nullptr;
        }
        _trees.clear();
    }

private:

    std::vector<myfloat_t> _rcd_predictions;  // 各个训练样本在当前状态下的预测值

    std::vector<Tree*> _trees;  // 各个树
};

class SlaveForestTrainer
{
public:
    // 训练forest，调用SlaveTreeTrainer::train_tree()
    void train_forest();
    
    SlaveForestTrainer(Forest* p_forest);
    
    ~SlaveForestTrainer();

private:
    Predictor* _p_predictor;

    Forest* _p_forest;

    Loss* _p_loss;

    // 根据当前的_rcd_predictions计算残差，并存入_p_loss中的_residues中
    // 在开始训练一棵新的树之前调用
    void calc_residues();
    // 执行fully_corrective，update_forest()调用update_tree()
    void fc_update_forest();

    void update_tree(int32_t tree_id);
};

}
}

#endif
