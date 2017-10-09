#ifndef IDL_DFGBDT_PREDICTOR_H
#define IDL_DFGBDT_PREDICTOR_H

#include "data.h"

namespace idl {
namespace dfgbdt {

class Forest;

class PredTagElem
{
public:
    myfloat_t _prediction;

    bool _is_pos;

    bool operator <(const PredTagElem& b) const 
    {
        return _prediction > b._prediction;
    }

    PredTagElem(const TestDataSet* p_test_data, index_t rcd_idx)
    {
        _prediction = p_test_data->_predictions[rcd_idx];
        _is_pos = fabs(p_test_data->_tags[rcd_idx] - 1.0) < Params::_epsilon;
    }
};

class RocCurve
{
public:
    class Coordinate
    {
    public:
        double _x;

        double _y;

        Coordinate(double x, double y) : _x(x),
                                         _y(y) {}
    };

    std::vector<Coordinate> _points;

    void append_point(double x, double y)
    {
        _points.push_back(Coordinate(x, y));
        return;
    }
};

class Predictor
{
public:
    void evaluate_forest();

    Predictor(Forest* forest) : _forest(forest) 
    {
        _p_test_data = TestDataSet::get_data_ptr();
        CHECK(_p_test_data->size() > 0) 
            << "Test data set is empty. Fail to initialize Predictor!";
    }

    ~Predictor() {}
private:
    RocCurve _roc_curve;
    // 混淆矩阵的四个元素
    index_t _pos_pos;
    index_t _pos_neg;
    index_t _neg_pos;
    index_t _neg_neg;

    double _auc;

    Forest* _forest;

    TestDataSet* _p_test_data;

    // 对所有测试样本进行预测，预测结果放入TestDataSet::_predictions中
    void predict_test_records();
    // 计算混淆矩阵
    void calc_confusion_matrix();
    // 计算roc曲线的坐标
    void calc_roc_curve();
    // 计算auc
    void calc_auc();
};

}
}

#endif
