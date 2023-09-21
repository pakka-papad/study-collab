#ifndef UPDATE_PROGRESS
#define UPDATE_PROGRESS

#include <iostream>

void updateProgressBar(int64_t progress, int64_t total, int barWidth = 50) {
    double fraction = static_cast<double>(progress) / total;
    int numChars = static_cast<int>(fraction * barWidth);

    std::cout << "[";
    for (int i = 0; i < barWidth; ++i) {
        if (i < numChars) std::cout << "=";
        else std::cout << " ";
    }
    std::cout << "] " << int(fraction * 100.0) << "%" << "\r";
    std::cout.flush();
}

#endif