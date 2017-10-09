#include "forest.h"
#include "predictor.h"

namespace idl {
namespace dfgbdt {

void Forest::add_tree(Tree* p_new_tree, Loss* p_loss)
{
    // 根据新得到的树的叶节点预测值更新各个训练样本的预测值
    if (Params::_is_sampling_flag == true) {
        TrainDataSet* p_train_data = TrainDataSet::get_data_ptr();
        for (index_t rcd_idx = 0; rcd_idx < p_train_data->size(); ++rcd_idx) {
            const auto& rcd = p_train_data->_signed_data[rcd_idx];
            int32_t leaf_node_id = p_new_tree->cast_train_record_to_leaf(rcd);
            std::vector<index_t>* p_node_raw_index = p_new_tree->get_node_raw_index_ptr(leaf_node_id);
            p_node_raw_index->push_back(rcd_idx);
            myfloat_t node_prediction = p_new_tree->get_node_prediction(leaf_node_id);
            _rcd_predictions[rcd_idx] += node_prediction;
        }
        if (Params::_cur_sampling_flag == true 
            && Params::_correction_method == "cb") {
            for (const int32_t& node_id : p_new_tree->_leaf_node_ids) {
                VLOG(3) << "new tree node_id:" << node_id
                    << "start to set node prob";
                //cause fully-corrective should travel node 0 prob 
                p_new_tree->correction_node_prediction(node_id);
            }
        }
    } else {
        for (const int32_t& node_id : p_new_tree->_leaf_node_ids) {
            const std::vector<index_t>* p_node_index = p_new_tree->get_node_sample_index_ptr(node_id);
            myfloat_t node_prediction = p_new_tree->get_node_prediction(node_id);
            for (const index_t& rcd_idx : *(p_node_index)) {
                _rcd_predictions[rcd_idx] += node_prediction;
            }
        }
    }
    // 新的树加入_tree中
    _trees.push_back(p_new_tree);
    LOG(INFO) << "Tree with id: " << _trees.size() - 1 << " has been added into forest.";
    return;
}

void Forest::adjust_tree_node_prediction(int32_t tree_id, int32_t node_id, myfloat_t pred_diff)
{
    _trees[tree_id]->adjust_node_prediction(node_id, pred_diff);
    return;
}

void Forest::set_rcd_prediction(index_t index, myfloat_t new_prediction)
{
    _rcd_predictions[index] = new_prediction;
    return;
}

myfloat_t Forest::get_rcd_prediction(index_t index)
{
    return _rcd_predictions[index];
}

myfloat_t Forest::predict_record(const u_map<int32_t, datafloat_t>& record)
{
    myfloat_t prediction = 0.0;
    for (int32_t tree_id = 0; tree_id < (int32_t)_trees.size(); ++tree_id) {
        prediction += _trees[tree_id]->predict_record(record);
    }
    return prediction;
}

void Forest::write_forest_to_file(const std::string& file_name)
{
    std::vector<std::string> output_vec;
    const int32_t root_node_id = 0;
    for (int32_t tree_id = 0; tree_id < (int32_t)_trees.size(); ++tree_id) {
        std::ostringstream oss;
        oss << "booster[" << tree_id << + "]:";
        output_vec.push_back(oss.str());
        _trees[tree_id]->output_tree_structure(root_node_id, output_vec);
    }

    HdfsWriter gbdt_model_writer(file_name);
    gbdt_model_writer.begin_write();
    gbdt_model_writer.write_block_data(output_vec);
    gbdt_model_writer.finish_write();
    return;
}

void SlaveForestTrainer::train_forest()
{
    LOG(INFO) << "Begin forest training.";
    for (int32_t tree_id = 0; tree_id < Params::_tree_num; ++tree_id) {
        Params::_cur_tree_iteration = tree_id;
        if (tree_id == 1) {
            Params::_cur_sampling_flag = Params::_is_sampling_flag;
        }
        calc_residues();
        Tree* p_tree = new Tree();
        TestMasterTreeTrainer* p_master_tree_trainer = new TestMasterTreeTrainer();
        SlaveTreeTrainer* p_slave_tree_trainer = 
                          new SlaveTreeTrainer(p_tree, _p_loss, p_master_tree_trainer);
        p_slave_tree_trainer->train_tree();
        _p_forest->add_tree(p_tree, _p_loss);
        delete p_slave_tree_trainer;
        p_slave_tree_trainer = nullptr;
        delete p_master_tree_trainer;
        p_master_tree_trainer = nullptr;
        if (Params::_enable_fully_corrective) {
            fc_update_forest();
        }
        if (0 == (tree_id + 1) % Params::_eval_frequency || tree_id + 1 == Params::_tree_num) {
            _p_predictor->evaluate_forest();
        }
    }
    LOG(INFO) << "Finish forest training.";
    return;
}

SlaveForestTrainer::SlaveForestTrainer(Forest* p_forest)
{
    _p_forest = p_forest;
    index_t rcd_num = TrainDataSet::get_data_ptr()->size();
    _p_loss = new Loss(rcd_num);
    _p_predictor = new Predictor(_p_forest);
}

SlaveForestTrainer::~SlaveForestTrainer() 
{
    delete _p_loss;
    _p_loss = nullptr;
    delete _p_predictor;
    _p_predictor = nullptr;
}

void SlaveForestTrainer::calc_residues()
{
    index_t rcd_num = TrainDataSet::get_data_ptr()->size();
    if ("LS" == Params::_loss_type) {
        std::vector<datafloat_t>& tags = TrainDataSet::get_data_ptr()->_tags;
//#pragma omp parallel for num_threads(Params::_thread_num)
        for (index_t rcd_idx = 0; rcd_idx < rcd_num; ++rcd_idx) {
            _p_loss->_residues[rcd_idx] = _p_forest->get_rcd_prediction(rcd_idx) - (myfloat_t)(tags[rcd_idx]);
        }
    } else {
//#pragma omp parallel for num_threads(Params::_thread_num)
        for (index_t rcd_idx = 0; rcd_idx < rcd_num; ++rcd_idx) {
            _p_loss->_residues[rcd_idx] = _p_forest->get_rcd_prediction(rcd_idx);
        }
    }
    return;
}

void SlaveForestTrainer::fc_update_forest()
{
    LOG(INFO) << "Begin updating tree.";
    int32_t tree_num = _p_forest->get_tree_num();
    calc_residues();
    // 逐棵树迭代，修正每棵树的叶节点prediction
    // 设定迭代开始位置，保证迭代最后终止于当前的最后一颗树上
    int32_t itor = tree_num - (Params::_tree_iteration_times % tree_num);
    for (int32_t i = 0; i < Params::_tree_iteration_times; ++i) {
        int32_t tree_id = itor % tree_num;
        ++itor;
        update_tree(tree_id);
    }
    // 根据迭代后最终的各样本残差情况，反向计算出各样本当前的prediction
    index_t rcd_num = TrainDataSet::get_data_ptr()->size();
    if ("LS" == Params::_loss_type) {
        std::vector<datafloat_t>& tags = TrainDataSet::get_data_ptr()->_tags;
//#pragma omp parallel for num_threads(Params::_thread_num)
        for (index_t rcd_idx = 0; rcd_idx < rcd_num; ++rcd_idx) {
            _p_forest->set_rcd_prediction(rcd_idx, _p_loss->_residues[rcd_idx] + (myfloat_t)(tags[rcd_idx]));
        }
    } else {
//#pragma omp parallel for num_threads(Params::_thread_num)
        for (index_t rcd_idx = 0; rcd_idx < rcd_num; ++rcd_idx) {
            _p_forest->set_rcd_prediction(rcd_idx, _p_loss->_residues[rcd_idx]);
        }
    }
    LOG(INFO) << "Finish updating tree.";
    return;
}

void SlaveForestTrainer::update_tree(int32_t tree_id)
{
    LOG(INFO) << "Updating tree " << tree_id;
    Tree* p_tree = _p_forest->_trees[tree_id];
#pragma omp parallel for num_threads(Params::_thread_num)
    for (int32_t i = 0; i < (int32_t)(p_tree->_leaf_node_ids).size(); ++i) {
        // 在每一个节点上计算属于该节点的样本yw的和
        int32_t node_id = (p_tree->_leaf_node_ids)[i];
        std::vector<index_t>* p_node_raw_index = p_tree->get_node_raw_index_ptr(node_id);
        std::vector<index_t>* p_node_sample_index = p_tree->get_node_sample_index_ptr(node_id);
        LossElem node_loss_sum;
        //if (Params::_cur_sampling_flag == true
        //    && Params::_correction_method == "wb") {
        //    VLOG(3) << "Start to wb calc group loss";
        //    node_loss_sum = _p_loss->calc_wb_group_loss(*p_node_sample_index);
        //} else {
        //    VLOG(3) << "Start to calc group loss";
        //    node_loss_sum = _p_loss->calc_group_loss(*p_node_sample_index);
        //}
        if (Params::_is_sampling_flag == true) {
            node_loss_sum = _p_loss->calc_group_loss(*p_node_raw_index);
        } else {
            node_loss_sum = _p_loss->calc_group_loss(*p_node_sample_index);
        }
        myfloat_t old_prediction = p_tree->get_node_prediction(node_id);
        myfloat_t y_sum = node_loss_sum._y;
        myfloat_t w_sum = node_loss_sum._w;
        // 计算该节点新的prediction
        myfloat_t new_prediction = 0.0;
        Loss::solve_l1_l2(old_prediction * w_sum + y_sum, w_sum, new_prediction);
        VLOG(3) << "After fully corrective, node_id:" << node_id
                  << "old_prediction:" << old_prediction
                  << "w_sum:" << w_sum
                  << "y_sum:" << y_sum
                  << "new_prediction:" << new_prediction
                  << "node sample size:" << p_node_sample_index->size()
                  << "node raw size:" << p_node_raw_index->size();
        //if (Params::_cur_sampling_flag == true
        //    && Params::_correction_method == "cb") {
        //    myfloat_t correction_bias = p_tree->get_correction_bias(node_id);
        //    myfloat_t correction_w = 
        //        (fabs(old_prediction) < 1e-9) 
        //        ? 0.0 : (new_prediction - old_prediction) 
        //                / old_prediction - correction_bias;
        //    VLOG(3) << "CB correction method, old_prediction:" << old_prediction
        //              << "new_prediction:" << new_prediction
        //              << "correction_bias:" << correction_bias
        //              << "correction_w:" << correction_w
        //              << "after correction prediction:"
        //              << old_prediction + old_prediction * correction_w;
        //    new_prediction = old_prediction + old_prediction * correction_w;
        //}
        // 更新节点prediction
        p_tree->set_node_prediction(node_id, new_prediction);
        VLOG(3) << "fully corrective, node_id:" << node_id
                   << "new_prediction:" << new_prediction
                   << "old_prediction:" << old_prediction
                   << "sample size:" << p_node_sample_index->size();
        // 更新属于该节点的样本的残�
        if (Params::_is_sampling_flag == true) {
            for (const index_t& rcd_idx : *(p_node_raw_index)) {
                _p_loss->_residues[rcd_idx] += new_prediction - old_prediction;
            }
        } else {
            for (const index_t& rcd_idx : *(p_node_sample_index)) {
                _p_loss->_residues[rcd_idx] += new_prediction - old_prediction;
            }
        }
    }
    return;
}

} // namespace idl
} // namespace dfgbdt
