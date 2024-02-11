#include "Triangle.hpp"
#include "rasterizer.hpp"
#include <Eigen/Eigen>
#include <iostream>
#include <opencv2/opencv.hpp>

constexpr double MY_PI = 3.1415926;
inline float deg2rad(float degrees) { return degrees * MY_PI / 180.0; }

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();
    /* 
    1, 0, 0, -eye_pos[0],
    0, 1, 0, -eye_pos[1],
    0, 0, 1, -eye_pos[2],
    0, 0, 0, 1
     */
    Eigen::Matrix4f translate;
    translate << 1, 0, 0, -eye_pos[0], 0, 1, 0, -eye_pos[1], 0, 0, 1,
        -eye_pos[2], 0, 0, 0, 1;

    view = translate * view;

    return view;
}

Eigen::Matrix4f get_model_matrix(float rotation_angle)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f rotate;
    /*
    Rotate the triangle around the Z axis:
    cos(theta), -sin(theta), 0, 0,
    sin(theta), cos(theta), 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
    */
    rotate << cos(rotation_angle / 180 * MY_PI), -sin(rotation_angle / 180 * MY_PI),
        0, 0, sin(rotation_angle / 180 * MY_PI),
        cos(rotation_angle / 180 * MY_PI), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1;
    
    model = rotate * model;

    return model;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio,
                                      float zNear, float zFar)
{
    // Students will implement this function

    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();

    /*
    Perspective projection:
    1/(tan(fov/2)*aspect), 0, 0, 0,
    0, 1/tan((fov/2)), 0, 0,
    0, 0, (zNear+zFar)/(zNear-zFar), 2*zNear*zFar/(zNear-zFar),
    0, 0, -1, 0
    */
    projection << 1 / (tan(deg2rad(eye_fov) / 2) * aspect_ratio), 0, 0, 0,
        0, 1 / tan(deg2rad(eye_fov) / 2), 0, 0, 
        0, 0, (zNear + zFar) / (zNear - zFar), 2 * zNear * zFar / (zNear - zFar),
        0, 0, -1, 0;

    return projection;
}

Eigen::Matrix4f get_rotation(Vector3f axis, float angle)
{
    Eigen::Matrix4f rotation;
    // Using quaternion to calculate the rotation matrix
    // Eigen::Quaternionf q;
    // q = Eigen::AngleAxisf(angle, axis);
    // rotation = q.toRotationMatrix();
    return rotation;
}

int main(int argc, const char** argv)
{
    float angle = 0;
    bool command_line = false;
    std::string filename = "output.png";

    if (argc >= 3) {
        command_line = true;
        angle = std::stof(argv[2]); // -r by default
        if (argc == 4) {
            filename = std::string(argv[3]);
        }
    }

    rst::rasterizer r(700, 700);

    Eigen::Vector3f eye_pos = {0, 0, 5};

    std::vector<Eigen::Vector3f> pos{{2, 0, -2}, {0, 2, -2}, {-2, 0, -2}};

    std::vector<Eigen::Vector3i> ind{{0, 1, 2}};

    auto pos_id = r.load_positions(pos);
    auto ind_id = r.load_indices(ind);

    int key = 0;
    int frame_count = 0;

    if (command_line) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);

        cv::imwrite(filename, image);

        return 0;
    }

    while (key != 27) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);

        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::imshow("image", image);
        key = cv::waitKey(10);

        std::cout << "frame count: " << frame_count++ << '\n';

        if (key == 'a') {
            angle += 10;
        }
        else if (key == 'd') {
            angle -= 10;
        }
    }

    return 0;
}
