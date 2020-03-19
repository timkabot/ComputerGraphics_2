#include "reflection.h"

Reflection::Reflection(short width, short height) :ShadowRays(width, height)
{
}

Reflection::~Reflection()
{
}

Payload Reflection::Hit(const Ray& ray, const IntersectableData& data, const MaterialTriangle* triangle, const unsigned int raytrace_depth) const
{
    if (raytrace_depth <= 0)
    {
        return Miss(ray);
    }
    if (triangle == nullptr) return Miss(ray);
    Payload payload;
    payload.color = triangle->emissive_color;

    float3 X = ray.position + ray.direction * data.t;
    float3 N = triangle->GetNormal(data.baricentric);

    if (triangle->reflectiveness)
    {
        float3 reflection_direction = ray.direction - 2.0f * dot(N, ray.direction) * N;
        Ray reflection_ray(X, reflection_direction);
        return TraceRay(reflection_ray, raytrace_depth - 1);
    }

    // Diffuse
    for (auto light : lights)
    {
        Ray to_light(X, light->position - X);
        float to_light_distance = length(light->position - X);

        float t = TraceShadowRay(to_light, to_light_distance);
        if (fabs(t - to_light_distance) > 1e-3f)
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
