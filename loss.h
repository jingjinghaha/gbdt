#ifndef IDL_DFGBDT_LOSS_H
#define IDL_DFGBDT_LOSS_H

#include "data.h"

namespace idl {
namespace dfgbdt {

class LossElem
{
public:
    // 使用double主要是因为LossElem可能表示很多样本的损失的和
    // 对于这种大量样本相加的情况float会出现精度不足
    double _y;

    double _w;
    
    void set(myfloat_t y, myfloat_t w) {
        _y = y;
        _w = w;
        return;
    }

    void set(const LossElem& elem) {
        _y = elem._y;
        _w = elem._w;
        return;
    }
    
    void add(myfloat_t y, myfloat_t w) {
        _y += y;
        _w += w;
        return;
    }

    void add(const LossElem& b) {
        _y += b._y;
        _w += b._w;
    }

    void minus(const LossElem& b) {
        _y -= b._y;
        _w -= b._w;
        return;
    }

    LossElem operator +(const LossElem& b) {
        LossElem res;
        res._y = _y + b._y;
        res._w = _w + b._w;
        return res;
    }

    LossElem operator -(const LossElem& b) {
        LossElem res;
        res._y = _y - b._y;
        res._w = _w - b._w;
        return res;
    }

    bool operator ==(const LossElem& b) {
        return (_y > b._y - Params::_dbl_epsilon && _y < b._y + Params::_dbl_epsilon) &&
               (_w > b._w - Params::_dbl_epsilon && _w < b._w + Params::_dbl_epsilon);
    }

    LossElem() : _y(0.0),
                 _w(0.0) {}

};

class Loss
{
public:
    std::vector<myfloat_t> _residues;  // 各个样本当前的训练残差

    std::vector<LossElem> _rcd_loss;  // 各个样本当前的训练损失
    std::vector<LossElem>* get_rcd_loss() {
        return &_rcd_loss;
    }
    // 在每棵树训练开始的时候，根据_residues初始化_rcd_loss
    void init_loss();
    // 根据每个样本训练残差计算生成每个样本的训练损失
    void calc_rcd_loss(const std::vector<index_t>& rcds, myfloat_t prediction);
    void calc_wb_rcd_loss(const std::vector<index_t>& rcds, myfloat_t prediction);
    // 根据_residues计算一批样本的loss之和，用于fully_corractive
    LossElem calc_group_loss(const std::vector<index_t>& rcds);
    LossElem calc_wb_group_loss(const std::vector<index_t>& rcds);
    // lam_l1和lam_l2直接从Params类中获得，不作为参数传入
    static inline myfloat_t solve_l1_l2(myfloat_t y, myfloat_t w, myfloat_t& x);
    static inline myfloat_t calc_ins_loss(myfloat_t y, myfloat_t w);
    // 构造函数
    Loss(index_t rcd_num) {
        _residues.resize(rcd_num);
        _rcd_loss.resize(rcd_num);
    }

private:
    enum LossElemOperation {LOSS_ADD, LOSS_SET};

    inline void calc_LS_loss(myfloat_t weight, myfloat_t residue, LossElemOperation op_type, LossElem& elem);

    inline void calc_ModLS_loss(myfloat_t weight, myfloat_t residue, myfloat_t binary_label,
                                LossElemOperation op_type, LossElem& elem);

    inline void calc_Logistic_loss(myfloat_t weight, myfloat_t residue, myfloat_t is_inclass,
                                   LossElemOperation op_type, LossElem& elem);
    
    inline myfloat_t tag_to_binary(myfloat_t tag, myfloat_t pos_label, myfloat_t neg_label);
};


myfloat_t Loss::solve_l1_l2(myfloat_t y, myfloat_t w, myfloat_t& x)
{
    w += Params::_lam_l2 + Params::_delta;
    x = y / w;
    myfloat_t x0 = Params::_lam_l1 / w;
    x = (x > x0) ? (x - x0) : ((x < -x0) ? (x + x0) : 0.0);
    return 0.5 * w * x * x - x * y + Params::_lam_l1 * fabs(x);
}
 
myfloat_t Loss::calc_ins_loss(myfloat_t y, myfloat_t w)
{
    w += Params::_delta;
    return y * y / w;
}   

}
}

#endif
