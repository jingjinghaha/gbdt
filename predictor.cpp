#include "predictor.h"
#include "forest.h"

namespace idl {
namespace dfgbdt {

void Predictor::evaluate_forest()
{
    predict_test_records();
    calc_confusion_matrix();
    calc_auc();
    return;
}

void Predictor::predict_test_records()
{
    index_t rcd_num = _p_test_data->size();
    (_p_test_data->_predictions).resize(rcd_num);
#pragma omp parallel for
    for (index_t rcd_idx = 0; rcd_idx < rcd_num; ++rcd_idx) {
        const u_map<int32_t, datafloat_t>& record = (_p_test_data->_raw_data)[rcd_idx];
        (_p_test_data->_predictions)[rcd_idx] = _forest->predict_record(record);
    }
    return;
}

void Predictor::calc_confusion_matrix()
{
    index_t rcd_num = _p_test_data->size();
    _pos_pos = 0;
    _pos_neg = 0;
    _neg_pos = 0;
    _neg_neg = 0;
    for (index_t rcd_idx = 0; rcd_idx < rcd_num; ++rcd_idx) {
        bool tag_is_pos = fabs(_p_test_data->_tags[rcd_idx] - 1.0) < Params::_epsilon;
        bool pred_is_pos = (_p_test_data->_predictions)[rcd_idx] > Params::_pos_threshold;
        tag_is_pos ? (pred_is_pos ? ++_pos_pos : ++_pos_neg) : 
                     (pred_is_pos ? ++_neg_pos : ++_neg_neg);
    }
    LOG(INFO) << "Finish calculating confusion matrix.";
    LOG(INFO) << "pos_pos: " << _pos_pos << " pos_neg: " << _pos_neg 
              << " neg_pos: " << _neg_pos << " neg_neg: " << _neg_neg;
    return;
}

void Predictor::calc_roc_curve()
{
    index_t rcd_num = _p_test_data->size();
    std::vector<PredTagElem> pred_tag_elems;
    pred_tag_elems.reserve(rcd_num);
    for (index_t rcd_idx = 0; rcd_idx < rcd_num; ++rcd_idx) {
        pred_tag_elems.push_back(PredTagElem(_p_test_data, rcd_idx));
    }
    index_t total_pos_num = 0;
    index_t total_neg_num = 0;
    for (index_t rcd_idx = 0; rcd_idx < rcd_num; ++rcd_idx) {
        pred_tag_elems[rcd_idx]._is_pos ? ++total_pos_num : ++total_neg_num;
    }
    std::sort(pred_tag_elems.begin(), pred_tag_elems.end());
    _roc_curve._points.clear();
    _roc_curve._points.reserve(rcd_num);
    _roc_curve.append_point(0.0, 0.0);
    index_t cur_pos_num = 0;
    index_t cur_neg_num = 0;
    for (index_t rcd_idx = 0; rcd_idx < rcd_num; ++rcd_idx) {
        pred_tag_elems[rcd_idx]._is_pos ? ++cur_pos_num : ++cur_neg_num;
        if (rcd_idx == rcd_num - 1 ||
            pred_tag_elems[rcd_idx]._prediction > pred_tag_elems[rcd_idx + 1]._prediction) {
            double x = (double)cur_neg_num / (double)total_neg_num;
            double y = (double)cur_pos_num / (double)total_pos_num;
            _roc_curve.append_point(x, y);
        }
    }
    _roc_curve._points.shrink_to_fit();
    return;
}

void Predictor::calc_auc()
{
    LOG(INFO) << "Begin calculating auc...";
    _roc_curve._points.clear();
    calc_roc_curve();
    _auc = 0.0;
    for (index_t i = 0; i < (index_t)_roc_curve._points.size() - 1; ++i) {
        _auc += 0.5 * (_roc_curve._points[i]._y + _roc_curve._points[i + 1]._y) 
                    * (_roc_curve._points[i + 1]._x - _roc_curve._points[i]._x);
    }
    LOG(INFO) << "Finish calculating auc. Auc of " << _forest->get_tree_num() << " tree(s) = " << _auc;
    return;
}

}
}
