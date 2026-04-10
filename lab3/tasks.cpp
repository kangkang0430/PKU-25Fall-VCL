#include "Labs/3-Rendering/tasks.h"

namespace VCX::Labs::Rendering {

    glm::vec4 GetTexture(Engine::Texture2D<Engine::Formats::RGBA8> const & texture, glm::vec2 const & uvCoord) {
        if (texture.GetSizeX() == 1 || texture.GetSizeY() == 1) return texture.At(0, 0);
        glm::vec2 uv      = glm::fract(uvCoord);
        uv.x              = uv.x * texture.GetSizeX() - .5f;
        uv.y              = uv.y * texture.GetSizeY() - .5f;
        std::size_t xmin  = std::size_t(glm::floor(uv.x) + texture.GetSizeX()) % texture.GetSizeX();
        std::size_t ymin  = std::size_t(glm::floor(uv.y) + texture.GetSizeY()) % texture.GetSizeY();
        std::size_t xmax  = (xmin + 1) % texture.GetSizeX();
        std::size_t ymax  = (ymin + 1) % texture.GetSizeY();
        float       xfrac = glm::fract(uv.x), yfrac = glm::fract(uv.y);
        return glm::mix(glm::mix(texture.At(xmin, ymin), texture.At(xmin, ymax), yfrac), glm::mix(texture.At(xmax, ymin), texture.At(xmax, ymax), yfrac), xfrac);
    }

    glm::vec4 GetAlbedo(Engine::Material const & material, glm::vec2 const & uvCoord) {
        glm::vec4 albedo       = GetTexture(material.Albedo, uvCoord);
        glm::vec3 diffuseColor = albedo;
        return glm::vec4(glm::pow(diffuseColor, glm::vec3(2.2)), albedo.w);
    }

    /******************* 1. Ray-triangle intersection *****************/
    bool IntersectTriangle(Intersection & output, Ray const & ray, glm::vec3 const & p1, glm::vec3 const & p2, glm::vec3 const & p3) {
        // your code here
        
        // ray.Direction -> D
        // ray.Origin -> O
        // p1 -> V0   p2 -> V1   p3 -> V2
        // O + tD = (1 − u − v) * V0 + u * V1 + v * V2 
        // [-D, V1 - V0, V2 - V0] * [t, u, v]^T = O - V0
        // t = 1 / (P * E1) * (Q * E2)
        // u = 1 / (P * E1) * (P * T)
        // v = 1 / (P * E1) * (Q * D)
        // 其中 E1 = V1 - V0    E2 = V2 - V0    P = D x E2    Q = T x E1
        glm::vec3 D  =  ray.Direction;
        glm::vec3 O  =  ray.Origin;
        glm::vec3 E1 =  p2 - p1;
        glm::vec3 E2 =  p3 - p1;
        glm::vec3 T  =  O  - p1;
        glm::vec3 P  =  cross(D, E2);
        glm::vec3 Q  =  cross(T, E1);
        float denominator = dot(P, E1);
        if(abs(denominator) < 1e-5) return false;

        float t =  dot(Q, E2) / denominator;

        float u =  dot(P,  T) / denominator;
        if(u < 0.0 || u > 1.0) return false;

        float v =  dot(Q,  D) / denominator;
        if(v < 0.0 || v + u > 1.0) return false;

        output.t = t;
        output.u = u;
        output.v = v;
        return true;
    }

    glm::vec3 RayTrace(const RayIntersector & intersector, Ray ray, int maxDepth, bool enableShadow) {
        glm::vec3 color(0.0f);
        glm::vec3 weight(1.0f);

        for (int depth = 0; depth < maxDepth; depth++) {
            auto rayHit = intersector.IntersectRay(ray);
            if (! rayHit.IntersectState) return color;
            const glm::vec3 pos       = rayHit.IntersectPosition;
            const glm::vec3 n         = rayHit.IntersectNormal;
            const glm::vec3 kd        = rayHit.IntersectAlbedo;
            const glm::vec3 ks        = rayHit.IntersectMetaSpec;
            const float     alpha     = rayHit.IntersectAlbedo.w;
            const float     shininess = rayHit.IntersectMetaSpec.w * 256;

            glm::vec3 result(0.0f);
            /******************* 2. Whitted-style ray tracing *****************/
            // your code here
            result += kd * intersector.InternalScene->AmbientIntensity;

            for (const Engine::Light & light : intersector.InternalScene->Lights) {
                glm::vec3 l;
                float     attenuation;
                /******************* 3. Shadow ray *****************/
                if (light.Type == Engine::LightType::Point) {
                    l           = light.Position - pos;
                    attenuation = 1.0f / glm::dot(l, l);
                    if (enableShadow) {
                        // your code here
                        auto shadow_hit = intersector.IntersectRay(Ray(pos, l));
                        while(shadow_hit.IntersectState == true && shadow_hit.IntersectAlbedo.w < 0.2f){ // 判断是否是在光源另一边的障碍物
                            shadow_hit = intersector.IntersectRay(Ray(shadow_hit.IntersectPosition, l));
                        }
                        if(shadow_hit.IntersectState){
                            glm::vec3 l1 = shadow_hit.IntersectPosition - light.Position;
                            if(dot(l, l1) < 0) continue; // 障碍物在光源和pos点之间
                        }
                    }
                } else if (light.Type == Engine::LightType::Directional) {
                    l           = light.Direction;
                    attenuation = 1.0f;
                    if (enableShadow) {
                        // your code here
                        auto shadow_hit = intersector.IntersectRay(Ray(pos, l));
                        while(shadow_hit.IntersectState == true && shadow_hit.IntersectAlbedo.w < 0.2f){
                            shadow_hit = intersector.IntersectRay(Ray(shadow_hit.IntersectPosition, l));
                        }
                        if(shadow_hit.IntersectState){ // 障碍物一定在光源和顶点之间
                            continue;
                        }
                    }
                }

                /******************* 2. Whitted-style ray tracing *****************/
                // your code here
                glm::vec3 normalize_viewdir     = normalize(ray.Direction);
                glm::vec3 normalize_normal      = normalize(n);
                glm::vec3 normalize_lightdir    = normalize(l);

                // diffuse
                glm::vec3 diffuse_light = kd * light.Intensity * attenuation * glm::max(glm::dot(normalize_normal, normalize_lightdir), 0.0f);

                // specular
                glm::vec3 normalize_halfdir = normalize(-normalize_viewdir + normalize_lightdir);
                glm::vec3 specular_light = ks * light.Intensity * attenuation * pow(glm::max(glm::dot(normalize_halfdir, normalize_normal), 0.0f), shininess);

                result += diffuse_light + specular_light;
            }

            if (alpha < 0.9) {
                // refraction
                // accumulate color
                glm::vec3 R = alpha * glm::vec3(1.0f);
                color += weight * R * result;
                weight *= glm::vec3(1.0f) - R;

                // generate new ray
                ray = Ray(pos, ray.Direction);
            } else {
                // reflection
                // accumulate color
                glm::vec3 R = ks * glm::vec3(0.5f);
                color += weight * (glm::vec3(1.0f) - R) * result;
                weight *= R;

                // generate new ray
                glm::vec3 out_dir = ray.Direction - glm::vec3(2.0f) * n * glm::dot(n, ray.Direction);
                ray               = Ray(pos, out_dir);
            }
        }

        return color;
    }
} // namespace VCX::Labs::Rendering
