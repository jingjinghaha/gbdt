/**
* Copyright (C) 2014-2016 All rights reserved.
* @file 
* @brief 
* @details 
* @author Weidong Huang (huangweidong01@baidu.com)
* @version 1.0
* @date 2016-11-22 19:41
*/
#include "sampling.h"

namespace idl {
namespace dfgbdt {

void Sampling::sampling_importance_rcd(TreeNode* p_root_node,
                                      Loss* p_loss) {
    LOG(INFO) << "Start to init loss";
    p_loss->init_loss();
    TrainDataSet* train_data_ptr = TrainDataSet::get_data_ptr();
    index_t rcd_num = train_data_ptr->size();
    LOG(INFO) << "sampling_method:" << Params::_sampling_method;
    LOG(INFO) << "sampling param:" << Params::_sampling_param;
    LOG(INFO) << "Finish initing loss, rcd_num:" << rcd_num
               << ",start to sampling importance rcd"
               << "sampling_method:" << Params::_sampling_method;
    //whether to update sampling prob,because tree start sample at iteration 1,
    //so we mod equal 1 will to calc sampling prob
    if (Params::_cur_tree_iteration 
         % Params::_sampling_update_iteration == 1) { 
         calc_sampling_prob(p_loss);
     }
    //sampling
    for (index_t rcd_index = 0; rcd_index < rcd_num; ++rcd_index) {
        //if(train_data_ptr->_tags[rcd_index] == 1) {
        //    p_root_node->_node_index.push_back(rcd_index);
        //    continue;
        //}
        double prob = train_data_ptr->_probs[rcd_index];
        if (rand() > RAND_MAX * prob) {
            VLOG(3) << "prob:" << prob << "instance will be filtered";
            continue;
        }
        //(*_p_rcd_loss)[rcd_index]._y = g_x / prob;
        //(*_p_rcd_loss)[rcd_index]._w = h_x / prob;
        VLOG(3) << "instance will be sampled, prob:" << prob
                << "index:" << rcd_index;
        p_root_node->_node_sample_index.push_back(rcd_index);
    }
    LOG(INFO) << "Finish Sampling, rcd_num:" << p_root_node->_node_sample_index.size();
    return;
}

void Sampling::calc_sampling_prob(Loss* p_loss) {
    TrainDataSet* train_data_ptr = TrainDataSet::get_data_ptr();
    std::vector<LossElem>* _p_rcd_loss = p_loss->get_rcd_loss();
    index_t rcd_num = train_data_ptr->size();
#pragma omp parallel for num_threads(Params::_thread_num)
    for (index_t rcd_index = 0; rcd_index < rcd_num; ++rcd_index) {
        double prob = 0.0;
        myfloat_t g_x = (*_p_rcd_loss)[rcd_index]._y;
        myfloat_t h_x = (*_p_rcd_loss)[rcd_index]._w;
        if (Params::_sampling_method == "loss") {
            myfloat_t loss = Loss::calc_ins_loss(g_x, h_x);
            VLOG(0) << "rcd_index:" << rcd_index << ",loss:" << loss;
            prob = Params::_sampling_param * loss;
        } else if (Params::_sampling_method == "grad1") {
            prob = Params::_sampling_param * fabs(g_x);
        } else if (Params::_sampling_method == "grad2") {
            prob = Params::_sampling_param * h_x;
        } else {
            prob = 1;
        }
        if (train_data_ptr->_tags[rcd_index] == 1) {
            prob = fmax(Params::_pos_prob_min, prob);
            prob = fmin(Params::_pos_prob_max, prob);
        } else {
            prob = fmax(Params::_neg_prob_min, prob);
            prob = fmin(Params::_neg_prob_max, prob);
        }
        VLOG(3) << "rcd_index:" << rcd_index
            << ",grad1:" << g_x << ",grad2:" << h_x
            << "prob:" << prob;
        train_data_ptr->_probs[rcd_index] = prob;
    }
    return;
}

}//dfgbdt
}//idl
