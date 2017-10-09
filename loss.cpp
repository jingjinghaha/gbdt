#include <math.h>
#include "loss.h"

namespace idl {
namespace dfgbdt {

void Loss::init_loss()
{
    index_t rcd_num = TrainDataSet::get_data_ptr()->size();
    std::vector<index_t> all_rcd(rcd_num);
    for (index_t idx = 0; idx < rcd_num; ++idx) {
        all_rcd[idx] = idx;
    }
    calc_rcd_loss(all_rcd, 0.0);
    return;
}

void Loss::calc_wb_rcd_loss(const std::vector<index_t>& rcds, myfloat_t prediction)
{
    TrainDataSet* p_train_data = TrainDataSet::get_data_ptr();
    TrainDataSet::Weights& weights = p_train_data->_weights;
    std::vector<datafloat_t>& tags = p_train_data->_tags;
    std::vector<datafloat_t>& probs = p_train_data->_probs;
    if ("LS" == Params::_loss_type) {
        for (const index_t& idx : rcds) {
            calc_LS_loss(weights[idx]/probs[idx],
                         _residues[idx] + prediction, LOSS_SET, _rcd_loss[idx]);
        }
    } else if ("MODLS" == Params::_loss_type) {
        for (const index_t& idx : rcds) {
            myfloat_t binary_label = tag_to_binary(tags[idx], 1.0, -1.0);
            calc_ModLS_loss(weights[idx]/probs[idx],
                            _residues[idx] + prediction, binary_label, LOSS_SET, _rcd_loss[idx]);
        }
    } else if ("LOGISTIC" == Params::_loss_type) {
        for (const index_t& idx : rcds) {
            myfloat_t is_inclass = tag_to_binary(tags[idx], 1.0, 0.0);
            calc_Logistic_loss(weights[idx]/probs[idx], 
                               _residues[idx] + prediction, is_inclass, LOSS_SET, _rcd_loss[idx]);
            VLOG(3) << "calc rcd loss,idx:" << idx
                    << "prob:" << probs[idx]
                    << "weight:" << weights[idx]
                    << "residues:" << _residues[idx];
        }
    } else {
        LOG(FATAL) << "FATAL ERROR in Loss::calc_rcd_loss(): "
                   << "Unknown loss_type: " << Params::_loss_type;
    }
    // fgbdt这里还有一句：_prediction0 = pred0;
    return;
}

void Loss::calc_rcd_loss(const std::vector<index_t>& rcds, myfloat_t prediction)
{
    TrainDataSet* p_train_data = TrainDataSet::get_data_ptr();
    TrainDataSet::Weights& weights = p_train_data->_weights;
    std::vector<datafloat_t>& tags = p_train_data->_tags;
    if ("LS" == Params::_loss_type) {
        for (const index_t& idx : rcds) {
            calc_LS_loss(weights[idx],
                         _residues[idx] + prediction, LOSS_SET, _rcd_loss[idx]);
        }
    } else if ("MODLS" == Params::_loss_type) {
        for (const index_t& idx : rcds) {
            myfloat_t binary_label = tag_to_binary(tags[idx], 1.0, -1.0);
            calc_ModLS_loss(weights[idx],
                            _residues[idx] + prediction, binary_label, LOSS_SET, _rcd_loss[idx]);
        }
    } else if ("LOGISTIC" == Params::_loss_type) {
        for (const index_t& idx : rcds) {
            myfloat_t is_inclass = tag_to_binary(tags[idx], 1.0, 0.0);
            calc_Logistic_loss(weights[idx], 
                               _residues[idx] + prediction, is_inclass, LOSS_SET, _rcd_loss[idx]);
        }
    } else {
        LOG(FATAL) << "FATAL ERROR in Loss::calc_rcd_loss(): "
                   << "Unknown loss_type: " << Params::_loss_type;
    }
    // TODO：fgbdt这里还有一句：_prediction0 = pred0;
    return;
}

LossElem Loss::calc_wb_group_loss(const std::vector<index_t>& rcds)
{
    TrainDataSet* p_train_data = TrainDataSet::get_data_ptr();
    TrainDataSet::Weights& weights = p_train_data->_weights;
    std::vector<datafloat_t>& tags = p_train_data->_tags;
    std::vector<datafloat_t>& probs = p_train_data->_probs;
    LossElem res_elem;
    CHECK(probs.size() != 0);
    if ("LS" == Params::_loss_type) {
        for (size_t idx : rcds) {
            calc_LS_loss(weights[idx]/probs[idx], _residues[idx],
                         LOSS_ADD, res_elem);
        }
    } else if ("ModLS" == Params::_loss_type) {
        for (size_t idx : rcds) {
            myfloat_t binary_label = tag_to_binary(tags[idx], 1.0, -1.0);
            calc_ModLS_loss(weights[idx]/probs[idx], _residues[idx], 
                            binary_label, LOSS_ADD, res_elem);
        }
    } else if ("LOGISTIC" == Params::_loss_type) { 
        for (size_t idx : rcds) {
            myfloat_t is_inclass = tag_to_binary(tags[idx], 1.0, 0.0); 
            calc_Logistic_loss(weights[idx]/probs[idx], _residues[idx], 
                               is_inclass, LOSS_ADD, res_elem);
            VLOG(3) << "calc wb group loss,idx:" << idx
                    << "prob:" << probs[idx]
                    << "weight:" << weights[idx]
                    << "residues:" << _residues[idx];
        }
    }
    return res_elem;
}

LossElem Loss::calc_group_loss(const std::vector<index_t>& rcds)
{
    TrainDataSet* p_train_data = TrainDataSet::get_data_ptr();
    TrainDataSet::Weights& weights = p_train_data->_weights;
    std::vector<datafloat_t>& tags = p_train_data->_tags;
    LossElem res_elem;
    if ("LS" == Params::_loss_type) {
        for (auto & idx : rcds) {
            calc_LS_loss(weights[idx], _residues[idx],
                         LOSS_ADD, res_elem);
        }
    } else if ("ModLS" == Params::_loss_type) {
        for (auto & idx : rcds) {
            myfloat_t binary_label = tag_to_binary(tags[idx], 1.0, -1.0);
            calc_ModLS_loss(weights[idx], _residues[idx], 
                            binary_label, LOSS_ADD, res_elem);
        }
    } else if ("LOGISTIC" == Params::_loss_type) { 
        for (auto & idx : rcds) {
            myfloat_t is_inclass = tag_to_binary(tags[idx], 1.0, 0.0); 
            calc_Logistic_loss(weights[idx], _residues[idx], 
                               is_inclass, LOSS_ADD, res_elem);
            VLOG(3) << "idx :"  << idx
                    << "loss y:" << res_elem._y
                    << "loss w:" << res_elem._w
                    << "weights:" << weights[idx]
                    << "residues:" << _residues[idx];
        }
    }
    return res_elem;
}
/**========================== Private =========================**/

void Loss::calc_LS_loss(myfloat_t weight, myfloat_t residue, LossElemOperation op_type, LossElem& elem)
{
    double nfp = -residue;
    double fpp = 1.0;
    LOSS_ADD == op_type ? elem.add(nfp * weight, fpp * weight) : elem.set(nfp * weight, fpp * weight);
    return;
}

void Loss::calc_ModLS_loss(myfloat_t weight, myfloat_t residue, myfloat_t binary_label,
                           LossElemOperation op_type, LossElem& elem)
{
    double fpp = (residue * binary_label > 1.0) ? 
                    std::max(0.1, 1.0 - 5.0 * (residue * binary_label - 1.0)) : 1.0;
    double nfp = (residue * binary_label > 1.0) ? 0.0 : (binary_label - residue);
    LOSS_ADD == op_type ? elem.add(nfp * weight, fpp * weight) : elem.set(nfp * weight, fpp * weight);
    return;
}

void Loss::calc_Logistic_loss(myfloat_t weight, myfloat_t residue, myfloat_t is_inclass,
                              LossElemOperation op_type, LossElem& elem)
{
    double p = 1.0 / (1.0 + exp(-residue));
    double nfp = is_inclass - p;
    double fpp = p * (1.0 - p);
    LOSS_ADD == op_type ? elem.add(nfp * weight, fpp * weight) : elem.set(nfp * weight, fpp * weight);
    return;
}

myfloat_t Loss::tag_to_binary(myfloat_t tag, myfloat_t pos_label, myfloat_t neg_label)
{
    return fabs(tag - 1.0) < (10.0 * Params::_epsilon) ? pos_label : neg_label;
}

} // namespace dfgbdt
} // namespace idl
