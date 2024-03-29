#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>

std::vector<cv::Point2f> control_points;

void mouse_handler(int event, int x, int y, int flags, void *userdata) 
{
    if (event == cv::EVENT_LBUTTONDOWN && control_points.size() < 4) 
    {
        std::cout << "Left button of the mouse is clicked - position (" << x << ", "
        << y << ")" << '\n';
        control_points.emplace_back(x, y);
    }     
}

void naive_bezier(const std::vector<cv::Point2f> &points, cv::Mat &window) 
{
    auto &p_0 = points[0];
    auto &p_1 = points[1];
    auto &p_2 = points[2];
    auto &p_3 = points[3];

    for (double t = 0.0; t <= 1.0; t += 0.001) 
    {
        auto point = std::pow(1 - t, 3) * p_0 + 3 * t * std::pow(1 - t, 2) * p_1 +
                 3 * std::pow(t, 2) * (1 - t) * p_2 + std::pow(t, 3) * p_3;

        window.at<cv::Vec3b>(point.y, point.x)[2] = 255;
    }
}

cv::Point2f lerping(const cv::Point2f &a, const cv::Point2f &b, float t){
    return a + t*(b-a);
}

cv::Point2f recursive_bezier(const std::vector<cv::Point2f> &control_points, float t) 
{
    // TODO: Implement de Casteljau's algorithm
    if (control_points.size() == 1){
        return control_points[0];
    }

    std::vector<cv::Point2f> temp_points;
    for (int i=1; i<control_points.size(); ++i){
        temp_points.push_back(lerping(control_points[i-1], control_points[i], t));
    }
    return recursive_bezier(temp_points, t);
}

void draw_pixel(cv::Mat &window, const cv::Point2f &point, const cv::Vec3b &color, float alpha) {
    int x = std::floor(point.x);
    int y = std::floor(point.y);

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int newX = x + dx;
            int newY = y + dy;

            if (newX >= 0 && newY >= 0 && newX < window.cols && newY < window.rows) {
                float dist = sqrt(pow(newX - point.x, 2) + pow(newY - point.y, 2));
                float influence = std::max(0.0f, 1.0f - dist);

                cv::Vec3b &pixel = window.at<cv::Vec3b>(newY, newX);
                for (int i = 0; i < 3; i++) {
                    pixel[i] = cv::saturate_cast<uchar>(pixel[i] + alpha * influence * color[i]);
                }
            }
        }
    }
}

void bezier(const std::vector<cv::Point2f> &control_points, cv::Mat &window) 
{
    // TODO: Iterate through all t = 0 to t = 1 with small steps, and call de Casteljau's 
    // recursive Bezier algorithm.
    cv::Vec3b color = {0, 255, 0}; 
    for(float t=0.0; t<=1.0; t+=0.001){
        auto point = recursive_bezier(control_points, t);
        // solution1
        // window.at<cv::Vec3b>(point.y, point.x)[1] = 255;

        draw_pixel(window, point, color, 0.5); 

        // solution2
        // int xmin = std::floor(point.x);
        // int ymin = std::floor(point.y);
        // float maxDis = 1.06066017f;
        // float color0 = 0;
        // float color1 = 0;
        // float color2 = 0;
        // float color3 = 0;
        // int colorValue = 0;
        // color0 = 255 *  (maxDis - std::sqrt( std::pow(point.x-xmin-0.25f,2)+std::pow(point.y-ymin-0.25f,2) )) / maxDis;
        // color1 = 255 *  (maxDis - std::sqrt( std::pow(point.x-xmin-0.25f,2)+std::pow(point.y-ymin-0.75f,2) )) / maxDis;
        // color2 = 255 *  (maxDis - std::sqrt( std::pow(point.x-xmin-0.75f,2)+std::pow(point.y-ymin-0.25f,2) )) / maxDis;
        // color3 = 255 *  (maxDis - std::sqrt( std::pow(point.x-xmin-0.75f,2)+std::pow(point.y-ymin-0.75f,2) )) / maxDis;
        // colorValue = (color0 + color1+color2+color3 )/4.0f;
        // window.at<cv::Vec3b>(point.y, point.x)[1] = std::min(255,colorValue + window.at<cv::Vec3b>(point.y, point.x)[1]);//set the color to green
    }

}

int main() 
{
    cv::Mat window = cv::Mat(700, 700, CV_8UC3, cv::Scalar(0));
    cv::cvtColor(window, window, cv::COLOR_BGR2RGB);
    cv::namedWindow("Bezier Curve", cv::WINDOW_AUTOSIZE);

    cv::setMouseCallback("Bezier Curve", mouse_handler, nullptr);

    int key = -1;
    while (key != 27) 
    {
        for (auto &point : control_points) 
        {
            cv::circle(window, point, 3, {255, 255, 255}, 3);
        }

        if (control_points.size() == 4) 
        {
            naive_bezier(control_points, window);
            bezier(control_points, window);

            // cv::imshow("Bezier Curve", window);
            cv::imwrite("my_bezier_curve.png", window);
            key = cv::waitKey(0);

            return 0;
        }

        cv::imshow("Bezier Curve", window);
        key = cv::waitKey(20);
    }

return 0;
}
