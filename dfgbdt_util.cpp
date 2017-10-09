#include "dfgbdt_util.h"
#include "external/config.h"

#include <limits>

namespace idl {
namespace dfgbdt {

const int32_t FuncState::_SUCCESS = 0;
const int32_t FuncState::_FAIL = 1;

/** Params成员变量初始化 **/
const std::string Params::_user_config_file = "../conf/user_conf.yaml";
const std::string Params::_sysm_config_file = "../conf/sysm_conf.yaml";
const myfloat_t Params::_epsilon = 1e-6;
const float Params::_flt_epsilon = 1e-6;
const double Params::_dbl_epsilon = 1e-6;
const myfloat_t Params::_delta = 1e-10;

std::string Params::_feat_info_file;
std::string Params::_train_data_file;
std::string Params::_test_data_file;
std::string Params::_disc_model_output_file;
std::string Params::_gbdt_model_output_file;
std::string Params::_sampling_method;
std::string Params::_correction_method;

int32_t Params::_machine_id;
int32_t Params::_thread_num;
int32_t Params::_max_bucket_num;
myfloat_t Params::_min_bucket_weight;
index_t Params::_min_feat_occur_threshold;
int32_t Params::_max_feat_num;
myfloat_t Params::_lam_l1;
myfloat_t Params::_lam_l2;
myfloat_t Params::_disc_lam_l2;
std::string Params::_loss_type;
myfloat_t Params::_min_node_weight;
int32_t Params::_max_leaf_num;
int32_t Params::_max_tree_depth;
int32_t Params::_cur_tree_iteration = 0;
int32_t Params::_tree_iteration_times;
int32_t Params::_tree_num;
int32_t Params::_eval_frequency;
int32_t Params::_sampling_update_iteration;
bool Params::_enable_fully_corrective;
bool Params::_is_sampling_flag;
bool Params::_cur_sampling_flag = false;
myfloat_t Params::_step_size;
myfloat_t Params::_pos_threshold;
double Params::_sampling_param;
myfloat_t Params::_pos_prob_max;
myfloat_t Params::_pos_prob_min;
myfloat_t Params::_neg_prob_max;
myfloat_t Params::_neg_prob_min;

const std::string HdfsParams::_hdfs_config_file = "../conf/hdfs_conf.yaml";

std::string HdfsParams::_fs_name;
std::string HdfsParams::_fs_ugi;
std::string HdfsParams::_log_file;
int32_t HdfsParams::_retry_num;
int32_t HdfsParams::_node_id;
int32_t HdfsParams::_node_num;
int32_t HdfsParams::_read_thread_num;
int32_t HdfsParams::_write_thread_num;
int32_t HdfsParams::_block_per_line;

void Params::load_params()
{
    LOG(INFO) << "Enter Params::load_params()";
    bdl::luna::Configure sysm_cfg;
    sysm_cfg.init(_sysm_config_file);
    bdl::luna::Configure user_cfg;
    user_cfg.init(_user_config_file);

    double double_cache = 0.0;
    int32_t int_cache = 0;
    sysm_cfg.get_int_by_name("mahine_id", _machine_id);
    sysm_cfg.get_int_by_name("thread_num", _thread_num);
    
    user_cfg.get_string_by_name("feat_info_file", _feat_info_file);
    user_cfg.get_string_by_name("train_data_file", _train_data_file);
    user_cfg.get_string_by_name("test_data_file", _test_data_file);
    user_cfg.get_string_by_name("disc_model_output_file", _disc_model_output_file);
    user_cfg.get_string_by_name("gbdt_model_output_file", _gbdt_model_output_file);
    user_cfg.get_int_by_name("max_bucket_num", _max_bucket_num);
    user_cfg.get_double_by_name("min_bucket_weight", double_cache);
    _min_bucket_weight = double_cache;
    // 此处应当以int64_t的形式读取。暂时先用int
    user_cfg.get_int_by_name("min_feat_occur_threshold", int_cache);
    _min_feat_occur_threshold = int_cache;
    user_cfg.get_int_by_name("max_feat_num", _max_feat_num);
    user_cfg.get_double_by_name("lambda_l1", double_cache);
    _lam_l1 = double_cache;
    user_cfg.get_double_by_name("lambda_l2", double_cache);
    _lam_l2 = double_cache;
    user_cfg.get_double_by_name("disc_lambda_l2", double_cache);
    _disc_lam_l2 = double_cache;
    user_cfg.get_string_by_name("loss_type", _loss_type);
    user_cfg.get_string_by_name("sampling_method", _sampling_method);
    user_cfg.get_string_by_name("correction_method", _correction_method);
    user_cfg.get_double_by_name("min_node_weight", double_cache);
    _min_node_weight = double_cache;
    user_cfg.get_int_by_name("max_leaf_num", _max_leaf_num);
    user_cfg.get_int_by_name("max_tree_depth", _max_tree_depth);
    user_cfg.get_int_by_name("tree_iteration_times", _tree_iteration_times);
    user_cfg.get_int_by_name("tree_num", _tree_num);
    user_cfg.get_int_by_name("eval_frequency", _eval_frequency);
    user_cfg.get_int_by_name("sampling_update_iteration", _sampling_update_iteration);
    user_cfg.get_bool_by_name("enable_fully_corrective", _enable_fully_corrective);
    user_cfg.get_double_by_name("step_size", double_cache);
    user_cfg.get_double_by_name("sampling_param", _sampling_param);
    user_cfg.get_double_by_name("pos_prob_max", _pos_prob_max);
    user_cfg.get_double_by_name("pos_prob_min", _pos_prob_min);
    user_cfg.get_double_by_name("neg_prob_max", _neg_prob_max);
    user_cfg.get_double_by_name("neg_prob_min", _neg_prob_min);
    user_cfg.get_bool_by_name("is_sampling_flag", _is_sampling_flag);
    
    _step_size = double_cache;
    user_cfg.get_double_by_name("positive_threshold", double_cache);
    _pos_threshold = double_cache;
    LOG(INFO) << "loading params finished.";
    adjust_params();
    LOG(INFO) << "adjusting params finished.";
    logging_params();
    return;
}

void Params::adjust_params()
{
    if (_thread_num < 1) {
        _thread_num = omp_get_num_procs();
    }
    _max_bucket_num = std::max(_max_bucket_num, 1);
    _min_bucket_weight = std::max(_min_bucket_weight, (myfloat_t)1.0);
    if (_max_feat_num <= 0) {
        _max_feat_num = std::numeric_limits<int32_t>::max();
    }
    if ("LS" != _loss_type && "MODLS" != _loss_type && "LOGISTIC" != _loss_type) {
        LOG(FATAL) << "Unknow loss_type: " << _loss_type;
    }
    _max_leaf_num = std::max(_max_leaf_num, 1);
    return;
}

void Params::logging_params()
{
    LOG(INFO) << "machine_id: " << _machine_id;
    LOG(INFO) << "thread_num: " << _thread_num;
    LOG(INFO) << "max_bucket_num: " << _max_bucket_num;
    LOG(INFO) << "min_bucket_weight: " << _min_bucket_weight;
    LOG(INFO) << "min_feat_occur_threshold: " << _min_feat_occur_threshold;
    LOG(INFO) << "max_feat_num: " << _max_feat_num;
    LOG(INFO) << "lambda_l1: " << _lam_l1;
    LOG(INFO) << "lambda_l2: " << _lam_l2;
    LOG(INFO) << "disc_lambda_l2: " << _disc_lam_l2;
    LOG(INFO) << "loss_type: " << _loss_type;
    LOG(INFO) << "min_node_weight: " << _min_node_weight;
    LOG(INFO) << "sampling_flag" << _is_sampling_flag;
    LOG(INFO) << "sampling_param" << _sampling_param;
    LOG(INFO) << "pos_prob_max" << _pos_prob_max;
    LOG(INFO) << "pos_prob_min" << _pos_prob_min;
    LOG(INFO) << "neg_prob_max" << _neg_prob_max;
    LOG(INFO) << "neg_prob_min" << _neg_prob_min;
    LOG(INFO) << "sampling_method" << _sampling_method;
    LOG(INFO) << "correction_method" << _correction_method;
    LOG(INFO) << "max_leaf_num: " << _max_leaf_num;
    LOG(INFO) << "max_tree_depth: " << _max_tree_depth;
    LOG(INFO) << "tree_iteration_times: " << _tree_iteration_times;
    LOG(INFO) << "tree_num: " << _tree_num; 
    LOG(INFO) << "sampling_update_iteration: " << _sampling_update_iteration; 
    LOG(INFO) << "enable_fully_corrective: " << _enable_fully_corrective;
    LOG(INFO) << "step_size: " << _step_size;
    LOG(INFO) << "pos_threshold: " << _pos_threshold;
    return; 
}

void HdfsParams::load_hdfs_params()
{
    bdl::luna::Configure hdfs_cfg;
    hdfs_cfg.init(_hdfs_config_file);

    hdfs_cfg.get_string_by_name("fs_name", _fs_name);
    hdfs_cfg.get_string_by_name("fs_ugi", _fs_ugi);
    hdfs_cfg.get_string_by_name("log_file", _log_file);
    hdfs_cfg.get_int_by_name("retry_num", _retry_num);
    hdfs_cfg.get_int_by_name("machine_id", _node_id);
    hdfs_cfg.get_int_by_name("machine_num", _node_num);
    hdfs_cfg.get_int_by_name("read_thread_num", _read_thread_num);
    hdfs_cfg.get_int_by_name("write_thread_num", _write_thread_num);
    hdfs_cfg.get_int_by_name("block_per_line", _block_per_line);

    logging_hdfs_params();
    return;
}

void HdfsParams::logging_hdfs_params()
{
    LOG(INFO) << "fs_name: " << _fs_name;
    LOG(INFO) << "fs_ugi: " << _fs_ugi;
    LOG(INFO) << "node_id: " << _node_id;
    LOG(INFO) << "node_num: " << _node_num;
    LOG(INFO) << "block_per_line: " << _block_per_line;
    return;
}

/*
int32_t Util::split_line_to_string(const char& s_char, std::string& line,
                                   std::vector<std::string>& str_vec)
{
    str_vec.clear();
    if (line.empty()) {
        return FuncState::_FAIL;
    }
    if (Params::newline_char == line[line.size() - 1]) {
        line.erase(line.size() - 1, 1);
    }
    line.insert(0, 1, s_char);
    line.push_back(s_char);
    int32_t begin_idx = 0;
    int32_t end_idx = begin_idx + 1;
    bool all_empty = true;
    while (end_idx < line.size() - 1) {
        while (s_char != line[end_idx]) {
            ++end_idx;
        }
        std::string spatch(line, begin_idx + 1, end_idx - begin_idx - 1);
        if (!spatch.empty()) {
            all_empty = false;
        }
        str_vec.push_back(std::move(spatch));

        begin_idx = end_idx;
        ++end_idx;
    }
    return all_empty ? FuncState::_FAIL : FuncState::_SUCCESS;
}
*/

}
}
