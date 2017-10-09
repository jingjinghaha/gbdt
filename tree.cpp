#include "tree.h"

namespace idl {
namespace dfgbdt {

void Tree::split_node(const SlaveNodeSplit* p_split, 
                      int32_t& left_child_id, int32_t& right_child_id)
{
    LOG(INFO) << "Begin spliting node " << p_split->_node_id;
    TreeNode* p_parent = _nodes[p_split->_node_id];
    TreeNode* p_left = new TreeNode();
    TreeNode* p_right = new TreeNode();

    // 划分节点records
    index_t parent_index_num = p_parent->_node_sample_index.size();
    p_left->_node_sample_index.reserve(parent_index_num / 2);
    p_right->_node_sample_index.reserve(parent_index_num / 2);
    for (const index_t& idx : p_parent->_node_sample_index) {
        if ((p_split->_right_chd_index).count(idx)) {
            p_right->_node_sample_index.push_back(idx);
        } else {
            p_left->_node_sample_index.push_back(idx);
        }
    }
    LOG(INFO) << "parent node_id:" << p_split->_node_id
            << "left node size:" << p_left->_node_sample_index.size()
            << "right node size:" << p_right->_node_sample_index.size();
    p_parent->_node_sample_index.clear();
    // node_id
    p_left->_node_id = _nodes.size();
    p_right->_node_id = _nodes.size() + 1;
    p_left->_parent_id = p_parent->_node_id;
    p_right->_parent_id = p_parent->_node_id;
    p_parent->_left_child_id = p_left->_node_id;
    p_parent->_right_child_id = p_right->_node_id;
    left_child_id = p_left->_node_id;
    right_child_id = p_right->_node_id;
    // 其他属性
    p_parent->_global_feat_id = p_split->_global_feat_id;
    p_parent->_cut = p_split->_cut;
	p_parent->_local_feat_id = p_split->_local_feat_id;
	p_parent->_cut_keyid = p_split->_cut_keyid;
    p_left->_prediction = p_split->_left_chd_prediction;
    p_right->_prediction = p_split->_right_chd_prediction;
    p_left->_depth = p_parent->_depth + 1;
    p_right->_depth = p_parent->_depth + 1;
    p_parent->_is_leaf = false;

    _nodes.push_back(p_left);
    _nodes.push_back(p_right);
    ++_leaf_num;
    LOG(INFO) << "Left child node with id: " << left_child_id << " has been created.";
    LOG(INFO) << "    It's prediction is " << p_left->_prediction;
    LOG(INFO) << "Right child node with id: " << right_child_id << " has been created.";
    LOG(INFO) << "     It's prediction is " << p_right->_prediction;
    return;
}

void Tree::register_leaf_node_ids()
{
    _leaf_node_ids.clear();
    for (int32_t node_id = 0; node_id < node_num(); ++node_id) {
        if (_nodes[node_id]->_is_leaf) {
            if (_nodes[node_id]->_left_child_id != -1 ||
                _nodes[node_id]->_right_child_id != -1 ||
                _nodes[node_id]->_node_sample_index.empty()) {
                LOG(FATAL) << "FATAL ERROR in Tree::register_leaf_node_ids(): "
                           << "Member variable error detected in TreeNode No." << node_id;
            }
            _leaf_node_ids.push_back(node_id);
        }
    }
    return;
}

void Tree::adjust_node_prediction(int32_t node_id, myfloat_t pred_diff)
{
    _nodes[node_id]->_prediction += pred_diff;
    return;
}

void Tree::scale_node_prediction(int32_t node_id, myfloat_t pred_scaling)
{
    _nodes[node_id]->_prediction *= pred_scaling;
    return;
}

void Tree::set_node_prediction(int32_t node_id, myfloat_t prediction)
{
    _nodes[node_id]->_prediction = prediction;
    return;
}

void Tree::add_node(TreeNode* p_node)
{
    _nodes.push_back(p_node);
    ++_leaf_num;
    return;
}

int32_t Tree::cast_train_record_to_leaf(const u_map<int32_t, keyid_t>& record)
{
	const int32_t root_node_id = 0;
	const TreeNode* p_cur_node = _nodes[root_node_id];
	while (!p_cur_node->_is_leaf) {
		int32_t local_split_feat_id = p_cur_node->_local_feat_id;
		if (!record.count(local_split_feat_id) || 
			record.at(local_split_feat_id) <= p_cur_node->_cut_keyid) {
			p_cur_node = _nodes[p_cur_node->_left_child_id];
		} else {
            p_cur_node = _nodes[p_cur_node->_right_child_id];
		}
	}
	return p_cur_node->_node_id;
}

myfloat_t Tree::predict_record(const u_map<int32_t, datafloat_t>& record)
{
    const int32_t root_node_id = 0;
    const TreeNode* p_cur_node = _nodes[root_node_id];
    while (!p_cur_node->_is_leaf) {
        int32_t global_split_feat_id = p_cur_node->_global_feat_id;
        // 缺失值和<=cut的样本被分到左枝
        if (!record.count(global_split_feat_id) || 
            (myfloat_t)(record.at(global_split_feat_id)) <= p_cur_node->_cut) {
            p_cur_node = _nodes[p_cur_node->_left_child_id];
        } else {
            p_cur_node = _nodes[p_cur_node->_right_child_id];
        }
    }
    return p_cur_node->_prediction;
}

void Tree::correction_node_prediction(int32_t node_id) {
    if (_is_calc_bias_flag == false) {
        calc_prob_bias(_nodes[node_id]);
        _is_calc_bias_flag = true;
    }
    LOG(INFO) << "correction node prediction, node_id:" << node_id
              << "prediction:" 
              << _nodes[node_id]->_prediction
              << "bias:" << _correction_bias;
    adjust_node_prediction(node_id, _correction_bias);
}

myfloat_t Tree::get_correction_bias(int32_t node_id) {
    if (_is_calc_bias_flag == false) {
        calc_prob_bias(_nodes[node_id]);
        _is_calc_bias_flag = true;
    }
    return _correction_bias;
}

void Tree::calc_prob_bias(TreeNode* p_node) {
    TrainDataSet* p_train_data = TrainDataSet::get_data_ptr();
    const std::vector<index_t>& index_vec = p_node->_node_sample_index;
    size_t pos_num = 0;
    size_t neg_num = 0;
    myfloat_t pos_sum = 0.0;
    myfloat_t neg_sum = 0.0;
    for (size_t idx : index_vec) {
        if (1 == p_train_data->_tags[idx]) {
            pos_sum += p_train_data->_probs[idx];
            ++pos_num;
        } else {
            neg_sum += p_train_data->_probs[idx];
            ++neg_num;
        }
    }
    myfloat_t pos_avg = ((pos_num == 0) ? 0.0 : pos_sum / pos_num);
    myfloat_t neg_avg = ((neg_num == 0) ? 0.0 : neg_sum / neg_num);
    if (pos_num == 0 || neg_num == 0) {
        LOG(INFO) << "calc prob bias, pos num is:" << pos_num
                 << "neg num is:" << neg_num;
        _correction_bias = 0.0;
    } else {
        _correction_bias = std::log((pos_avg) / (neg_avg));
    }
    LOG(INFO) << "calc prob bias, pos_sum:" << pos_sum
              << "pos_num:" << pos_num 
              << "neg_sum:" << neg_sum
              << "neg_num:" << neg_num
              << "bias:" << _correction_bias;
    return;
}

void Tree::output_tree_structure(int32_t root_node_id, std::vector<std::string>& output_vec)
{
    const int32_t null_id = -1;
    if (root_node_id == null_id) {
        return;
    }
    const TreeNode* p_node = _nodes[root_node_id];
    std::ostringstream oss;
    oss.str("");
    const std::string indent = "    ";
    for (int32_t i = 0; i < p_node->_depth; ++i) {
        oss << indent;
    }
    oss << p_node->_node_id << ":";
    if (p_node->_is_leaf) {
        oss << "leaf=" << (p_node->_prediction);
        output_vec.push_back(oss.str());
    } else {
        oss << "[" << (p_node->_global_feat_id) << "<" << (p_node->_cut) << "]";
        int32_t left_chd_id = p_node->_left_child_id;
        int32_t right_chd_id = p_node->_right_child_id;
        oss << " yes=" << left_chd_id << ",no=" << right_chd_id << ",missing=" << left_chd_id;
        output_vec.push_back(oss.str());
        output_tree_structure(left_chd_id, output_vec);
        output_tree_structure(right_chd_id, output_vec);
    }
    return;
}

void SlaveTreeTrainer::train_tree()
{
    LOG(INFO) << "Begin training a new tree.";
    //_p_loss->init_loss();
    //if (!Params::_cur_sampling_flag) {
    //    _p_loss->init_loss();
    //}
    create_root_node();

    while (true) {
        _p_master_trainer->train_tree();
        MasterNodeSplit* p_master_split = receive_split_from_master();
        if (nullptr == p_master_split) {
            break;
        }
        SlaveNodeSplit* p_slave_split = get_corresponding_split(p_master_split);
        delete p_master_split;
        p_master_split = nullptr;
        create_new_leaves(p_slave_split);
        delete p_slave_split;
    }
    myfloat_t prediction_scaling = Params::_enable_fully_corrective ? 
                                   1.0 : Params::_step_size;
    for (int32_t node_id = 0; node_id < _p_tree->node_num(); ++node_id) {
        _p_tree->scale_node_prediction(node_id, prediction_scaling);
    }
    _p_tree->register_leaf_node_ids();
    LOG(INFO) << "Finish training a tree.";
    return;
}

void SlaveTreeTrainer::create_root_node()
{
   if (Params::_cur_sampling_flag == true) {
       sampling_add_root_node();
   } else {
       add_root_node();
   }
    build_root_node();
    return;
}

void SlaveTreeTrainer::sampling_add_root_node() {
    const int32_t root_node_id = 0;
    TreeNode* p_root_node = new TreeNode();
    Sampling::sampling_importance_rcd(p_root_node, _p_loss);
    p_root_node->_node_id = root_node_id;
    p_root_node->_depth = 0;
    _p_tree->add_node(p_root_node);
    return;
}

void SlaveTreeTrainer::add_root_node() {
    const int32_t root_node_id = 0;
    index_t rcd_num = TrainDataSet::get_data_ptr()->size();
    TreeNode* p_root_node = new TreeNode();
    p_root_node->_node_sample_index.reserve(rcd_num);
    for (index_t rcd = 0; rcd < rcd_num; ++rcd) {
        p_root_node->_node_sample_index.push_back(rcd);
    }
    p_root_node->_node_id = root_node_id;
    p_root_node->_depth = 0;
    _p_tree->add_node(p_root_node);

    update_loss(root_node_id);
    return;
}

void SlaveTreeTrainer::build_root_node() {
    const int32_t root_node_id = 0;
    SlaveNodeSplit* p_root_split = build_split(root_node_id);
    if (p_root_split) {
        MasterNodeSplit* p_master_root_split = new MasterNodeSplit(p_root_split);
        send_split_to_master(p_master_root_split, _p_tree->leaf_num());
        delete p_master_root_split;
        p_master_root_split = nullptr;
    }
    LOG(INFO) << "Finish creating tree root node.";
    return;
}

void SlaveTreeTrainer::update_loss(int32_t node_id)
{
    const std::vector<index_t>& node_index = _p_tree->_nodes[node_id]->_node_sample_index;
    myfloat_t node_prediction = _p_tree->_nodes[node_id]->_prediction;
    if (Params::_cur_sampling_flag == true
        && Params::_correction_method == "wb") {
        _p_loss->calc_wb_rcd_loss(node_index, node_prediction);
    } else {
        _p_loss->calc_rcd_loss(node_index, node_prediction);
    }
    return;
}

SlaveNodeSplit* SlaveTreeTrainer::build_split(int32_t node_id)
{
    TreeNode* p_node = _p_tree->_nodes[node_id];
    if (p_node->_depth == 0 || p_node->_depth < Params::_max_tree_depth) {
        return _p_split_holder->find_local_best_split(_p_loss, p_node);
    } else {
        return nullptr;
    }
}

void SlaveTreeTrainer::send_split_to_master(MasterNodeSplit* p_split, int32_t tree_leaf_num)
{
    std::vector<MasterNodeSplit>& master_input_channel = _p_master_trainer->_input_channel;
    if (p_split) {
        master_input_channel.push_back(*p_split);
        LOG(INFO) << "Slave node split of node " << p_split->_node_id 
                  << " has been putted into input channel of master tree trainer.";
    }
    _p_master_trainer->_tree_leaf_num = tree_leaf_num;
    return;
}

void SlaveTreeTrainer::broadcast_split_to_other_slaves(SlaveNodeSplit* p_split)
{
    // TODO
    // 在单机实现版本里什么都不做
    return;
}

MasterNodeSplit* SlaveTreeTrainer::receive_split_from_master()
{
    // TODO
    std::vector<MasterNodeSplit>& master_output_channel = _p_master_trainer->_output_channel;
    if (master_output_channel.empty()) {
        LOG(INFO) << "output channel of master tree trainer is empty.";
        return nullptr;
    }
    MasterNodeSplit* p_res = new MasterNodeSplit(master_output_channel.back());
    master_output_channel.clear();
    return p_res;
}

SlaveNodeSplit* SlaveTreeTrainer::receive_split_from_other_slave()
{
    // TODO
    LOG(FATAL) << "FATAL ERROR in SlaveTreeTrainer::receive_split_from_other_slave(): "
               << "You shall not be here!";
    return nullptr;
}

SlaveNodeSplit* SlaveTreeTrainer::get_corresponding_split(MasterNodeSplit* p_master_split)
{
    SlaveNodeSplit* p_res_split = nullptr;
    if (Params::_machine_id == p_master_split->_slave_id) {
        // 本机被选中
        p_res_split = _p_split_holder->take_out_split(p_master_split->_node_id);
        if (nullptr == p_res_split) {
            LOG(FATAL) << "FATAL ERROR in SlaveTreeTrainer::process_split_from_master(): "
                       << "Can not find split of choosen node_id in the SlaveSplitHolder!";
        }
        broadcast_split_to_other_slaves(p_res_split);
    } else {
        // 本机未被选中
        _p_split_holder->destroy_split(p_master_split->_node_id);
        p_res_split = receive_split_from_other_slave();
    }
    return p_res_split;
}

void SlaveTreeTrainer::create_new_leaves(const SlaveNodeSplit* p_split)
{
    if (nullptr == p_split) {
        LOG(FATAL) << "FATAL ERROR in SlaveTreeTrainer::create_new_leaves(): "
                   << "p_split is nullptr!";
    }
    // 分裂被选中的树节点
    int32_t left_child_id = -1;
    int32_t right_child_id = -1;
    _p_tree->split_node(p_split, left_child_id, right_child_id);
    // 更新新生成节点上records的loss，并生成新节点的split
    update_loss(left_child_id);
    update_loss(right_child_id);
    SlaveNodeSplit* p_left_child_split = build_split(left_child_id);
    SlaveNodeSplit* p_right_child_split = build_split(right_child_id);
    // 新生成的split传给master，压入master上的priority_queue中
    int32_t leaf_num = _p_tree->leaf_num();
    if (p_left_child_split) {
        MasterNodeSplit* p_left_master_split = new MasterNodeSplit(p_left_child_split);
        send_split_to_master(p_left_master_split, leaf_num);
        delete p_left_master_split;
        p_left_master_split = nullptr;
    } else {
        send_split_to_master(nullptr, leaf_num);
    }
    if (p_right_child_split) {
        MasterNodeSplit* p_right_master_split = new MasterNodeSplit(p_right_child_split);
        send_split_to_master(p_right_master_split, leaf_num);
        delete p_right_master_split;
        p_right_master_split = nullptr;
    } else {
        send_split_to_master(nullptr, leaf_num);
    }
    // p_left_child_split和p_right_child_split的实体在SplitHolder里，不可delete
    return;
}

/**============================ 不用与单机版本========================
void MasterTreeTrainer::train_tree()
{
    while (true) {
        // 开始训练一颗新的树
        _node_split_queue.clear();
        while (true) {
            // 训练树上的每一个节点
            // 从各个slave获取split的候选，并选出各个节点上gain最大的，加入queue
            std::vector<MasterNodeSplit> new_splits = collect_splits_from_slaves();
            for (auto& s : new_splits) {
                _node_split_queue.push(s);
            }
            // _node_split_queue的size对应于树的叶节点数目，如果过大，停止这棵树的训练
            if (_node_split_queue.size() >= Params::_max_leaf_num || 
                _node_split_queue.empty()) {
                break;
            }
            // 寻找depth不超过阈值的gain最大的split
            MasterNodeSplit split;
            bool suitable_split_found = false;
            while (!_node_split_queue.empty()) {
                split = _node_split_queue.top();
                _node_split_queue.pop();
                if (split._depth < Params::_max_tree_depth) {
                    suitable_split_found = true;
                    break;
                }
            }
            // 如果未能找到合适的split，停止这棵树的训练
            if (!suitable_split_found) {
                break;
            }
            // 将选中的split广播给所有slave
            broadcast_chosen_split_to_slaves(&split);
        }
        // 向slave广播nullptr，表示一棵树训练结束
        broadcast_chosen_split_to_slaves(nullptr);
    }
    return;
}

std::vector<MasterNodeSplit> MasterTreeTrainer::collect_splits_from_slaves()
{
    // TODO，实现时候注意接收到的split有可能是nullptr
}

void MasterTreeTrainer::broadcast_chosen_split_to_slaves(MasterNodeSplit* p_chosen_split)
{
    // TODO
}

===================================================================**/

/**================ 用于单机实现版本 ==================**/
void TestMasterTreeTrainer::train_tree()
{
    std::vector<MasterNodeSplit> new_splits = collect_splits_from_slaves();
    LOG(INFO) << "Master tree trainer has collected " << new_splits.size() << " splits.";
    for (auto& s : new_splits) {
        _node_split_queue.push(s);
    }
    // _node_split_queue的size对应于树的叶节点数目，如果过大，停止这棵树的训练
    if (_tree_leaf_num >= Params::_max_leaf_num || 
        _node_split_queue.empty()) {
        broadcast_chosen_split_to_slaves(nullptr);
        return;
    }
    // 寻找depth不超过阈值的gain最大的split
    MasterNodeSplit split;
    bool suitable_split_found = false;
    while (!_node_split_queue.empty()) {
        split = _node_split_queue.top();
        _node_split_queue.pop();
        if (split._depth < Params::_max_tree_depth) {
            suitable_split_found = true;
            break;
        }
    }
    // 如果未能找到合适的split，停止这棵树的训练
    if (!suitable_split_found) {
        broadcast_chosen_split_to_slaves(nullptr);
        return;
    }
    // 将选中的split广播给所有slave
    broadcast_chosen_split_to_slaves(&split);
    return;
}

std::vector<MasterNodeSplit> TestMasterTreeTrainer::collect_splits_from_slaves()
{
    std::vector<MasterNodeSplit> res;
    std::map<int32_t, int32_t> node_id_to_res_idx;
    for (const auto& split : _input_channel) {
        int32_t node_id = split._node_id;
        if (node_id_to_res_idx.count(node_id)) {
            MasterNodeSplit& cur_max_split = res[node_id_to_res_idx.at(node_id)];
            if (cur_max_split < split) {
                cur_max_split = split;
            }
        } else {
            node_id_to_res_idx[node_id] = res.size();
            res.push_back(split);
        }
    }
    _input_channel.clear();
    if (res.size() > 2) {
        LOG(FATAL) << "FATAL ERROR in TestMasterTreeTrainer::collect_splits_from_slaves(): "
                   << "receive splits of more than two nodes!";
    }
    return res;
}

void TestMasterTreeTrainer::broadcast_chosen_split_to_slaves(MasterNodeSplit* p_chosen_split)
{
    _output_channel.clear();
    if (p_chosen_split) {
        _output_channel.push_back(*p_chosen_split);   
    }
    return;
}

}
}
