#include "Labs/5-Visualization/tasks.h"

#include <numbers>

using VCX::Labs::Common::ImageRGB;
namespace VCX::Labs::Visualization {

    struct CoordinateStates {
        // your code here
        std::vector<Car> data;
        float min_values[7];
        float max_values[7];

        CoordinateStates(std::vector<Car> const & input_data) : data(input_data) {
            for(int i = 0; i < 7; ++i) {
                min_values[i] = std::numeric_limits<float>::max();
                max_values[i] = std::numeric_limits<float>::lowest();
            }
            for(auto const & car : data) {
                min_values[0] = std::min(min_values[0], float(car.cylinders));
                max_values[0] = std::max(max_values[0], float(car.cylinders));

                min_values[1] = std::min(min_values[1], car.displacement);
                max_values[1] = std::max(max_values[1], car.displacement);

                min_values[2] = std::min(min_values[2], car.weight);
                max_values[2] = std::max(max_values[2], car.weight);

                min_values[3] = std::min(min_values[3], car.horsepower);
                max_values[3] = std::max(max_values[3], car.horsepower);

                min_values[4] = std::min(min_values[4], car.acceleration);
                max_values[4] = std::max(max_values[4], car.acceleration);

                min_values[5] = std::min(min_values[5], car.mileage);
                max_values[5] = std::max(max_values[5], car.mileage);

                min_values[6] = std::min(min_values[6], float(car.year));
                max_values[6] = std::max(max_values[6], float(car.year));
            }
        }
    };

    float linear_interpolation(float val, float m, float M, float tM, float TM) {
        return (val - m) / (M - m) * (TM - tM) + tM;
    }

    int mouse_mode = 0;
    bool first_draw = false;
    bool select_bars[7] = { false, false, false, false, false, false, false };
    float select_bars_y[7][2];

    bool PaintParallelCoordinates(Common::ImageRGB & input, InteractProxy const & proxy, std::vector<Car> const & data, bool force) {
        // your code here
        // for example: 
        //   static CoordinateStates states(data);
        //   SetBackGround(input, glm::vec4(1));
        //   ...
        
        static CoordinateStates states(data);
        SetBackGround(input, glm::vec4(1.0f));

        // 处理鼠标交互
        glm::vec2 mouse_from(0.0f, 0.0f);
        glm::vec2 mouse_to(0.0f, 0.0f);

        // 如果鼠标在画布内
        if(proxy.IsHovering()) {

            if(proxy.IsClicking(true)) {
                mouse_from = proxy.MousePos();
                mouse_mode = 1;
            }
            else if(proxy.IsDragging(true)) {
                mouse_from = proxy.DraggingStartPoint();
                mouse_to = proxy.MousePos();
                mouse_mode = 2;
            }
            else if(proxy.IsClicking(false)) {
                first_draw = false;
                memset(select_bars, 0, sizeof(select_bars));
            }
        }

        // 如果不是第一次绘制且没有交互，跳过重绘
        if(first_draw && mouse_mode == 0 && !force) {
            return false;
        }

        // 处理数据
        const float padding_percent = 0.1f;
        const float bar_up = 0.15f;
        const float bar_down = 0.9f;
        const float bar_width = 0.04f;
        const float origin_alpha = 0.5f;
        const float select_alpha = 0.2f;

        int max_vals[7];
        int min_vals[7];
        float coordinate_x[7];

        for(int i = 0 ; i < 7 ; ++i) {
            int distance = int((states.max_values[i] - states.min_values[i]) * padding_percent) + 1;
            min_vals[i] = int(states.min_values[i]) - distance;
            max_vals[i] = int(states.max_values[i]) + distance;
            coordinate_x[i] = (0.85f / 6.0f) * i + 0.075f;
        }

        // 绘制坐标轴

        std::string axis_names[7] = { "cylinders", "displacement", "weight", "horsepower", "acceleration", "mileage", "year" };
        for(int i = 0 ; i < 7 ; ++i) {
            // 纵线
            DrawLine(input, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 
                     glm::vec2(coordinate_x[i], bar_up), 
                     glm::vec2(coordinate_x[i], bar_down), 3.0f);

            // 填充区域
            DrawFilledRect(input, glm::vec4(0.0f, 0.5f, 0.5f, 0.3f), 
                           glm::vec2(coordinate_x[i] - bar_width / 2.0f, bar_up), 
                           glm::vec2(bar_width, bar_down - bar_up));
            
            // 文字
            PrintText(input, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 
                      glm::vec2(coordinate_x[i], 0.09f), 0.025f, axis_names[i]);
            
            // 最大值
            PrintText(input, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 
                      glm::vec2(coordinate_x[i], 0.125f), 0.03f, std::to_string(max_vals[i]));
            
            // 最小值
            PrintText(input, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 
                      glm::vec2(coordinate_x[i], 0.925f), 0.03f, std::to_string(min_vals[i]));
        }
        
        // 交互
        int select_bar = -1;

        if(mouse_mode != 0) {
            for(int i = 0 ; i < 7 ; ++i) {
                if(std::abs(mouse_from.x - coordinate_x[i]) <= bar_width) {
                    select_bar = i;

                    mouse_from.y = std::clamp(mouse_from.y, bar_up, bar_down);
                    mouse_to.y   = std::clamp(mouse_to.y, bar_up, bar_down);

                    if(mouse_mode == 2) {
                        select_bars[i] = true;
                        select_bars_y[i][0] = mouse_from.y;
                        select_bars_y[i][1] = mouse_to.y;
                    }

                    break;
                }
            }
        }

        // 绘制数据线
        if(!first_draw) {
            first_draw = true;
            int data_count = states.data.size();

            for(int i = 0 ; i < data_count ; ++i) {
                const Car& car = states.data[i];

                float cur_data[7] = {
                    float(car.cylinders),
                    car.displacement,
                    car.weight,
                    car.horsepower,
                    car.acceleration,
                    car.mileage,
                    float(car.year)
                };


                float mileage_normalized = car.mileage / 50.0f;
            
                // 基于mileage从蓝到红渐变
                float t = glm::clamp(mileage_normalized, 0.0f, 1.0f);
                glm::vec4 line_color = glm::vec4(
                    0.1f + 0.7f * t,       // R: 从0.1到0.8
                    0.2f,                  // G: 固定0.2
                    0.6f - 0.5f * t,       // B: 从0.6到0.1
                    origin_alpha
                );
                // glm::vec4 line_color(0.1f, 0.3f, 0.6f, 0.4f);
                
                float prev_y = linear_interpolation(cur_data[0], float(min_vals[0]), float(max_vals[0]), bar_down, bar_up);
                for(int j = 1 ; j < 7 ; ++j) {
                    float cur_y = linear_interpolation(cur_data[j], float(min_vals[j]), float(max_vals[j]), bar_down, bar_up);

                    DrawLine(input, line_color, 
                             glm::vec2(coordinate_x[j - 1], prev_y), 
                             glm::vec2(coordinate_x[j], cur_y), 1.0f);

                    prev_y = cur_y;
                }
            }
            return true;
        }

        if(select_bar == -1) {
            return false;
        }

        if(mouse_mode == 1) {
            DrawFilledRect(input, glm::vec4(1.0f, 0.2f, 0.1f, 0.7f), 
                           glm::vec2(coordinate_x[select_bar] - bar_width / 2.0f, bar_up), 
                           glm::vec2(bar_width, bar_down - bar_up));

            memset(select_bars, 0, sizeof(select_bars));

            int data_count = states.data.size();
            for(int i = 0 ; i < data_count ; ++i) {
                const Car& car = states.data[i];

                float cur_data[7] = {
                    float(car.cylinders),
                    car.displacement,
                    car.weight,
                    car.horsepower,
                    car.acceleration,
                    car.mileage,
                    float(car.year)
                };


                float color = (cur_data[select_bar] - float(min_vals[select_bar])) / (float(max_vals[select_bar]) - float(min_vals[select_bar]));
                glm::vec4 line_color = glm::vec4(
                    0.1f + 0.7f * color, 
                    0.2f, 
                    0.6f - 0.5f * color, 
                    origin_alpha);
                
                float prev_y = linear_interpolation(cur_data[0], float(min_vals[0]), float(max_vals[0]), bar_down, bar_up);
                for(int j = 1 ; j < 7 ; ++j) {
                    float cur_y = linear_interpolation(cur_data[j], float(min_vals[j]), float(max_vals[j]), bar_down, bar_up);

                    DrawLine(input, line_color, 
                             glm::vec2(coordinate_x[j - 1], prev_y), 
                             glm::vec2(coordinate_x[j], cur_y), 1.0f);

                    prev_y = cur_y;
                }
            }
            return true;
        }


        if(mouse_mode == 2) {
            for(int i = 0 ; i < 7 ; ++i) {
                if(select_bars[i]) {
                    DrawFilledRect(input, glm::vec4(1.0f, 0.2f, 0.1f, 0.3f), 
                                   glm::vec2(coordinate_x[i] - bar_width / 2.0f, select_bars_y[i][0]), 
                                   glm::vec2(bar_width, select_bars_y[i][1] - select_bars_y[i][0]));
                }
            }

            int data_count = states.data.size();
            for(int i = 0 ; i < data_count ; ++i) {
                const Car& car = states.data[i];

                float cur_data[7] = {
                    float(car.cylinders),
                    car.displacement,
                    car.weight,
                    car.horsepower,
                    car.acceleration,
                    car.mileage,
                    float(car.year)
                };

                float color = (cur_data[select_bar] - float(min_vals[select_bar])) / (float(max_vals[select_bar]) - float(min_vals[select_bar]));
                float line_alpha = origin_alpha;
                for(int j = 0 ; j < 7 ; ++j) {
                    if(select_bars[j]) {
                        float cur_y = linear_interpolation(cur_data[j], float(min_vals[j]), float(max_vals[j]), bar_down, bar_up);
                        if(cur_y < std::min(select_bars_y[j][0], select_bars_y[j][1]) || 
                           cur_y > std::max(select_bars_y[j][0], select_bars_y[j][1])) {
                            line_alpha = select_alpha;
                            break;
                        }
                    }
                }

                glm::vec4 line_color = glm::vec4(
                    0.1f + 0.7f * color, 
                    0.2f, 
                    0.6f - 0.5f * color, 
                    line_alpha);
                
                float prev_y = linear_interpolation(cur_data[0], float(min_vals[0]), float(max_vals[0]), bar_down, bar_up);
                for(int j = 1 ; j < 7 ; ++j) {
                    float cur_y = linear_interpolation(cur_data[j], float(min_vals[j]), float(max_vals[j]), bar_down, bar_up);

                    DrawLine(input, line_color, 
                             glm::vec2(coordinate_x[j - 1], prev_y), 
                             glm::vec2(coordinate_x[j], cur_y), 1.2f);

                    prev_y = cur_y;
                }
            }

            for(int i = 0 ; i < 7 ; ++i) {
                if(select_bars[i]) {
                    float y_start = std::min(select_bars_y[i][0], select_bars_y[i][1]);;
                    float y_end   = std::max(select_bars_y[i][0], select_bars_y[i][1]);;
                    DrawFilledRect(input, glm::vec4(1.0f, 0.2f, 0.1f, 0.3f), 
                                   glm::vec2(coordinate_x[i] - bar_width / 2.0f, y_start), 
                                   glm::vec2(bar_width, y_end - y_start));

                    float bar_height = bar_down - bar_up;
                    float val_start = max_vals[i] - (y_start - bar_up) / bar_height * (float(max_vals[i]) - float(min_vals[i]));
                    float val_end   = max_vals[i] - (y_end   - bar_up) / bar_height * (float(max_vals[i]) - float(min_vals[i]));
                    
                    PrintText(input, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 
                              glm::vec2(coordinate_x[i] - 0.005f, y_start - 0.02f), 0.015f, std::to_string(int(val_start)));
                    PrintText(input, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 
                              glm::vec2(coordinate_x[i] - 0.005f, y_end + 0.01f), 0.015f, std::to_string(int(val_end))); 

                }
            }
            return true;
        }

        return false;
    }

    void LIC(ImageRGB & output, Common::ImageRGB const & noise, VectorField2D const & field, int const & step) {
        // your code here
        int width = output.GetSizeX();
        int height = output.GetSizeY();
        SetBackGround(output, glm::vec4(0.0f));

        // 设置卷积核
        int total_steps = step * 2;
        float kernel[total_steps];
        float kernel_sum = 0.0f;

        for(int i = 0 ; i < total_steps ; ++i) {
            float x = (float(i) / float(total_steps - 1)) * 2.0f - 1.0f; // [-1, 1]
            kernel[i] = 0.5f * (1.0f + std::cos(std::numbers::pi_v<float> * x)); // Hanning window
            kernel_sum += kernel[i];
        }

        for(int i = 0 ; i < total_steps ; ++i) {
            kernel[i] /= kernel_sum;
        }


        for(int i = 0 ; i < height ; ++i) {
            for(int j = 0 ; j < width ; ++j) {
                std::vector<glm::vec3> streamlineColors;
                std::vector<float> streamlineWeights;

                float y = float(i) + 0.5f;
                float x = float(j) + 0.5f;

                streamlineColors.push_back(noise.At(int(x), int(y)));
                streamlineWeights.push_back(kernel[step]);

                // 向前追踪
                float cur_x = x;
                float cur_y = y;
                for(int s = 1 ; s < step ; ++s) {
                    float dx = field.At(int(cur_x), int(cur_y)).x;
                    float dy = field.At(int(cur_x), int(cur_y)).y;
                    float dt_x = 0;   // 到达下一个像素垂直边界所需的时间
                    float dt_y = 0;   // 到达下一个像素水平边界所需的时间
                    float dt = 0;

                    // 当前位置在 [floor(y), ceil(y)] 区间内
                    if(dy > 0) dt_y = (floor(cur_y) + 1 - cur_y) / dy;      // 到达下边界
                    else if(dy < 0) dt_y = (ceil(cur_y) - 1 - cur_y) / dy;  // 到达上边界
                    
                    // 当前位置在 [floor(x), ceil(x)] 区间内
                    if(dx > 0) dt_x = (floor(cur_x) + 1 - cur_x) / dx;      // 到达右边界
                    else if(dx < 0) dt_x = (ceil(cur_x) - 1 - cur_x) / dx;  // 到达左边界

                    if(dx == 0 && dy == 0) dt = 0;
                    else dt = std::min(dt_x, dt_y);

                    cur_x = std::min(std::max(cur_x + dx * dt, 0.0f), float(width - 1));
                    cur_y = std::min(std::max(cur_y + dy * dt, 0.0f), float(height - 1));

                    streamlineColors.push_back(noise.At(int(cur_x), int(cur_y)));
                    streamlineWeights.push_back(kernel[step + s]);
                }

                // 向后追踪
                cur_x = x;
                cur_y = y;
                for(int s = 1 ; s < step ; ++s) {
                    float dx = -field.At(int(cur_x), int(cur_y)).x;
                    float dy = -field.At(int(cur_x), int(cur_y)).y;
                    float dt_x = 0;  // 到达下一个像素垂直边界所需的时间
                    float dt_y = 0;  // 到达下一个像素水平边界所需的时间
                    float dt = 0;

                    if(dy > 0) dt_y = (floor(cur_y) + 1 - cur_y) / dy;
                    else if(dy < 0) dt_y = (ceil(cur_y) - 1 - cur_y) / dy;

                    if(dx > 0) dt_x = (floor(cur_x) + 1 - cur_x) / dx;
                    else if(dx < 0) dt_x = (ceil(cur_x) - 1 - cur_x) / dx; 

                    if(dx == 0 && dy == 0) dt = 0;
                    else dt = std::min(dt_x, dt_y);

                    cur_x = std::min(std::max(cur_x + dx * dt, 0.0f), float(width - 1));
                    cur_y = std::min(std::max(cur_y + dy * dt, 0.0f), float(height - 1));

                    streamlineColors.insert(streamlineColors.begin(), noise.At(int(cur_x), int(cur_y)));
                    streamlineWeights.insert(streamlineWeights.begin(), kernel[step - s]);
                }

                // 计算颜色
                glm::vec3 final_color(0.0f);
                float total_weight = 0.0f;
                int streamline_length = streamlineColors.size();
                for(int s = 0 ; s < streamline_length ; ++s) {
                    final_color += streamlineColors[s] * streamlineWeights[s];
                    total_weight += streamlineWeights[s];
                }

                if(total_weight > 0.0f) {
                    output.At(j, i) = final_color / total_weight;
                }
                else {
                    output.At(j, i) = noise.At(j, i);
                }
            }
        }
    }
}; // namespace VCX::Labs::Visualization
