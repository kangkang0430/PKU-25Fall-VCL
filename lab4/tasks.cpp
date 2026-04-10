#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <spdlog/spdlog.h>
#include <iostream>
#include "Labs/4-Animation/tasks.h"
#include "IKSystem.h"
#include "CustomFunc.inl"
#include <fstream>
#include "stb_image.h"
#include "stb_image_write.h"


namespace VCX::Labs::Animation {
    void ForwardKinematics(IKSystem & ik, int StartIndex) {
        if (StartIndex == 0) {
            ik.JointGlobalRotation[0] = ik.JointLocalRotation[0];
            ik.JointGlobalPosition[0] = ik.JointLocalOffset[0];
            StartIndex                = 1;
        }
        
        for (int i = StartIndex; i < ik.JointLocalOffset.size(); i++) {
            // your code here: forward kinematics, update JointGlobalPosition and JointGlobalRotation
            ik.JointGlobalPosition[i] = ik.JointGlobalPosition[i - 1] + ik.JointGlobalRotation[i - 1] * ik.JointLocalOffset[i];
            ik.JointGlobalRotation[i] = glm::normalize(ik.JointGlobalRotation[i - 1] * ik.JointLocalRotation[i]);
        }
    }

    void InverseKinematicsCCD(IKSystem & ik, const glm::vec3 & EndPosition, int maxCCDIKIteration, float eps) {
        ForwardKinematics(ik, 0);
        // These functions will be useful: glm::normalize, glm::rotation, glm::quat * glm::quat
        for (int CCDIKIteration = 0; CCDIKIteration < maxCCDIKIteration && glm::l2Norm(ik.EndEffectorPosition() - EndPosition) > eps; CCDIKIteration++) {
            // your code here: ccd ik
            int n = ik.NumJoints();
            for (int i = n - 2; i >= 0; i--) {
                glm::vec3 jointPos = ik.JointGlobalPosition[i];
                glm::vec3 currentEndPos = ik.EndEffectorPosition();
                
                glm::vec3 toCurrentEnd  = glm::normalize(currentEndPos - jointPos);
                glm::vec3 toTarget      = glm::normalize(EndPosition - jointPos);
                glm::quat rotation_quat = glm::rotation(toCurrentEnd, toTarget);
                ik.JointLocalRotation[i]  = glm::normalize(rotation_quat * ik.JointLocalRotation[i]);
                ForwardKinematics(ik, i);
            } 
        }
    }

    void InverseKinematicsFABR(IKSystem & ik, const glm::vec3 & EndPosition, int maxFABRIKIteration, float eps) {
        ForwardKinematics(ik, 0);
        int nJoints = ik.NumJoints();
        std::vector<glm::vec3> backward_positions(nJoints, glm::vec3(0, 0, 0)), forward_positions(nJoints, glm::vec3(0, 0, 0));
        for (int IKIteration = 0; IKIteration < maxFABRIKIteration && glm::l2Norm(ik.EndEffectorPosition() - EndPosition) > eps; IKIteration++) {
            // task: fabr ik
            // backward update
            glm::vec3 next_position         = EndPosition;
            backward_positions[nJoints - 1] = EndPosition;

            for (int i = nJoints - 2; i >= 0; i--) {
                // your code here
                glm::vec3 dir = glm::normalize(ik.JointGlobalPosition[i] - next_position);
                float length = glm::l2Norm(ik.JointLocalOffset[i + 1]);
                backward_positions[i] = next_position + dir * length;
                next_position = backward_positions[i];  
            }

            // forward update
            glm::vec3 now_position = ik.JointGlobalPosition[0];
            forward_positions[0] = ik.JointGlobalPosition[0];
            for (int i = 0; i < nJoints - 1; i++) {
                // your code here
                glm::vec3 dir = glm::normalize(backward_positions[i + 1] - now_position);
                float length = glm::l2Norm(ik.JointLocalOffset[i + 1]);
                forward_positions[i + 1] = now_position + dir * length;
                now_position = forward_positions[i + 1];
            }
            ik.JointGlobalPosition = forward_positions; // copy forward positions to joint_positions
        }

        // Compute joint rotation by position here.
        for (int i = 0; i < nJoints - 1; i++) {
            ik.JointGlobalRotation[i] = glm::rotation(glm::normalize(ik.JointLocalOffset[i + 1]), glm::normalize(ik.JointGlobalPosition[i + 1] - ik.JointGlobalPosition[i]));
        }
        ik.JointLocalRotation[0] = ik.JointGlobalRotation[0];
        for (int i = 1; i < nJoints - 1; i++) {
            ik.JointLocalRotation[i] = glm::inverse(ik.JointGlobalRotation[i - 1]) * ik.JointGlobalRotation[i];
        }
        ForwardKinematics(ik, 0);
    }


    struct SimpleImage {
        int width, height;
        std::vector<uint8_t> pixels; // 灰度值 0-255
    
        SimpleImage(int w, int h) : width(w), height(h), pixels(w * h, 0) {}
    
        uint8_t& at(int x, int y) { return pixels[y * width + x]; }
        const uint8_t& at(int x, int y) const { return pixels[y * width + x]; }
    };

    SimpleImage CreateTestImage() {
        // 创建一个简单的字母 "A" 的测试图像
        SimpleImage image(20, 20);
    
        for (int y = 0; y < 20; y++) {
            for (int x = 0; x < 20; x++) {
                // 左斜线
                if (x == y / 2) image.at(x, y) = 255;
                // 右斜线  
                if (x == 20 - y / 2) image.at(x, y) = 255;
                // 横线
                if (y == 10 && x > 5 && x < 15) image.at(x, y) = 255;
            }
        }
        return image;
    }

    SimpleImage LoadImageFile(const std::string& filename) {
        int width, height, channels;
        unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 0);
    
        if (!data) {
            spdlog::error("无法加载图像: {}", filename);
            return CreateTestImage();
        }
    
        SimpleImage image(width, height);
    
        // 转换为灰度图
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int index = (y * width + x) * channels;
                if (channels == 1) {
                    image.at(x, y) = data[index];
                } 
                else {
                    unsigned char r = data[index];
                    unsigned char g = data[index + 1];
                    unsigned char b = data[index + 2];
                    image.at(x, y) = static_cast<uint8_t>(0.299f * r + 0.587f * g + 0.114f * b);
                }
            }
        }
    
        stbi_image_free(data);
        return image;
    }

    
    SimpleImage ExtractSkeleton(const SimpleImage& input) {
        SimpleImage skeleton(input.width, input.height);
        SimpleImage temp = input;
    
        // 添加安全计数器，防止无限循环
        int maxIterations = 30;  // 最大迭代次数
        int iteration = 0;
    
        bool changed;
        do {
            changed = false;
            iteration++;
        
            SimpleImage marker(input.width, input.height);
        
            // 第一轮：标记要删除的点
            for (int y = 1; y < input.height - 1; y++) {
                for (int x = 1; x < input.width - 1; x++) {
                    if (temp.at(x, y) < 128) continue; // 使用128作为阈值，而不是0
                
                    // 获取8邻域（使用128作为阈值）
                    int p2 = temp.at(x, y-1) > 128 ? 1 : 0;
                    int p3 = temp.at(x+1, y-1) > 128 ? 1 : 0;
                    int p4 = temp.at(x+1, y) > 128 ? 1 : 0;
                    int p5 = temp.at(x+1, y+1) > 128 ? 1 : 0;
                    int p6 = temp.at(x, y+1) > 128 ? 1 : 0;
                    int p7 = temp.at(x-1, y+1) > 128 ? 1 : 0;
                    int p8 = temp.at(x-1, y) > 128 ? 1 : 0;
                    int p9 = temp.at(x-1, y-1) > 128 ? 1 : 0;
                
                    // 计算邻域中1的个数
                    int neighbors = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;
                
                    // 计算0->1的转换次数
                    int transitions = 0;
                    if (p2 == 0 && p3 == 1) transitions++;
                    if (p3 == 0 && p4 == 1) transitions++;
                    if (p4 == 0 && p5 == 1) transitions++;
                    if (p5 == 0 && p6 == 1) transitions++;
                    if (p6 == 0 && p7 == 1) transitions++;
                    if (p7 == 0 && p8 == 1) transitions++;
                    if (p8 == 0 && p9 == 1) transitions++;
                    if (p9 == 0 && p2 == 1) transitions++;
                
                    // 细化条件
                    if (neighbors >= 2 && neighbors <= 6 &&
                        transitions == 1 &&
                        (p2 * p4 * p6) == 0 && 
                        (p4 * p6 * p8) == 0) {
                        marker.at(x, y) = 255; // 标记为删除
                        changed = true;
                    }
                }
            }
        
            // 删除标记的点
            for (int y = 0; y < input.height; y++) {
                for (int x = 0; x < input.width; x++) {
                    if (marker.at(x, y) > 0) {
                        temp.at(x, y) = 0;
                    }
                }
            }
        } while (changed && iteration < maxIterations);
    
        skeleton = temp;
        spdlog::info("骨架提取完成，共迭代 {} 次", iteration);
        return skeleton;
    }

    /*
    void SaveSimpleImage(const SimpleImage& image, const std::string& filename) {
        std::vector<unsigned char> rgbData(image.width * image.height * 3);
    
        for (int y = 0; y < image.height; y++) {
            for (int x = 0; x < image.width; x++) {
                int index = (y * image.width + x) * 3;
                uint8_t pixel = image.at(x, y);
                rgbData[index] = pixel;     // R
                rgbData[index + 1] = pixel; // G
                rgbData[index + 2] = pixel; // B
            }
        }
    
        // 保存为PNG
        int success = stbi_write_png(filename.c_str(), image.width, image.height, 3, rgbData.data(), image.width * 3);
    
        if (success) {
            spdlog::info("PNG图像已保存: {}", filename);
        } 
        else {
            spdlog::error("保存PNG失败: {}", filename);
        }
    }
    */
    
    std::vector<glm::vec3> ExtractContourPoints(const SimpleImage& skeleton) {
        std::vector<glm::vec3> points;
    
        for (int y = 1; y < skeleton.height; y+=2) {
            for (int x = 1; x < skeleton.width; x+=2) {
                if (skeleton.at(x, y) > 150) {
                    // 将图像坐标转换为3D空间坐标
                    float fx = static_cast<float>(x) / skeleton.width;
                    float fy = static_cast<float>(y) / skeleton.height;
                    points.emplace_back(fx, 0.0f, fy);
                }
            }
        }
    
        return points;
    }

    std::vector<glm::vec3> LoadSimpleContourFromImage(const std::string& imagePath) {
        std::vector<glm::vec3> points;
    
        // 读取图像
        SimpleImage input = LoadImageFile(imagePath);
    
        // 提取图像骨架
        SimpleImage skeleton = ExtractSkeleton(input);

        // SaveSimpleImage(skeleton, "4-Animation/skeleton_image.png");
    
        // 提取轮廓点
        points = ExtractContourPoints(skeleton);
    
        return points;
    }

    IKSystem::Vec3ArrPtr IKSystem::BuildCustomTargetPosition() {
        // get function from https://www.wolframalpha.com/input/?i=Albert+Einstein+curve
        /*
        int nums = 5000;
        using Vec3Arr = std::vector<glm::vec3>;
        std::shared_ptr<Vec3Arr> custom(new Vec3Arr(nums));
        int index = 0;
        for (int i = 0; i < nums; i++) {
            float x_val = 1.5e-3f * custom_x(92 * glm::pi<float>() * i / nums);
            float y_val = 1.5e-3f * custom_y(92 * glm::pi<float>() * i / nums);
            if (std::abs(x_val) < 1e-3 || std::abs(y_val) < 1e-3) continue;
            (*custom)[index++] = glm::vec3(1.6f - x_val, 0.0f, y_val - 0.2f);
        }
        custom->resize(index);
        return custom;
        */
        

        // Sub-Task 4.2 (bonus=0.5') 输入一张简单的 2D 图像（比如 MNIST 手写数字），提取图像的骨架，作为 IK 的目标轨迹。
        auto points = LoadSimpleContourFromImage("/Users/kangenming/Desktop/vci-2025/src/VCX/Labs/4-Animation/mnist.bmp");
        using Vec3Arr = std::vector<glm::vec3>;
        std::shared_ptr<Vec3Arr> custom(new Vec3Arr(points.begin(), points.end()));
        return custom;

        /*
        Sub-Task 4 (0.5')
        使用 IK 绘制自定义曲线。程序入口在 tasks.cpp 中 BuildCustomTargetPosition 函数。
        模仿原有的函数进行修改，返回类型为 std::shared_ptr<std::vector<glm::vec3>>
        自定义绘制曲线（比如数字，姓名首字母缩写等）
        */

        // 手动定义字母 'K', 'E', 'M' 的线段端点，生成目标轨迹点。
        
        /*
        int nums = 2000; 
        using Vec3Arr = std::vector<glm::vec3>;
        std::shared_ptr<Vec3Arr> custom(new Vec3Arr(nums));
        int index = 0;
    
        // 定义字母的各个线段端点
        // 假设每个字母高度为1，宽度为0.8，字母间距为0.3
    
        // 字母K的线段
        std::vector<glm::vec2> K_segments = {
            // 竖线
            {0.0f, 0.0f}, {0.0f, 1.0f},
            // 左上斜线
            {0.8f, 0.0f}, {0.0f, 0.5f},
            // 左下斜线
            {0.0f, 0.5f}, {0.8f, 1.0f}
        };
    
        // 字母E的线段
        std::vector<glm::vec2> E_segments = {
            // 竖线
            {0.0f, 0.0f}, {0.0f, 1.0f},
            // 上横线
            {0.0f, 0.0f}, {0.8f, 0.0f},
            // 中横线
            {0.0f, 0.5f}, {0.6f, 0.5f},
            // 下横线
            {0.0f, 1.0f}, {0.8f, 1.0f}
        };
    
        // 字母M的线段
        std::vector<glm::vec2> M_segments = {
            // 左竖线
            {0.0f, 0.0f}, {0.0f, 1.0f},
            // 左上到中间
            {0.0f, 0.0f}, {0.4f, 0.5f},
            // 中间到右上
            {0.4f, 0.5f}, {0.8f, 0.0f},
            // 右竖线
            {0.8f, 0.0f}, {0.8f, 1.0f}
        };
    
        // 生成K字母的点
        for (int i = 0; i < K_segments.size(); i += 2) {
            glm::vec2 start = K_segments[i];
            glm::vec2 end = K_segments[i + 1];
            int segment_points = 50; // 每个线段的点数
        
            for (int j = 0; j <= segment_points; j++) {
                float t = (float)j / segment_points;
                glm::vec2 point = start + t * (end - start);
                // K字母在左侧
                (*custom)[index++] = glm::vec3(-0.5f + point.x * 0.3f, 0.0f, point.y * 0.3f + 0.5f);
            }
        }
    
        // 生成E字母的点
        for (int i = 0; i < E_segments.size(); i += 2) {
            glm::vec2 start = E_segments[i];
            glm::vec2 end = E_segments[i + 1];
            int segment_points = 50;
        
            for (int j = 0; j <= segment_points; j++) {
                float t = (float)j / segment_points;
                glm::vec2 point = start + t * (end - start);
                // E字母在中间
                (*custom)[index++] = glm::vec3(0.0f + point.x * 0.3f, 0.0f, point.y * 0.3f + 0.5f);
            }
        }
    
        // 生成M字母的点
        for (int i = 0; i < M_segments.size(); i += 2) {
            glm::vec2 start = M_segments[i];
            glm::vec2 end = M_segments[i + 1];
            int segment_points = 50;
        
            for (int j = 0; j <= segment_points; j++) {
                float t = (float)j / segment_points;
                glm::vec2 point = start + t * (end - start);
                // M字母在右侧
                (*custom)[index++] = glm::vec3(0.5f + point.x * 0.3f, 0.0f, point.y * 0.3f + 0.5f);
            }
        }
    
        custom->resize(index);
        return custom;
        */
    }

    static Eigen::VectorXf glm2eigen(std::vector<glm::vec3> const & glm_v) {
        Eigen::VectorXf v = Eigen::Map<Eigen::VectorXf const, Eigen::Aligned>(reinterpret_cast<float const *>(glm_v.data()), static_cast<int>(glm_v.size() * 3));
        return v;
    }

    static std::vector<glm::vec3> eigen2glm(Eigen::VectorXf const & eigen_v) {
        return std::vector<glm::vec3>(
            reinterpret_cast<glm::vec3 const *>(eigen_v.data()),
            reinterpret_cast<glm::vec3 const *>(eigen_v.data() + eigen_v.size())
        );
    }

    static Eigen::SparseMatrix<float> CreateEigenSparseMatrix(std::size_t n, std::vector<Eigen::Triplet<float>> const & triplets) {
        Eigen::SparseMatrix<float> matLinearized(n, n);
        matLinearized.setFromTriplets(triplets.begin(), triplets.end());
        return matLinearized;
    }

    // solve Ax = b and return x
    static Eigen::VectorXf ComputeSimplicialLLT(
        Eigen::SparseMatrix<float> const & A,
        Eigen::VectorXf const & b) {
        auto solver = Eigen::SimplicialLLT<Eigen::SparseMatrix<float>>(A);
        return solver.solve(b);
    }

    /*
    float g(MassSpringSystem & system, Eigen::VectorXf & x_vec, Eigen::VectorXf & y_vec, float ddt){
        int n = system.Positions.size();
        float inertia_energy = 0.0f;
        float spring_energy = 0.0f;

        // 计算惯性部分
        inertia_energy = (x_vec - y_vec).squaredNorm() * system.Mass / (2.0f * ddt * ddt);

        // 计算弹簧势能部分
        std::vector<glm::vec3> x_vec3 = eigen2glm(x_vec);
        for (auto const & spring : system.Springs) {
            auto const p0 = spring.AdjIdx.first;
            auto const p1 = spring.AdjIdx.second;

            glm::vec3 const x01 = x_vec3[p1] - x_vec3[p0];
            float length = glm::length(x01);
            spring_energy += 0.5f * system.Stiffness * (length - spring.RestLength) * (length - spring.RestLength);
        }

        return inertia_energy + spring_energy;
    }
        */

    Eigen::VectorXf f_inner(MassSpringSystem & system, Eigen::VectorXf & x_vec){
        std::vector<glm::vec3> x_vec3 = eigen2glm(x_vec);
        int n = system.Positions.size();
        std::vector<glm::vec3> forces(n, glm::vec3(0, 0, 0));

        for (auto const & spring : system.Springs) {
            auto const p0 = spring.AdjIdx.first;
            auto const p1 = spring.AdjIdx.second;

            glm::vec3 const x01 = x_vec3[p1] - x_vec3[p0];
            float length = glm::length(x01);

            glm::vec3 e01 = glm::normalize(x01);
            glm::vec3 f = system.Stiffness * (length - spring.RestLength) * e01;
            forces[p0] += f;
            forces[p1] -= f;
        }
        return glm2eigen(forces);
    }

    Eigen::VectorXf grad_g(MassSpringSystem & system, Eigen::VectorXf & x_vec, Eigen::VectorXf & y_vec, float ddt){
        int n = system.Positions.size();
        Eigen::VectorXf grad = Eigen::VectorXf::Zero(3 * n);

        // 惯性部分的梯度
        grad += system.Mass / (ddt * ddt) * (x_vec - y_vec);

        // 弹簧部分的梯度
        grad -= f_inner(system, x_vec);

        return grad;
    }

    void Hessian_Matrix(MassSpringSystem & system, Eigen::VectorXf & x_vec, float ddt, Eigen::SparseMatrix<float> & dst_matrix){
        int n = system.Positions.size();
        std::vector<Eigen::Triplet<float>> triplets;
        
        // 质量矩阵
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < 3; j++) {
                triplets.emplace_back(3 * i + j, 3 * i + j, system.Mass / (ddt * ddt));
            }
        }

        // 弹簧刚度矩阵
        std::vector<glm::vec3> x_vec3 = eigen2glm(x_vec);
        for (auto const & spring : system.Springs) {
            auto const p0 = spring.AdjIdx.first;
            auto const p1 = spring.AdjIdx.second;

            glm::vec3 const x01 = x_vec3[p1] - x_vec3[p0];
            float length = glm::length(x01);
            glm::vec3 e01 = glm::normalize(x01);
            float k = system.Stiffness * (1 - spring.RestLength / length);
            glm::mat3 outer_product;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    outer_product[i][j] = e01[i] * e01[j];
                }
            }

            glm::mat3 I = glm::mat3(1.0f);

            glm::mat3 H_e = system.Stiffness * (outer_product + (1.0f - spring.RestLength / length) * (I - outer_product));

            for (int a = 0; a < 3; a++) {
                for (int b = 0; b < 3; b++) {
                    triplets.emplace_back(3 * p0 + a, 3 * p0 + b, H_e[a][b]);
                    triplets.emplace_back(3 * p1 + a, 3 * p1 + b, H_e[a][b]);
                    triplets.emplace_back(3 * p0 + a, 3 * p1 + b, -H_e[a][b]);
                    triplets.emplace_back(3 * p1 + a, 3 * p0 + b, -H_e[a][b]);
                }
            }
        }
        dst_matrix = CreateEigenSparseMatrix(3 * n, triplets);
    }

    void AdvanceMassSpringSystem(MassSpringSystem & system, float const dt) {
        // your code here: rewrite following code
        
        /*
        int const steps = 1000;
        float const ddt = dt / steps; 
        for (std::size_t s = 0; s < steps; s++) {
            std::vector<glm::vec3> forces(system.Positions.size(), glm::vec3(0));
            for (auto const spring : system.Springs) {
                auto const p0 = spring.AdjIdx.first;
                auto const p1 = spring.AdjIdx.second;
                glm::vec3 const x01 = system.Positions[p1] - system.Positions[p0];
                glm::vec3 const v01 = system.Velocities[p1] - system.Velocities[p0];
                glm::vec3 const e01 = glm::normalize(x01);
                glm::vec3 f = (system.Stiffness * (glm::length(x01) - spring.RestLength) + system.Damping * glm::dot(v01, e01)) * e01;
                forces[p0] += f;
                forces[p1] -= f;
            }
            for (std::size_t i = 0; i < system.Positions.size(); i++) {
                if (system.Fixed[i]) continue;
                system.Velocities[i] += (glm::vec3(0, -system.Gravity, 0) + forces[i] / system.Mass) * ddt;
                system.Positions[i] += system.Velocities[i] * ddt;
            }
        }
        */
       
        // use implicit euler method to advance mass spring system
        int const steps = 1;
        float const ddt = dt / steps; 
        for (std::size_t s = 0; s < steps; s++) {
            int n = system.Positions.size();
            Eigen::VectorXf x0 = glm2eigen(system.Positions);
            Eigen::VectorXf v0 = glm2eigen(system.Velocities);
            Eigen::VectorXf f0 = glm2eigen(std::vector<glm::vec3>(n, glm::vec3(0, -system.Gravity * system.Mass, 0)));
            
            Eigen::VectorXf y0 = x0 + ddt * v0 + ddt * ddt * f0 / system.Mass;

            // 计算梯度
            Eigen::VectorXf g_grad = grad_g(system, x0, y0, ddt);

            // 计算Hessian矩阵
            Eigen::SparseMatrix<float> H;
            Hessian_Matrix(system, x0, ddt, H);

            // 求解步长
            Eigen::VectorXf delta_x = ComputeSimplicialLLT(H, -g_grad);

            // 更新位置和速度
            Eigen::VectorXf x1 = x0 + delta_x;
            Eigen::VectorXf inner_force = f_inner(system, x1);
            Eigen::VectorXf v1 = v0 + ddt * (inner_force + f0) / system.Mass;

            // 更新系统状态
            std::vector<glm::vec3> new_positions = eigen2glm(x1);
            std::vector<glm::vec3> new_velocities = eigen2glm(v1);
            for (int i = 0; i < n; i++) {
                if (!system.Fixed[i]) {
                    system.Positions[i] = new_positions[i];
                    system.Velocities[i] = new_velocities[i];
                }
            }    
        }
    }
}
