#include <random>

#include <spdlog/spdlog.h>

#include "Labs/1-Drawing2D/tasks.h"

using VCX::Labs::Common::ImageRGB;

namespace VCX::Labs::Drawing2D {
    /******************* 1.Image Dithering *****************/
    void DitheringThreshold(
        ImageRGB &       output,
        ImageRGB const & input) {
        for (std::size_t x = 0; x < input.GetSizeX(); ++x)
            for (std::size_t y = 0; y < input.GetSizeY(); ++y) {
                glm::vec3 color = input.At(x, y);
                output.At(x, y) = {
                    color.r > 0.5 ? 1 : 0,
                    color.g > 0.5 ? 1 : 0,
                    color.b > 0.5 ? 1 : 0,
                };
            }
    }

    void DitheringRandomUniform(
        ImageRGB &       output,
        ImageRGB const & input) {
        // your code here:
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dis(-0.5, 0.5);
        for(std::size_t x = 0 ; x < input.GetSizeX() ; ++x)
            for(std::size_t y = 0 ; y < input.GetSizeY() ; ++y) {
                double d = dis(gen);
                glm::vec3 color = input.At(x, y);
                output.At(x, y) = {
                    (color.r + d > 0.5) ? 1 : 0,
                    (color.g + d > 0.5) ? 1 : 0,
                    (color.b + d > 0.5) ? 1 : 0,
                }; 
            }

    }

    void DitheringRandomBlueNoise(
        ImageRGB &       output,
        ImageRGB const & input,
        ImageRGB const & noise) {
        // your code here:
        for(size_t x = 0 ; x < input.GetSizeX() ; x++){
            for(size_t y = 0 ; y < input.GetSizeY() ; y++){
                glm::vec3 color = input.At(x, y);
                glm::vec3 noise_color = noise.At(x, y);
                output.At(x, y) = {
                    color.r + noise_color.r > 1 ? 1 : 0,
                    color.g + noise_color.g > 1 ? 1 : 0,
                    color.b + noise_color.b > 1 ? 1 : 0,
                };
            }
        }
    }

    void DitheringOrdered(
        ImageRGB &       output,
        ImageRGB const & input) {
        // your code here:
        int dx[9] = {0, 0, 0, 1, 1, 1, 2, 2, 2};
        int dy[9] = {0, 1, 2, 0, 1, 2, 0, 1, 2};
        int matrix[3][3] = { {6, 8, 4},
                            {1, 0, 3},
                            {5, 2, 7}
        };
        for (std::size_t x = 0; x < input.GetSizeX(); ++x) {
            for (std::size_t y = 0; y < input.GetSizeY(); ++y) {
                glm::vec3 color = input.At(x, y);
                for(int k = 0 ; k < 9 ; k++){
                    output.At(3 * x + dx[k], 3 * y + dy[k]) = {
                        color.r * 9 > matrix[k / 3][k % 3] ? 1 : 0,
                        color.g * 9 > matrix[k / 3][k % 3] ? 1 : 0,
                        color.b * 9 > matrix[k / 3][k % 3] ? 1 : 0,
                    };
                }  
            }
        }
    }

    void DitheringErrorDiffuse(
        ImageRGB &       output,
        ImageRGB const & input) {
        // your code here:
        size_t X = input.GetSizeX();
        size_t Y = input.GetSizeY();
        for(size_t y = 0; y < Y; ++y) {
            for(size_t x = 0; x < X; ++x) {
                output.At(x, y) = input.At(x, y);
            }
        }

        for(size_t y = 0 ; y < Y ; ++y) {
            for(size_t x = 0 ; x < X ; ++x) {
                glm::vec3 oldColor = output.At(x, y);
                glm::vec3 newColor = {
                    oldColor.r > 0.5 ? 1.0 : 0.0,
                    oldColor.g > 0.5 ? 1.0 : 0.0,
                    oldColor.b > 0.5 ? 1.0 : 0.0
                };
                output.At(x, y) = newColor;
            
                double d = oldColor.r - newColor.r;

                if(y < Y - 1){
                    if(x > 0){
                        glm::vec3 c = output.At(x - 1, y + 1);
                        output.At(x - 1, y + 1) = {
                            c.r + d * 3 / 16,
                            c.g + d * 3 / 16,
                            c.b + d * 3 / 16
                        };
                    }
                    glm::vec3 c = output.At(x, y + 1);
                    output.At(x, y + 1) = {
                        c.r + d * 5 / 16,
                        c.g + d * 5 / 16,
                        c.b + d * 5 / 16,
                    };
                    if(x < X - 1){
                        glm::vec3 c = output.At(x + 1, y + 1);
                        output.At(x + 1, y + 1) = {
                            c.r + d * 1 / 16,
                            c.g + d * 1 / 16,
                            c.b + d * 1 / 16,
                        };
                    }
                }

                if(x < X - 1){
                    glm::vec3 c = output.At(x + 1, y);
                    output.At(x + 1, y) = {
                        c.r += d * 7 / 16,
                        c.g += d * 7 / 16,
                        c.b += d * 7 / 16,
                    };
                }
            }
        }
    }

    /******************* 2.Image Filtering *****************/
    void Blur(
        ImageRGB &       output,
        ImageRGB const & input) {
        // your code here:
        int X = input.GetSizeX(), Y = input.GetSizeY();
        glm::vec3 p1 = glm::vec3(input.At(0, 0));
        glm::vec3 p2 = glm::vec3(input.At(0, 1));
        glm::vec3 p3 = glm::vec3(input.At(1, 0));
        glm::vec3 p4 = glm::vec3(input.At(1, 1));
        glm::vec3 p5, p6, p7, p8, p9;

        output.At(0, 0) = {
            (p1.r + p2.r + p3.r + p4.r) / 4.0,
            (p1.g + p2.g + p3.g + p4.g) / 4.0,
            (p1.b + p2.b + p3.b + p4.b) / 4.0,
        };


        p1 = glm::vec3(input.At(X - 1, 0));
        p2 = glm::vec3(input.At(X - 2, 0));
        p3 = glm::vec3(input.At(X - 1, 1));
        p4 = glm::vec3(input.At(X - 2, 1));

        output.At(X - 1, 0) = {
            (p1.r + p2.r + p3.r + p4.r) / 4.0,
            (p1.g + p2.g + p3.g + p4.g) / 4.0,
            (p1.b + p2.b + p3.b + p4.b) / 4.0,
        };


        p1 = glm::vec3(input.At(0, Y - 1));
        p2 = glm::vec3(input.At(0, Y - 2));
        p3 = glm::vec3(input.At(1, Y - 1));
        p4 = glm::vec3(input.At(1, Y - 2));

        output.At(0, Y - 1) = {
            (p1.r + p2.r + p3.r + p4.r) / 4.0,
            (p1.g + p2.g + p3.g + p4.g) / 4.0,
            (p1.b + p2.b + p3.b + p4.b) / 4.0,
        };


        p1 = glm::vec3(input.At(X - 1, Y - 1));
        p2 = glm::vec3(input.At(X - 2, Y - 1));
        p3 = glm::vec3(input.At(X - 1, Y - 2));
        p4 = glm::vec3(input.At(X - 2, Y - 2));

        output.At(X - 1, Y - 1) = {
            (p1.r + p2.r + p3.r + p4.r) / 4.0,
            (p1.g + p2.g + p3.g + p4.g) / 4.0,
            (p1.b + p2.b + p3.b + p4.b) / 4.0,
        };
        
        for(size_t x = 1 ; x < X - 1 ; ++x){
            p1 = input.At(x - 1, 0);
            p2 = input.At(x, 0);
            p3 = input.At(x + 1, 0);
            p4 = input.At(x - 1, 1);
            p5 = input.At(x, 1);
            p6 = input.At(x + 1, 1);
            output.At(x, 0) = {
                (p1.r + p2.r + p3.r + p4.r + p5.r + p6.r) / 6.0,
                (p1.g + p2.g + p3.g + p4.g + p5.g + p6.g) / 6.0,
                (p1.b + p2.b + p3.b + p4.b + p5.b + p6.b) / 6.0,
            };
        }

        for(size_t x = 1 ; x < X - 1 ; ++x){
            p1 = input.At(x - 1, Y - 1);
            p2 = input.At(x, Y - 1);
            p3 = input.At(x + 1, Y - 1);
            p4 = input.At(x - 1, Y - 2);
            p5 = input.At(x, Y - 2);
            p6 = input.At(x + 1, Y - 2);
            output.At(x, Y - 1) = {
                (p1.r + p2.r + p3.r + p4.r + p5.r + p6.r) / 6.0,
                (p1.g + p2.g + p3.g + p4.g + p5.g + p6.g) / 6.0,
                (p1.b + p2.b + p3.b + p4.b + p5.b + p6.b) / 6.0,
            };
        }

        for(size_t y = 1 ; y < Y - 1 ; ++y){
            p1 = input.At(0, y - 1);
            p2 = input.At(0, y);
            p3 = input.At(0, y + 1);
            p4 = input.At(1, y - 1);
            p5 = input.At(1, y);
            p6 = input.At(1, y + 1);
            output.At(0, y) = {
                (p1.r + p2.r + p3.r + p4.r + p5.r + p6.r) / 6.0,
                (p1.g + p2.g + p3.g + p4.g + p5.g + p6.g) / 6.0,
                (p1.b + p2.b + p3.b + p4.b + p5.b + p6.b) / 6.0,
            };
        }

        for(size_t y = 1 ; y < Y - 1 ; ++y){
            p1 = input.At(X - 1, y - 1);
            p2 = input.At(X - 1, y);
            p3 = input.At(X - 1, y + 1);
            p4 = input.At(X - 2, y - 1);
            p5 = input.At(X - 2, y);
            p6 = input.At(X - 2, y + 1);
            output.At(X - 1, y) = {
                (p1.r + p2.r + p3.r + p4.r + p5.r + p6.r) / 6.0,
                (p1.g + p2.g + p3.g + p4.g + p5.g + p6.g) / 6.0,
                (p1.b + p2.b + p3.b + p4.b + p5.b + p6.b) / 6.0,
            };
        }

        for(size_t x = 1 ; x < X - 1 ; ++x){
            for(size_t y = 1 ; y < Y - 1 ; ++y){
                p1 = input.At(x - 1, y - 1);
                p2 = input.At(x - 1, y);
                p3 = input.At(x - 1, y + 1);
                p4 = input.At(x, y - 1);
                p5 = input.At(x, y);
                p6 = input.At(x, y + 1);
                p7 = input.At(x + 1, y - 1);
                p8 = input.At(x + 1, y);
                p9 = input.At(x + 1, y + 1);
                output.At(x, y) = {
                    (p1.r + p2.r + p3.r + p4.r + p5.r + p6.r + p7.r + p8.r + p9.r) / 9.0,
                    (p1.g + p2.g + p3.g + p4.g + p5.g + p6.g + p7.g + p8.g + p9.g) / 9.0,
                    (p1.b + p2.b + p3.b + p4.b + p5.b + p6.b + p7.b + p8.b + p9.b) / 9.0,
                };
            }
        }
    }

    void Edge(
        ImageRGB &       output,
        ImageRGB const & input) {
        // your code here:
        int sobel_x[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
        int sobel_y[3][3] = {{1, 2, 1}, {0, 0, 0}, {-1, -2, -1}};
        int width = input.GetSizeX();
        int height = input.GetSizeY();
        for(size_t x = 1 ; x < width - 1 ; ++x){
            for(size_t y = 1 ; y < height - 1 ; ++y){
                double rx = 0.0 , ry = 0.0;
                double gx = 0.0 , gy = 0.0;
                double bx = 0.0 , by = 0.0; 
                for(int i = -1 ; i <= 1 ; i++){
                    for(int j = -1 ; j <= 1 ; j++){
                        glm::vec3 c = input.At(x + i, y + j);
                        rx += c.r * sobel_x[i + 1][j + 1];
                        ry += c.r * sobel_y[i + 1][j + 1];
                        gx += c.g * sobel_x[i + 1][j + 1];
                        gy += c.g * sobel_y[i + 1][j + 1];
                        bx += c.b * sobel_x[i + 1][j + 1];
                        by += c.b * sobel_y[i + 1][j + 1];
                    }
                } 
    
                output.At(x, y) = {
                    sqrt(rx * rx + ry * ry),
                    sqrt(gx * gx + gy * gy),
                    sqrt(bx * bx + by * by)
                };
            }
        }
    }

    /******************* 3. Image Inpainting *****************/
    void Inpainting(
        ImageRGB &         output,
        ImageRGB const &   inputBack,
        ImageRGB const &   inputFront,
        const glm::ivec2 & offset) {
        output             = inputBack;
        std::size_t width  = inputFront.GetSizeX();
        std::size_t height = inputFront.GetSizeY();
        glm::vec3 * g      = new glm::vec3[width * height];
        memset(g, 0, sizeof(glm::vec3) * width * height);
        // set boundary condition
        for (std::size_t y = 0; y < height; ++y) {
            // set boundary for (0, y), your code: g[y * width] = ?
            g[y * width] = inputBack.At(offset.x, y + offset.y) - inputFront.At(0, y);
            // set boundary for (width - 1, y), your code: g[y * width + width - 1] = ?
            g[y * width + width - 1] = inputBack.At(width - 1 + offset.x, y + offset.y) - inputFront.At(width - 1, y);
        }
        for (std::size_t x = 0; x < width; ++x) {
            // set boundary for (x, 0), your code: g[x] = ?
            g[x] = inputBack.At(x + offset.x, offset.y) - inputFront.At(x, 0);
            // set boundary for (x, height - 1), your code: g[(height - 1) * width + x] = ?
            g[(height - 1) * width + x] = inputBack.At(x + offset.x, height - 1 + offset.y) - inputFront.At(x, height - 1);
        }

        // Jacobi iteration, solve Ag = b
        for (int iter = 0; iter < 8000; ++iter) {
            for (std::size_t y = 1; y < height - 1; ++y)
                for (std::size_t x = 1; x < width - 1; ++x) {
                    g[y * width + x] = (g[(y - 1) * width + x] + g[(y + 1) * width + x] + g[y * width + x - 1] + g[y * width + x + 1]);
                    g[y * width + x] = g[y * width + x] * glm::vec3(0.25);
                }
        }

        for (std::size_t y = 0; y < inputFront.GetSizeY(); ++y)
            for (std::size_t x = 0; x < inputFront.GetSizeX(); ++x) {
                glm::vec3 color = g[y * width + x] + inputFront.At(x, y);
                output.At(x + offset.x, y + offset.y) = color;
            }
        delete[] g;
    }

    /******************* 4. Line Drawing *****************/
    void DrawLine(
        ImageRGB &       canvas,
        glm::vec3 const  color,
        glm::ivec2 const p0,
        glm::ivec2 const p1) {
        // your code here:
        int width = canvas.GetSizeX();
        int height = canvas.GetSizeY();
        int x0 = p0.x, y0 = p0.y;
        int x1 = p1.x, y1 = p1.y;
        int dx = abs(x1 - x0);
        int dy = abs(y1 - y0);
        bool k = (dy > dx);
        if(k){ // 斜率大于1或小于-1
            x0 = x0 ^ y0;
            y0 = x0 ^ y0;
            x0 = x0 ^ y0;

            x1 = x1 ^ y1;
            y1 = x1 ^ y1;
            x1 = x1 ^ y1;

            dx = dx ^ dy;
            dy = dx ^ dy;
            dx = dx ^ dy;
        }

        if(x0 > x1){
            x0 = x0 ^ x1;
            x1 = x0 ^ x1;
            x0 = x0 ^ x1;

            y0 = y0 ^ y1;
            y1 = y0 ^ y1;
            y0 = y0 ^ y1;
        }

        int y = y0;
        int y_step = (y1 > y0) ? 1 : -1;

        dy *= 2;
        int F = dy - dx;
        dx *= 2;
        int dxdy = dy - dx;

        for(int x = x0 ; x < x1 ; x++){
            if(k){
                canvas.At(y, x) = color;
            }
            else{
                canvas.At(x, y) = color;
            }

            if(F < 0){
                F += dy;
            }
            else{
                y += y_step;
                F += dxdy;
            }
        }
    }

    /******************* 5. Triangle Drawing *****************/
    void DrawTriangleFilled(
        ImageRGB &       canvas,
        glm::vec3 const  color,
        glm::ivec2 const p0,
        glm::ivec2 const p1,
        glm::ivec2 const p2) {
        // your code here:
        int height = canvas.GetSizeY();
        int width = canvas.GetSizeX();

        for(size_t x = 0 ; x < width ; x++){
            for(size_t y = 0 ; y < height ; y++){
                glm::ivec2 p = {x, y};
                int d1 = (p.x - p1.x) * (p0.y - p1.y) - (p0.x - p1.x) * (p.y - p1.y);
                int d2 = (p.x - p2.x) * (p1.y - p2.y) - (p1.x - p2.x) * (p.y - p2.y);
                int d3 = (p.x - p0.x) * (p2.y - p0.y) - (p2.x - p0.x) * (p.y - p0.y);

                if((d1 > 0 && d2 > 0 && d3 > 0) || (d1 < 0 && d2 < 0 && d3 < 0)){
                    canvas.At(x, y) = color;
                }
            }
        }
        
    }

    /******************* 6. Image Supersampling *****************/
    void Supersample(
        ImageRGB &       output,
        ImageRGB const & input,
        int              rate) {
        // your code here:
        int inwidth = input.GetSizeX();
        int inheight = input.GetSizeY();
        int outwidth = output.GetSizeX();
        int outheight = output.GetSizeY();

        auto interpolate = [&](double a, double b) -> glm::vec3{
            int x0 = (int)a;
            int y0 = (int)b;
            int x1 = std::min(x0 + 1, inwidth - 1);
            int y1 = std::min(y0 + 1, inheight - 1);

            double dx = a - x0;
            double dy = b - y0;

            glm::vec3 c00 = input.At(x0, y0);
            glm::vec3 c01 = input.At(x0, y1);
            glm::vec3 c10 = input.At(x1, y0);
            glm::vec3 c11 = input.At(x1, y1);

            glm::vec3 ans = {
                c00.r * (1 - dx) * (1 - dy) + c01.r * (1 - dx) * dy + c10.r * dx * (1 - dy) + c11.r * dx * dy,
                c00.g * (1 - dx) * (1 - dy) + c01.g * (1 - dx) * dy + c10.g * dx * (1 - dy) + c11.g * dx * dy,
                c00.b * (1 - dx) * (1 - dy) + c01.b * (1 - dx) * dy + c10.b * dx * (1 - dy) + c11.b * dx * dy
            };

            return ans;
        };

        for(size_t x = 0 ; x < outwidth ; x++){
            for(size_t y = 0 ; y < outheight ; ++y){
                glm::vec3 color = {0, 0, 0};
                int cnt = 0;

                for(int i = 0 ; i < rate ; i++){
                    for(int j = 0 ; j < rate ; j++){
                        double dx = (x + (i + 0.5) / rate) * inwidth / outwidth;
                        double dy = (y + (j + 0.5) / rate) * inheight / outheight;

                        if(dx >= 0 && dx < inwidth && dy >= 0 && dy < inheight){
                            color += interpolate(dx, dy);
                            cnt++;
                        }
                    }
                }
                
                if(cnt){
                    output.At(x, y) = {
                        color.r / (double)cnt,
                        color.g / (double)cnt,
                        color.b / (double)cnt
                    };
                }
            }
        }
    }

    /******************* 7. Bezier Curve *****************/
    // Note: Please finish the function [DrawLine] before trying this part.
    glm::vec2 CalculateBezierPoint(
        std::span<glm::vec2> points,
        float const          t) {
        // your code here:
        glm::vec2 x1, x2, x3, y1, y2, p;
        x1.x = (1 - t) * points[0].x + t * points[1].x;
        x1.y = (1 - t) * points[0].y + t * points[1].y;

        x2.x = (1 - t) * points[1].x + t * points[2].x;
        x2.y = (1 - t) * points[1].y + t * points[2].y;

        x3.x = (1 - t) * points[2].x + t * points[3].x;
        x3.y = (1 - t) * points[2].y + t * points[3].y;

        y1.x = (1 - t) * x1.x + t * x2.x;
        y1.y = (1 - t) * x1.y + t * x2.y;

        y2.x = (1 - t) * x2.x + t * x3.x;
        y2.y = (1 - t) * x2.y + t * x3.y;

        p.x = (1 - t) * y1.x + t * y2.x;
        p.y = (1 - t) * y1.y + t * y2.y;
        
        return p;
    }
} // namespace VCX::Labs::Drawing2D