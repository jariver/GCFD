#include "alcfd.h"

int main() {
    al_cfd main_process;
    main_process.run();

    std::vector<std::string> assess_lhs = {"birthplace", "city", "season=2013"};
    std::string assess_rhs = "position";
//    main_process.assess(assess_lhs, assess_rhs);
    return 0;
}