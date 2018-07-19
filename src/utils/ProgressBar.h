#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H
#include <vector>
#include <iostream>
#include <sstream>
#include <string>

class ProgressBar
{
     friend std::ostream& operator<<(std::ostream& out, const ProgressBar& pbar){
        out << pbar.bar.str();
        return out;
    }
public:
    ProgressBar(const int& step, const int& total, const std::string& info){
        int barWidth = 60;
        float progress = ((float) (step) + 1.01) / (total-1);
        bar << "[";
        int pos = barWidth * progress;
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) bar <<"=";
            else if (i == pos) bar <<">";
            else bar << " ";
        }
        bar << "] " << int(progress * 100.0) << " %" << info << "\r"; 
    }
    std::stringstream bar;
};


#endif // PROGRESS_BAR_H

