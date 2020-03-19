#include "denoising.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <time.h>
#include <omp.h>
#include <random>

Denoising::Denoising(short width, short height) : AABB(width, height)
{
	raytracing_depth = 24;
}

Denoising::~Denoising()
{
}

void Denoising::Clear()
{
	history_buffer.resize(width * height);
	frame_buffer.resize(width * height);
}

Payload Denoising::Hit(const Ray& ray, const IntersectableData& data, const MaterialTriangle* triangle, const unsigned int max_raytrace_depth) const
{
    if (triangle == nullptr) return Miss(ray);
    Payload payload;
    payload.color = triangle->emissive_color;
    if (payload.color > float3{ 0,0,0 }) return payload;

    float3 X = ray.position + ray.direction * data.t;
    float3 N = triangle->GetNormal(data.baricentric);

    if (triangle->reflectiveness)
    {
        float3 reflection_direction = ray.direction - 2.0f * dot(N, ray.direction) * N;
        Ray reflection_ray(X, reflection_direction);
        return TraceRay(reflection_ray, max_raytrace_depth - 1);
    }

    // Diffuse
    const int num_secondary_rays = 1;
    float3 color;
    for (int i = 0; i < num_secondary_rays; i++)
    {
        float3 rand_direction = blue_noise[GetRandom(omp_get_thread_num() + clock())];
        if (dot(rand_direction, N) <= 0.f)
        {
            rand_direction = -rand_direction;
        }
        Ray to_light(X, rand_direction);

        Payload light_payload = TraceRay(to_light, max_raytrace_depth - 1);

        payload.color += light_payload.color * triangle->diffuse_color *
            std::max(0.0f, dot(N, to_light.direction));

        // Specular
        float3 reflection_direction = 2.0f * dot(N, to_light.direction) * N - to_light.direction;
        payload.color += light_payload.color * triangle->specular_color *
            powf(std::max(0.0f, dot(ray.direction, reflection_direction)),
                triangle->specular_exponent);
    }

    payload.color += color / num_secondary_rays;

    return payload;
}

void Denoising::SetHistory(unsigned short x, unsigned short y, float3 color)
{
	history_buffer[static_cast<size_t>(y)* static_cast<size_t>(width) + static_cast<size_t>(x)] = color;
}

float3 Denoising::GetHistory(unsigned short x, unsigned short y) const
{
	return history_buffer[static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)];
}


Payload Denoising::Miss(const Ray& ray) const
{
    return Payload();
}

int Denoising::GetRandom(const int thread_num) const
{
    static std::default_random_engine generator(thread_num);
    static std::uniform_int_distribution<int> distribution(0, 512 * 512);
    return distribution(generator);
}

void Denoising::DrawScene(int max_frame_number)
{
    camera.SetRenderTargetSize(width, height);
    for (int frame_number = 0; frame_number < max_frame_number; frame_number++)
    {
        std::cout << "Frame " << (frame_number + 1) << std::endl;
#pragma omp parallel for
        for (short x = 0; x < width; x++)
        {
#pragma omp parallel for
            for (short y = 0; y < height; y++)
            {
                Ray ray = camera.GetCameraRay(x, y);
                Payload payload = TraceRay(ray, raytracing_depth);
                SetPixel(x, y, payload.color);
                SetHistory(x, y, GetHistory(x, y) + payload.color);
            }
        }
    }

#pragma omp parallel for
    for (short x = 0; x < width; x++)
    {
#pragma omp parallel for
        for (short y = 0; y < height; y++)
        {
            SetPixel(x, y, GetHistory(x, y) / max_frame_number);
        }
    }
}

void Denoising::LoadBlueNoise(std::string file_name)
{
	int width, height, channels;
	unsigned char* img = stbi_load(file_name.c_str(), &width, &height, &channels, 0);
	for (int i = 0; i < width * height; i++) 
	{
        float3 pixel{
            (img[channels * i] - 128.f) / 128.f,
            (img[channels * i + 1] - 128.f) / 128.f,
            (img[channels * i + 2] - 128.f) / 128.f };
        blue_noise.push_back(pixel);
	}
}
