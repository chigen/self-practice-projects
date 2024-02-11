// clang-format off
//
// Created by goksu on 4/6/19.
//

#include <algorithm>
#include <vector>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>


rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions)
{
    auto id = get_next_id();
    pos_buf.emplace(id, positions);

    return {id};
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices)
{
    auto id = get_next_id();
    ind_buf.emplace(id, indices);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f> &cols)
{
    auto id = get_next_id();
    col_buf.emplace(id, cols);

    return {id};
}   

auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
{
    return Vector4f(v3.x(), v3.y(), v3.z(), w);
}


static bool insideTriangle(int x, int y, const Vector3f* _v)
{   
    // check if the point (x, y) is inside the triangle represented by _v[0], _v[1], _v[2]
    Eigen::Vector3f P(x, y, 0);

    Eigen::Vector3f cross_AB_P = (_v[1] - _v[0]).cross(P - _v[0]);
    Eigen::Vector3f cross_BC_P = (_v[2] - _v[1]).cross(P - _v[1]);
    Eigen::Vector3f cross_CA_P = (_v[0] - _v[2]).cross(P - _v[2]);
    // check if these 3 cross products have the same sign
    return (cross_AB_P.z()>=0 == cross_BC_P.z()>=0) && (cross_BC_P.z()>=0 == cross_CA_P.z()>=0);
}

static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector3f* v)
{
    float c1 = (x*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*y + v[1].x()*v[2].y() - v[2].x()*v[1].y()) / (v[0].x()*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*v[0].y() + v[1].x()*v[2].y() - v[2].x()*v[1].y());
    float c2 = (x*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*y + v[2].x()*v[0].y() - v[0].x()*v[2].y()) / (v[1].x()*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*v[1].y() + v[2].x()*v[0].y() - v[0].x()*v[2].y());
    float c3 = (x*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*y + v[0].x()*v[1].y() - v[1].x()*v[0].y()) / (v[2].x()*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*v[2].y() + v[0].x()*v[1].y() - v[1].x()*v[0].y());
    return {c1,c2,c3};
}

void rst::rasterizer::draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, col_buf_id col_buffer, Primitive type, bool superSampling)
{
    auto& buf = pos_buf[pos_buffer.pos_id];
    auto& ind = ind_buf[ind_buffer.ind_id];
    auto& col = col_buf[col_buffer.col_id];

    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

    Eigen::Matrix4f mvp = projection * view * model;
    for (auto& i : ind)
    {
        Triangle t;
        Eigen::Vector4f v[] = {
                mvp * to_vec4(buf[i[0]], 1.0f),
                mvp * to_vec4(buf[i[1]], 1.0f),
                mvp * to_vec4(buf[i[2]], 1.0f)
        };
        //Homogeneous division
        for (auto& vec : v) {
            vec /= vec.w();
        }
        //Viewport transformation
        for (auto & vert : v)
        {
            vert.x() = 0.5*width*(vert.x()+1.0);
            vert.y() = 0.5*height*(vert.y()+1.0);
            vert.z() = vert.z() * f1 + f2;
        }

        for (int i = 0; i < 3; ++i)
        {
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
            t.setVertex(i, v[i].head<3>());
        }

        auto col_x = col[i[0]];
        auto col_y = col[i[1]];
        auto col_z = col[i[2]];

        t.setColor(0, col_x[0], col_x[1], col_x[2]);
        t.setColor(1, col_y[0], col_y[1], col_y[2]);
        t.setColor(2, col_z[0], col_z[1], col_z[2]);
        rasterize_triangle(t, superSampling);
    }
    if (superSampling){
        downsample();
    }
}

//Screen space rasterization
void rst::rasterizer::rasterize_triangle(const Triangle& t, bool superSampling) {
    auto v = t.toVector4();
    
    // Find out the bounding box of current triangle.
    float aabb_min_x = std::min(v[0].x(), std::min(v[1].x(), v[2].x()));
    float aabb_min_y = std::min(v[0].y(), std::min(v[1].y(), v[2].y()));
    float aabb_max_x = std::max(v[0].x(), std::max(v[1].x(), v[2].x()));
    float aabb_max_y = std::max(v[0].y(), std::max(v[1].y(), v[2].y()));

    // Super-sampling: 2x2 grid
    // iterate through the pixel and find if the current pixel is inside the triangle
    if (superSampling){
        for (int x=2*aabb_min_x; x<=2*aabb_max_x; ++x){
            for (int y=2*aabb_min_y; y<=2*aabb_max_y; ++y){
                float ss_x = x / 2.0;
                float ss_y = y / 2.0;
                if (!insideTriangle(ss_x, ss_y, t.v)){
                    continue;
                }
                // compute the interpolated z value
                auto[alpha, beta, gamma] = computeBarycentric2D(ss_x, ss_y, t.v);
                float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                z_interpolated *= w_reciprocal;
                // original get_index: (height-1-y)*width + x
                int buf_index = (2*height-1-y)*2*width + x;
                if (z_interpolated < ss_depth_buf[buf_index]){
                    ss_depth_buf[buf_index] = z_interpolated;
                    ss_frame_buf[buf_index] = t.getColor();
                }
            }
        }
    }
    else{
        for (int x=aabb_min_x; x<=aabb_max_x; ++x){
            for (int y=aabb_min_y; y<=aabb_max_y; ++y){
                if (!insideTriangle(x, y, t.v)){
                    continue;
                }
                // compute the interpolated z value
                auto[alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
                float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                z_interpolated *= w_reciprocal;

                int buf_index = get_index(x, y);
                if (z_interpolated < depth_buf[buf_index]){
                    depth_buf[buf_index] = z_interpolated;
                    set_pixel(Eigen::Vector3f(x, y, 0), t.getColor());
                }
            }
        }
    }
    
    // If so, use the following code to get the interpolated z value.
    //auto[alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
    //float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
    //float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
    //z_interpolated *= w_reciprocal;

    // TODO : set the current pixel (use the set_pixel function) to the color of the triangle (use getColor function) if it should be painted.
}

void rst::rasterizer::downsample(){
    for (int x=0; x<width; ++x){
        for (int y=0; y<height; ++y){
            int buf_index = get_index(x, y);
            Eigen::Vector3f color_sum = Eigen::Vector3f(0, 0, 0);
            for (int subX=0; subX<2; ++subX){
                for (int subY=0; subY<2; ++subY){
                    int ss_buf_index = (2*height-1-2*y-subY)*2*width + 2*x+subX;
                    color_sum += ss_frame_buf[ss_buf_index];
                }
            }
            color_sum /= 4;
            set_pixel(Eigen::Vector3f(x, y, 0), color_sum);
        }
    }
}

void rst::rasterizer::set_model(const Eigen::Matrix4f& m)
{
    model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f& v)
{
    view = v;
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f& p)
{
    projection = p;
}

void rst::rasterizer::clear(rst::Buffers buff)
{
    if ((buff & rst::Buffers::Color) == rst::Buffers::Color)
    {
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{0, 0, 0});
        std::fill(ss_frame_buf.begin(), ss_frame_buf.end(), Eigen::Vector3f{0, 0, 0});
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
        std::fill(ss_depth_buf.begin(), ss_depth_buf.end(), std::numeric_limits<float>::infinity());
    }
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h)
{
    frame_buf.resize(w * h);
    ss_frame_buf.resize(2*w * 2*h);
    depth_buf.resize(w * h);
    ss_depth_buf.resize(2*w * 2*h);
}

int rst::rasterizer::get_index(int x, int y)
{
    return (height-1-y)*width + x;
}

void rst::rasterizer::set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color)
{
    //old index: auto ind = point.y() + point.x() * width;
    auto ind = (height-1-point.y())*width + point.x();
    frame_buf[ind] = color;

}

// clang-format on
