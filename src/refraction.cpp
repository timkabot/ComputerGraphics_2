#include "refraction.h"

Refraction::Refraction(short width, short height) :Reflection(width, height)
{
    raytracing_depth = 3;
}

Refraction::~Refraction()
{
}

Payload Refraction::Hit(const Ray& ray, const IntersectableData& data, const MaterialTriangle* triangle, const unsigned int max_raytrace_depth) const
{
    if (triangle == nullptr) return Miss(ray);
    Payload payload;
    payload.color = triangle->emissive_color;

    float3 X = ray.position + ray.direction * data.t;
    float3 N = triangle->GetNormal(data.baricentric);

    if (triangle->reflectiveness)
    {
        Ray reflection_ray(X, ray.direction - 2.0f * dot(N, ray.direction) * N);
        return TraceRay(reflection_ray, max_raytrace_depth - 1);
    }

    if (triangle->reflectiveness_and_transparency)
    {
        float kr = 1.f;
        float cosi = std::max(-1.f, std::min(1.f, dot(ray.direction, N)));
        float etai = 1.f;
        float etat = triangle->ior;
        if (cosi > 0.f)
        {
            std::swap(etai, etat);
        }
        float sint = etai / etat * sqrtf(std::max(0.f, 1 - cosi * cosi));
        if (sint >= 1.f)
        {
            kr = 1.f;
        }
        else
        {
            float cost = sqrtf(std::max(0.f, 1 - sint * sint));
            float cosit = cosi;
            float cosi = fabs(cosit);
            float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
            float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
            kr = (Rs * Rs + Rp * Rp) / 2.f;
        }

        bool outside = (dot(ray.direction, N) < 0);
        float3 bias = 0.001f * N;
        Payload refraction_payload;

        if (kr < 1.f)
        {
            float cosi = std::max(-1.f, std::min(1.f, dot(ray.direction, N)));
            float etai = 1.f;
            float etat = triangle->ior;
            if (cosi > 0.f)
            {
                std::swap(etai, etat);
            }
            cosi = fabs(cosi);
            float eta = etai / etat;
            float k = 1.f - eta * eta * (1.f - cosi * cosi);
            float3 refraction_direction{ 0,0,0 };
            if (k >= 0.f)
            {
                refraction_direction = eta * ray.direction + (eta * cosi - sqrtf(k)) * N;
            }
            Ray refraction_ray(outside ? X - bias : X + bias, refraction_direction);
            refraction_payload = TraceRay(refraction_ray, max_raytrace_depth - 1);
        }

        Ray reflection_ray(outside ? X + bias : X - bias, ray.direction - 2.0f * dot(N, ray.direction) * N);
        Payload reflection_payload = TraceRay(reflection_ray, max_raytrace_depth - 1);

        Payload final;
        final.color = reflection_payload.color * kr + refraction_payload.color * (1.f - kr);
        return final;
    }

    // Diffuse
    for (auto& light : lights)
    {
        Ray to_light(X, light->position - X);
        float to_light_distance = length(light->position - X);

        float t = TraceShadowRay(to_light, to_light_distance);
        if (fabs(t - to_light_distance) > t_min)
        {
            continue;
        }

        payload.color += light->color * triangle->diffuse_color *
            std::max(0.0f, dot(N, to_light.direction));

        // Specular
        float3 reflection_direction = 2.0f * dot(N, to_light.direction) * N - to_light.direction;

        payload.color += light->color * triangle->specular_color *
            powf(std::max(0.0f, dot(ray.direction, reflection_direction)),
                triangle->specular_exponent);
    }

    return payload;
}
