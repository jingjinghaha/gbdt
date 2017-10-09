#ifndef IDL_DFGBDT_NODE_H
#define IDL_DFGBDT_NODE_H

#include "loss.h"
#include "preprocessor.h"

namespace idl {
namespace dfgbdt {

class TreeNode
{
public:
    std::vector<index_t> _node_sample_index;
    std::vector<index_t> _node_raw_index;  // 属于当前节点的index，尽在叶节点上有效，在非叶节点上为空

    int32_t _node_id;  // 叶节点id，和下标一致

    int32_t _global_feat_id;  // 本节点上进行分割的global_feat_id，叶节点为-1

    myfloat_t _cut;  // 本节点上的分割点，叶节点为-1

    int32_t _local_feat_id;

    keyid_t _cut_keyid;

    int32_t _parent_id;  // 父节点id，根节点为-1

    int32_t _left_child_id;  // 左子节点id，叶节点为-1

    int32_t _right_child_id;  // 右子节点id，叶节点为-1

    myfloat_t _prediction;  // 节点预测值

    int32_t _depth;  // 节点深度

    bool _is_leaf;  // 是否是叶节点

    TreeNode() : _global_feat_id(-1),
                 _cut(0.0),
                 _local_feat_id(-1),
                 _cut_keyid(0),
                 _parent_id(-1),
                 _left_child_id(-1),
                 _right_child_id(-1),
                 _prediction(0),
                 _depth(-1),
                 _is_leaf(true) {}
};
 
class SlaveNodeSplit
{
public:
    u_set<index_t> _right_chd_index;

    int32_t _node_id;

    int32_t _global_feat_id;

    myfloat_t _cut;

    int32_t _local_feat_id;

    keyid_t _cut_keyid;

    myfloat_t _gain;

    myfloat_t _left_chd_prediction;

    myfloat_t _right_chd_prediction;
    
    int32_t _node_depth;
};

class MasterNodeSplit
{
public:
    int32_t _node_id;

    int32_t _slave_id;

    myfloat_t _gain;

    int32_t _depth;

    bool operator < (const MasterNodeSplit& b) const 
    {
        return _gain < b._gain;
    }

    MasterNodeSplit(const SlaveNodeSplit* p_slave_split) 
    {
        if (nullptr == p_slave_split) {
            LOG(FATAL) << "FATAL ERROR in MasterNodeSplit::MasterNodeSplit(): "
                       << "p_slave_split is nullptr!";
        }
        _node_id = p_slave_split->_node_id;
        _slave_id = Params::_machine_id;
        _gain = p_slave_split->_gain;
        _depth = p_slave_split->_node_depth;
    }

    MasterNodeSplit() : _node_id(-1),
                        _slave_id(-1),
                        _gain(0.0),
                        _depth(-1) {}

};

class NodeSplitHolder
{
public:
    // 通过搜索Loss，得到最佳的叶节点分裂方式，并存入_split_candidates
    SlaveNodeSplit* find_local_best_split(const Loss* p_loss, TreeNode* p_node);
    // 从_split_candidates中取出对象，但并不销毁。用于将SlaveNodeSplit转移给TreeTrainer
    // 如果node_id不存在，返回nullptr
    SlaveNodeSplit* take_out_split(int32_t node_id);
    // 从_split_candidates中移除并销毁对象
    void destroy_split(int32_t node_id);

    ~NodeSplitHolder()
    {
        for (auto& p : _split_candidates) {
            delete p.second;
            p.second = nullptr;
        }
        _split_candidates.clear();
    }

private:
    // 保存所有叶节点用本机上特征进行分裂的最佳方式
    u_map<int32_t, SlaveNodeSplit*> _split_candidates;

    void find_best_cut_keyid(const std::vector<LossElem>& loss_elem_buckets, 
                             myfloat_t prediction, keyid_t& cut_keyid,
                             myfloat_t& gain, myfloat_t& new_prediction, 
                             myfloat_t& left_prediction, myfloat_t& right_prediction);

    void calc_node_obj_loss(const LossElem& loss_sum, myfloat_t prediction, 
                            myfloat_t& node_obj_loss, myfloat_t& new_prediction);
};

}
}

#endif
