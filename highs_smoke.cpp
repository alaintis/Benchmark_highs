#include "Highs.h"
#include <iostream>

int main() {
    Highs h;
    h.setOptionValue("log_to_console", true);
    h.run();   // empty model
    std::cout << "HiGHS ran successfully\n";
    return 0;
}