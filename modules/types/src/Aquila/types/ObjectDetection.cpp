#include "Aquila/types/ObjectDetection.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
aq::Classification::Classification(const std::string& label_, float confidence_, int classNumber_) :
    label(label_), confidence(confidence_), classNumber(classNumber_)
{

}

void aq::CreateColormap(cv::Mat& lut, int num_classes, int ignore_class)
{
    lut.create(1, num_classes, CV_8UC3);
    for(int i = 0; i < num_classes; ++i)
        lut.at<cv::Vec3b>(i) = cv::Vec3b(i*180 / num_classes, 200, 255);
    cv::cvtColor(lut, lut, cv::COLOR_HSV2BGR);
    if(ignore_class != -1 && ignore_class < num_classes)
        lut.at<cv::Vec3b>(ignore_class) = cv::Vec3b(0,0,0);
}
