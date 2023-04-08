#include "input_reader.h"
#include "stat_reader.h"

#include <iostream>

using namespace transport;

int main() {
    Catalogue catalogue;
    
    DataReader(std::cin, catalogue);
    
    DataOutput(std::cout, catalogue);
    
    return 0;
}