/**
* Copyright (C) 2014-2016 All rights reserved.
* @file 
* @brief 
* @details 
* @author Weidong Huang (huangweidong01@baidu.com)
* @version 1.0
* @date 2016-11-23 15:39
*/
#ifndef IDL_DFGBDT_SAMPLING_H
#define IDL_DFGBDT_SAMPLING_H

#include "node.h"
#include "loss.h"
#include "glog/logging.h"

namespace idl {
namespace dfgbdt {
class Sampling {
public:
    static void sampling_importance_rcd(TreeNode* p_root_node,Loss* p_loss);
private:
    static void calc_sampling_prob(Loss* p_loss);
};
}
}


#endif
