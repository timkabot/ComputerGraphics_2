#include "bvh.h"


BVH::BVH(short width, short height) :AABB(width, height)
{
	raytracing_depth = 4;
}

BVH::~BVH()
{
}

bool cmp(const Mesh& a, const Mesh& b)
{
	return a.aabb_max.y < b.aabb_max.y;
}

void BVH::BuildBVH()
{
	std::sort(meshes.begin(), meshes.end(), cmp);
	auto middle = meshes.begin();
	std::advance(middle, 2);
	std::vector<Mesh> left_half(meshes.begin(), middle);
	std::vector<Mesh> right_half(middle, meshes.end());

	TLAS left;
	for (auto& mesh : left_half)
	{
		left.AddMesh(mesh);
	}
	tlases.push_back(left);

	TLAS right;
	for (auto& mesh : right_half)
	{
		right.AddMesh(mesh);
	}
	tlases.push_back(right);
}

Payload BVH::TraceRay(const Ray& ray, const unsigned int max_raytrace_depth) const
{
	if (max_raytrace_depth <= 0) return Miss(ray);

	IntersectableData closest_data(t_max);
	MaterialTriangle const* closest_triange = nullptr;

	for (auto& tlas : tlases)
	{
		if (!tlas.AABBTest(ray)) continue;

		for (auto& mesh : meshes)
		{
			if (!mesh.AABBTest(ray)) continue;
			for (auto& object : mesh.Triangles())
			{
				IntersectableData data = object.Intersect(ray);
				if (data.t < closest_data.t && data.t > t_min)
				{
					closest_data = data;
					closest_triange = &object;
				}
			}
		}
	}

	if (closest_data.t < t_max)
	{
		return Hit(ray, closest_data, closest_triange, max_raytrace_depth);
	}

	return Miss(ray);
}

float BVH::TraceShadowRay(const Ray& ray, const float max_t) const
{
	IntersectableData closest_data(max_t);
	for (auto& tlas : tlases)
	{
		if (!tlas.AABBTest(ray)) continue;

		for (auto& mesh : meshes) {
			if (!mesh.AABBTest(ray)) continue;
			for (auto& object : mesh.Triangles())
			{
				IntersectableData data = object.Intersect(ray);
				if (data.t < closest_data.t && data.t > t_min)
				{
					return data.t;
				}
			}
		}
	}

	return max_t;
}

bool TLAS::AABBTest(const Ray& ray) const
{
	float3 invRaydir = float3(1.0) / ray.direction;
	float3 t0 = (aabb_max - ray.position) * invRaydir;
	float3 t1 = (aabb_min - ray.position) * invRaydir;
	float3 tmin = min(t0, t1);
	float3 tmax = max(t0, t1);
	return maxelem(tmin) <= minelem(tmax);
}

void TLAS::AddMesh(const Mesh mesh)
{
	if (meshes.empty())
	{
		aabb_max = mesh.aabb_max;
		aabb_min = mesh.aabb_min;
	}
	meshes.push_back(mesh);
	aabb_max = max(mesh.aabb_max, aabb_max);
	aabb_min = min(mesh.aabb_min, aabb_min);
}
