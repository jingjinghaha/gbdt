#ifndef IDL_DFGBDT_RUNNER_H
#define IDL_DFGBDT_RUNNER_H

#include "forest.h"

namespace idl {
namespace dfgbdt {

class Runner
{
public:
    // 载入训练数据，并进行离散化预处理，预处理生成离散化模型，并将处理后的数据存入TrainDataSet中
    void load_and_preprocess_train_data();
    // 训练gdbt，也就是_p_forest
    void train_gbdt();

    Runner(char* argv_0);
    
    ~Runner();

private:
    Preprocessor* _p_preprocessor;
    
    Forest* _p_forest;

    SlaveForestTrainer* _p_forest_trainer;
};

}
}

#endif
