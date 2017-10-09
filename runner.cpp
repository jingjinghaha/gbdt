#include "runner.h"

#include <limits>

namespace idl {
namespace dfgbdt {

void Runner::load_and_preprocess_train_data()
{
    _p_preprocessor = new Preprocessor();
    _p_preprocessor->load_feat_info();
    int32_t global_feat_id_start = 0;
    int32_t global_feat_id_end = std::numeric_limits<int32_t>::max();

    _p_preprocessor->load_raw_train_data(global_feat_id_start, global_feat_id_end);
    _p_preprocessor->train_disc_model();
    if (!Params::_disc_model_output_file.empty()) {
        _p_preprocessor->write_disc_model_to_file(Params::_disc_model_output_file);
    }
    _p_preprocessor->apply_disc_model();
    _p_preprocessor->clear_raw_train_data();

    _p_preprocessor->load_raw_test_data();
    return;
}

void Runner::train_gbdt()
{
    _p_forest = new Forest();
    _p_forest_trainer = new SlaveForestTrainer(_p_forest);
    _p_forest_trainer->train_forest();
    if (!Params::_gbdt_model_output_file.empty()) {
        _p_forest->write_forest_to_file(Params::_gbdt_model_output_file);
    }
    return;
}

Runner::Runner(char* argv_0)
{
    FLAGS_log_dir = "../log/";
    google::InitGoogleLogging(argv_0);
    google::InstallFailureSignalHandler();
    google::SetStderrLogging(google::INFO);
    FLAGS_logbufsecs = 0;

    Params::load_params();
    HdfsParams::load_hdfs_params();

    // 初始化训练和预测数据集
    TrainDataSet::get_data_ptr();
    TestDataSet::get_data_ptr();

    _p_preprocessor = nullptr;
    _p_forest = nullptr;
    _p_forest_trainer = nullptr;
}

Runner::~Runner()
{
    delete _p_preprocessor;
    _p_preprocessor = nullptr;
    delete _p_forest;
    _p_forest = nullptr;
    delete _p_forest_trainer;
    _p_forest_trainer = nullptr;
    google::ShutdownGoogleLogging();
}

}
}

int main(int32_t argc, char** argv)
{
    idl::dfgbdt::Runner runner(argv[0]);
    runner.load_and_preprocess_train_data();
    runner.train_gbdt();
    return 0;
}
