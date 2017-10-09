#ifndef IDL_DFGBDT_TREE_H
#define IDL_DFGBDT_TREE_H

#include "node.h"
#include "sampling.h"

namespace idl {
namespace dfgbdt {

class Tree
{
public:
    friend class SlaveTreeTrainer;

    std::vector<int32_t> _leaf_node_ids;  // 保存各个叶节点的id
    // 训练过程中分裂节点，并返回新生成节点的id
    void split_node(const SlaveNodeSplit* p_split,
                    int32_t& left_child_id, int32_t& right_child_id); 

    void register_leaf_node_ids();  // 树生成完成后，记录所有叶节点的id

    void adjust_node_prediction(int32_t node_id, myfloat_t pred_diff);  // 用差值的方式调整节点的预测值

    void scale_node_prediction(int32_t node_id, myfloat_t pred_scaling);  // 缩放节点的预测值

    void set_node_prediction(int32_t node_id, myfloat_t prediction);  // 重置节点的预测值

    void add_node(TreeNode* p_node);  // 直接向_nodes中添加节点
    
    void output_tree_structure(int32_t root_node_id, std::vector<std::string>& output_vec);

    int32_t node_num() {
        return _nodes.size();
    }

    int32_t leaf_num() {
        return _leaf_num;
    }

    myfloat_t get_node_prediction(int32_t node_id) {
        return _nodes[node_id]->_prediction;
    }

    void correction_node_prediction(int32_t node_id);
    
    myfloat_t get_correction_bias(int32_t node_id);
   
    void calc_prob_bias(TreeNode* p_node);
    
    std::vector<index_t>* get_node_raw_index_ptr(int32_t node_id) {
        return Params::_is_sampling_flag ? &(_nodes[node_id]->_node_raw_index) :
                                            &(_nodes[node_id]->_node_sample_index);
    }
    
    std::vector<index_t>* get_node_sample_index_ptr(int32_t node_id) {
        return &(_nodes[node_id]->_node_sample_index);
    }
    
    int32_t cast_train_record_to_leaf(const u_map<int32_t, keyid_t>& record);
    // 对一条样本进行预测
    myfloat_t predict_record(const u_map<int32_t, datafloat_t>& record);

    Tree() : _leaf_num(0) {}

    ~Tree()
    {
        for (TreeNode*& ptr : _nodes) {
            delete ptr;
            ptr = nullptr;
        }
        _nodes.clear();
    }

private:
    std::vector<TreeNode*> _nodes;
    int32_t _leaf_num;
    bool _is_calc_bias_flag = false;
    myfloat_t _correction_bias = 0.0;
};

/**==================不用于单机实现版本===================
class MasterTreeTrainer
{
public:
    void train_tree();
private:
    std::priority_queue<MasterNodeSplit> _node_split_queue;  // 最大堆

    std::vector<MasterNodeSplit> collect_splits_from_slaves();  // 从slave获得节点切分候选，并选出每个节点上最优的
    void broadcast_chosen_split_to_slaves(MasterNodeSplit* p_chosen_split);  // 向slave广播被选中的切分
};
=====================================================*/

/**================ 用于单机实现版本 ==================**/
class TestMasterTreeTrainer
{
public:
    std::vector<MasterNodeSplit> _input_channel;

    std::vector<MasterNodeSplit> _output_channel;
    
    int32_t _tree_leaf_num;

    void train_tree();

private:
    std::priority_queue<MasterNodeSplit> _node_split_queue;  // 最大堆

    // 从slave获得节点切分候选，并选出每个节点上最优的
    std::vector<MasterNodeSplit> collect_splits_from_slaves();

    void broadcast_chosen_split_to_slaves(MasterNodeSplit* p_chosen_split);  // 向slave广播被选中的切分
};
/** ===============================================**/

/** SlaveTreeTrainer类调用Tree和NodeSplitHolder两个类进行树的训练**/
class SlaveTreeTrainer
{
public:
    void train_tree();  // 训练_p_tree

    SlaveTreeTrainer(Tree* p_tree, Loss* p_loss) : _p_tree(p_tree),
                                                   _p_loss(p_loss)
    {
        _p_split_holder = new NodeSplitHolder();
    }

    /**==================用于单机实现版本==================**/
    SlaveTreeTrainer(Tree* p_tree, Loss* p_loss, 
                     TestMasterTreeTrainer* p_master_trainer) : _p_tree(p_tree),
                                                                _p_loss(p_loss),
                                                                _p_master_trainer(p_master_trainer)
    {
        _p_split_holder = new NodeSplitHolder();
    }
    /**=================================================**/

    ~SlaveTreeTrainer() {
        // 析构过程中_p_tree和_p_loss不需要删除，本来就是从外面进来的
        delete _p_split_holder;
    }

private:
    Tree* _p_tree;

    Loss* _p_loss;

    NodeSplitHolder* _p_split_holder;
    /**==================用于单机实现版本==================**/
    TestMasterTreeTrainer* _p_master_trainer;
    /**=================================================**/

    void create_root_node();
    void add_root_node();
    void sampling_add_root_node();
    void build_root_node();
    // 根据新生成的节点升级loss里面属于该节点的样本的信息
    void update_loss(int32_t node_id);
    // 产生新节点的split_candidate，存入_p_split_holder->_split_candidates
    SlaveNodeSplit* build_split(int32_t node_id);

    // 下面的函数用于和MasterTreeTrainer之间的通信
    void send_split_to_master(MasterNodeSplit* p_split, int32_t tree_leaf_num);

    void broadcast_split_to_other_slaves(SlaveNodeSplit* p_split);

    MasterNodeSplit* receive_split_from_master();

    SlaveNodeSplit* receive_split_from_other_slave();
    // 处理来自master的最优分割点，如果自己被选中，调用send_split_to_other_slaves()，同时返回
    // 如果没被选中，则调用receive_split_from_other_slave()接收来自被选中slave的广播
    SlaveNodeSplit* get_corresponding_split(MasterNodeSplit* p_master_split);
    // 分裂节点，并生成新节点的split
    void create_new_leaves(const SlaveNodeSplit* p_split);
};

}
}

#endif
