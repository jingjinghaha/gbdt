#ifndef IDL_DFGBDT_DFGBDT_UTIL_H
#define IDL_DFGBDT_DFGBDT_UTIL_H

#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <algorithm>
#include <omp.h>

#include "glog/logging.h"

namespace idl {
namespace dfgbdt {

typedef float datafloat_t;
typedef double myfloat_t;
typedef int64_t index_t;  // 有符号类型
typedef int32_t keyid_t; //必须为有符号类型，因为会返回-1表示不可用

template<typename v1>
using u_set = std::unordered_set<v1>;
    
template<typename k2, typename v2>
using u_map = std::unordered_map<k2, v2>;

class FuncState
{
public:
    static const int32_t _SUCCESS;
    static const int32_t _FAIL;
};

class Params
{
public:
    static const std::string _user_config_file; // 用户配置文件路径
    static const std::string _sysm_config_file; // 系统配置文件路径

    static std::string _feat_info_file;  // 特征信息文件（HDFS）
    static std::string _train_data_file;  // 训练数据（HDFS）
    static std::string _test_data_file;   // 测试数据（HDFS）
    static std::string _disc_model_output_file;  // 离散化模型输出
    static std::string _gbdt_model_output_file; // gbdt模型输出
    static std::string _sampling_method;
    static std::string _correction_method;

    static int32_t _machine_id;  // 本机器的id
    static int32_t _thread_num;  // 并行线程数量
    static int32_t _max_bucket_num;  // 离散化中bucket数量上限
    static myfloat_t _min_bucket_weight;  // 离散化中每个bucket的最低权重。必须大于等于1.0
    static index_t _min_feat_occur_threshold;  // 每个特征出现的最少次数，否则将被过滤掉
    static int32_t _max_feat_num;  // 使用特征的数量上限
    static myfloat_t _lam_l1;
    static myfloat_t _lam_l2;
    static myfloat_t _disc_lam_l2;
    static std::string _loss_type;  // 损失类型
    static myfloat_t _min_node_weight; // 每个叶节点的最小样本权重之和
    static int32_t _max_leaf_num;  // 每棵树的最大叶节点数量
    static int32_t _max_tree_depth;  // 每棵树的最大深度
    static int32_t _tree_iteration_times;  // fully_corrective的迭代次数
    static int32_t _tree_num;  // gbdt树的数目
    static int32_t _eval_frequency; // 每隔多少颗树评估一下auc
    static int32_t _cur_tree_iteration;
    static bool _enable_fully_corrective;  // 是否启用fully_corrective
    static bool _is_sampling_flag;
    static bool _cur_sampling_flag;
    static int32_t _sampling_update_iteration;
    static myfloat_t _step_size;  // 每棵树训练结果的缩放系数
    static myfloat_t _pos_threshold;  // 正叶节点阈值
    static const myfloat_t _epsilon;
    static const float _flt_epsilon;
    static const double _dbl_epsilon;
    static double _sampling_param;
    static double _pos_prob_min;
    static double _pos_prob_max;
    static double _neg_prob_min;
    static double _neg_prob_max;
    static const myfloat_t _delta;

    static void load_params();

private:
    static void adjust_params();

    static void logging_params();
};

class HdfsParams
{
public:
    static const std::string _hdfs_config_file;

    static std::string _fs_name;
    static std::string _fs_ugi;
    static std::string _log_file;
    static int32_t _retry_num;
    static int32_t _node_id;
    static int32_t _node_num;
    static int32_t _read_thread_num;
    static int32_t _write_thread_num;
    static int32_t _block_per_line;

    static void load_hdfs_params();

private:
    static void logging_hdfs_params();
};

class Util
{
public:
/*
int32_t split_line_to_string(const char& s_char, std::string& line,
                             std::vector<std::string>& str_vec);
*/
    template <typename T>
    static std::string trans_vector_to_string(const std::vector<T>& vec, const char& sepa_char)
    {
        int32_t begin_idx = 0;
        int32_t end_idx = vec.size() - 1;
        return trans_vector_to_string(vec, sepa_char, begin_idx, end_idx);
    }

    template <typename T>   static std::string trans_vector_to_string(const std::vector<T>& vec, const char& sepa_char, 
                                              int32_t begin_idx, int32_t end_idx)
    {
        if (begin_idx > end_idx) {
            return "";
        }
        begin_idx = std::max(0, begin_idx);
        end_idx = std::min((int32_t)vec.size(), end_idx);
        std::ostringstream ss;
        for (int32_t i = begin_idx; i <= end_idx; ++i) {
            ss << vec[i] << sepa_char;
        }
        std::string res = ss.str();
        if (!res.empty()) {
            res.pop_back();
        }
        return res;
    }
};

}
}
    
#endif
