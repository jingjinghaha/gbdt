#include <data.h>

namespace idl {
namespace dfgbdt {

TrainDataSet* TrainDataSet::_s_p_instance = nullptr;

TestDataSet* TestDataSet::_s_p_instance = nullptr;

void TrainDataSet::save_signed_data_to_file(const std::string& file_name)
{
    std::ofstream fout(file_name);
    for (index_t idx = 0; idx < size(); ++idx) {
        fout << _tags[idx];
        for (const auto& p : _signed_data[idx]) {
            fout << " " << p.first << ":" << p.second;
        }
        fout << std::endl;
    }
    fout.close();
    return;
}

}
}
