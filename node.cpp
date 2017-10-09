#include "node.h"

namespace idl {
namespace dfgbdt {

SlaveNodeSplit* NodeSplitHolder::find_local_best_split(const Loss* p_loss, TreeNode* p_node)
{
    const std::vector<index_t>& node_index = p_node->_node_sample_index;
    CHECK(!node_index.empty()) 
         << "FATAL ERROR in NodeSplitHolder::find_local_best_split(): "
         << "_node_index of node " << p_node->_node_id << " is empty!";
    const TrainDataSet* p_train_data = TrainDataSet::get_data_ptr();
    const DiscretizationModel* p_disc_model = DiscretizationModel::get_disc_model_ptr();
    int32_t feat_num = p_disc_model->_feat_id_local_to_global.size();

    // 计算当前节点上所有record的loss之和
    LossElem total_loss;
    for (const index_t& idx : node_index) {
        total_loss.add((p_loss->_rcd_loss)[idx]);
    }
    // 对所有record里面出现过的feat进行桶排序
    std::vector<std::vector<LossElem> > feat_loss_elem_buckets(feat_num);
    for (int32_t feat_id = 0; feat_id < feat_num; ++feat_id) {
        keyid_t bucket_num = (keyid_t)((p_disc_model->_cut_points)[feat_id].size()) + 1;
        feat_loss_elem_buckets[feat_id].resize(bucket_num);
    }
#pragma omp parallel for num_threads(Params::_thread_num)
    for (index_t i = 0; i < (index_t)node_index.size(); ++i) {
        index_t idx = node_index[i];
        for (const auto& p : p_train_data->_signed_data[idx]) {
            int32_t feat_id = p.first;
            keyid_t keyid = p.second;
#pragma omp atomic
            feat_loss_elem_buckets[feat_id][keyid]._y += ((p_loss->_rcd_loss)[idx])._y;
#pragma omp atomic
            feat_loss_elem_buckets[feat_id][keyid]._w += ((p_loss->_rcd_loss)[idx])._w;	
        }
    }
    // 相减得到缺失值的loss
//#pragma omp parallel for
    for (int32_t feat_id = 0; feat_id < feat_num; ++feat_id) {
        LossElem missing_loss_sum = total_loss;
        for (keyid_t i = 1; i < (keyid_t)feat_loss_elem_buckets[feat_id].size(); ++i) {
            missing_loss_sum.minus(feat_loss_elem_buckets[feat_id][i]);
        }
        feat_loss_elem_buckets[feat_id][0] = missing_loss_sum;
    }

    // 计算每个特征上进行划分的增益
    std::vector<myfloat_t> feat_gains(feat_num);
    std::vector<keyid_t> feat_cut_keyids(feat_num);
    std::vector<myfloat_t> feat_new_predictions(feat_num);
    std::vector<myfloat_t> feat_left_predictions(feat_num);
    std::vector<myfloat_t> feat_right_predictions(feat_num);
#pragma omp parallel for num_threads(Params::_thread_num)
    for (int32_t feat_id = 0; feat_id < feat_num; ++feat_id) {
        std::vector<LossElem>& loss_elem_buckets = feat_loss_elem_buckets[feat_id];
        find_best_cut_keyid(loss_elem_buckets, p_node->_prediction, feat_cut_keyids[feat_id],
                            feat_gains[feat_id], feat_new_predictions[feat_id],
                            feat_left_predictions[feat_id], feat_right_predictions[feat_id]);
    }
    // 找出增益最大的特征
    int32_t best_feat_id = -1;
    myfloat_t max_gain = 0.0;
    for (int32_t feat_id = 0; feat_id < feat_num; ++feat_id) {
        if (feat_gains[feat_id] > max_gain + Params::_epsilon) {
            best_feat_id = feat_id;
            max_gain = feat_gains[feat_id];
        }
    }
    // 生成SlaveNodeSplit，如果没有找到合适的划分方式，返回nullptr
    if (best_feat_id < 0 || feat_gains[best_feat_id] < Params::_epsilon) {
        LOG(INFO) << "Fail to find a suitable way to split node " << p_node->_node_id;
        return nullptr;
    }
    // TODO：下一句注释掉后，根节点就不会有预测值了。
    p_node->_prediction = feat_new_predictions[best_feat_id];
    SlaveNodeSplit* p_split = new SlaveNodeSplit();
    p_split->_node_id = p_node->_node_id;
    p_split->_node_depth = p_node->_depth;
    p_split->_global_feat_id = p_disc_model->_feat_id_local_to_global.at(best_feat_id);
    p_split->_cut = p_disc_model->trans_cut_keyid_to_float(best_feat_id, 
                                                           feat_cut_keyids[best_feat_id]);
	p_split->_local_feat_id = best_feat_id;
	p_split->_cut_keyid = feat_cut_keyids[best_feat_id];
    p_split->_gain = feat_gains[best_feat_id];
    p_split->_left_chd_prediction = feat_left_predictions[best_feat_id];
    p_split->_right_chd_prediction = feat_right_predictions[best_feat_id];
    p_split->_right_chd_index.reserve(node_index.size() / 2);
    
    for (const index_t& idx : node_index) {
        const auto itor = (p_train_data->_signed_data)[idx].find(best_feat_id);
        if (itor != (p_train_data->_signed_data)[idx].end() && 
            itor->second > feat_cut_keyids[best_feat_id]) {
            p_split->_right_chd_index.insert(idx);
        }
    }
    _split_candidates[p_node->_node_id] = p_split;
    LOG(INFO) << "A slave node split has been built. "
              << "node_id: " << p_split->_node_id << " global_feat_id: " << p_split->_global_feat_id
              << " cut: " << p_split->_cut << " gain: " << p_split->_gain;
    return p_split;
}

SlaveNodeSplit* NodeSplitHolder::take_out_split(int32_t node_id)
{
    if (!_split_candidates.count(node_id)) {
        return nullptr;
    }
    SlaveNodeSplit* p_split = _split_candidates.at(node_id);
    _split_candidates.erase(node_id);
    return p_split;
}

void NodeSplitHolder::destroy_split(int32_t node_id)
{
    if (_split_candidates.count(node_id)) {
        delete _split_candidates.at(node_id);
        _split_candidates.erase(node_id);
    }
    return;
}

/** ========================== Private ==============================**/

void NodeSplitHolder::find_best_cut_keyid(const std::vector<LossElem>& loss_elem_buckets, 
                                          myfloat_t prediction, keyid_t& cut_keyid,
                                          myfloat_t& gain, myfloat_t& new_prediction, 
                                          myfloat_t& left_prediction, myfloat_t& right_prediction)
{
    CHECK(!loss_elem_buckets.empty())
         << "FATAL ERROR in NodeSplitHolder::find_best_cut_keyid(): "
         << "loss_elem_buckets is empty!";
    std::vector<LossElem> bucket_sum(loss_elem_buckets.size());
    bucket_sum[0].set(loss_elem_buckets[0]);
    for (keyid_t b_id = 1; b_id < (keyid_t)bucket_sum.size(); ++b_id) {
        bucket_sum[b_id].set(bucket_sum[b_id - 1]);
        bucket_sum[b_id].add(loss_elem_buckets[b_id]);
    }
    // 计算当前节点的目标函数损失
    myfloat_t total_obj_loss = 0.0;
    calc_node_obj_loss(bucket_sum.back(), prediction, total_obj_loss, new_prediction);
    left_prediction = new_prediction;
    right_prediction = new_prediction;
    // 遍历切分方案，keyid<=cut的样本被分到左枝，其他的分到右枝
    cut_keyid = -1;
    gain = 0.0;
    for (keyid_t cut = 0; cut < (keyid_t)bucket_sum.size() - 1; ++cut) {
        if (cut > 0 && gain > Params::_epsilon && bucket_sum[cut] == bucket_sum[cut - 1]) {
            continue;
        }
        LossElem left_node_loss = bucket_sum[cut];
        LossElem right_node_loss = bucket_sum.back() - bucket_sum[cut];
        if (left_node_loss._w < Params::_min_node_weight || 
            right_node_loss._w < Params::_min_node_weight) {
            continue;
        }
        // 左子节点
        myfloat_t left_obj_loss_cand = 0.0;
        myfloat_t left_prediction_cand = 0.0;
        calc_node_obj_loss(left_node_loss, prediction, left_obj_loss_cand, left_prediction_cand);
        // 右子节点
        myfloat_t right_obj_loss_cand = 0.0;
        myfloat_t right_prediction_cand = 0.0;
        calc_node_obj_loss(right_node_loss, prediction, right_obj_loss_cand, right_prediction_cand);
        
        myfloat_t gain_cand = total_obj_loss - (left_obj_loss_cand + right_obj_loss_cand);

        if (gain_cand > gain + Params::_epsilon) {
            cut_keyid = cut;
            gain = gain_cand;
            left_prediction = left_prediction_cand;
            right_prediction = right_prediction_cand;
        }
    }
    return;
}

void NodeSplitHolder::calc_node_obj_loss(const LossElem& loss_sum, myfloat_t prediction, 
                                         myfloat_t& node_obj_loss, myfloat_t& new_prediction)
{
    myfloat_t y_sum = loss_sum._y;
    myfloat_t w_sum = loss_sum._w;
    node_obj_loss = Loss::solve_l1_l2(prediction * w_sum + y_sum, w_sum, new_prediction);
    return;
}

} // namespace dfgbdt
} // namespace idl
