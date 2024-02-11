//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"


void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }


    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    // because its path tracing and maxDepth is 1, so the depth will not be used
    Vector3f hitColor = Vector3f(0);
    Intersection hit = intersect(ray);
    if (hit.emit.norm() > 0)
        hitColor = Vector3f(1);
    if (!hit.happened) return hitColor;
    // Implement Path Tracing Algorithm here
    Vector3f wo = normalize(-ray.direction);
    Vector3f p = hit.coords;
    Vector3f N = normalize(hit.normal);
    /*
        1. contribution from the light source
        uniformly sample the light at x'
        L_dir = L_i * f_r * cos(theta) * cos(theta') / ||x - x'||^2 /pdf_light
    */ 
    Intersection interLight;
    float pdf_light = 0.f;
    sampleLight(interLight, pdf_light);
    Vector3f L_dir = Vector3f(0);
    
    Vector3f xx = interLight.coords;
    Vector3f NN = interLight.normal;
    Vector3f wi = normalize(xx - p);
    
    // check if the light is not blocked
    // => the distance = intersect(xx-p).distance
    if ((intersect(Ray(p, wi)).coords - xx).norm() < 0.01){
        L_dir = interLight.emit * hit.m->eval(wo, wi, N) * dotProduct(wi, N) * dotProduct(-wi, NN) 
                / std::pow((xx - p).norm(),2) / pdf_light;
    }
    
    
    /*  
        2. contribution from other reflectors
        uniformly sample the hemisphere
        check the RussianRoulette rate
        if the ray hit a non-emitting object,
            L_indir = castRay(-wi) * f_r * cos(theta) / pdf_hemi / RussianRoulette / pdf_hemi
    */

    Vector3f L_indir = Vector3f(0);
    if (get_random_float() < Scene::RussianRoulette){
        Vector3f wi = hit.m->sample(wo, N);
        float pdf_hemi = hit.m->pdf(wi, wo, N);
        if (pdf_hemi > 0.f){
            L_indir = castRay(Ray(p, wi), depth) * hit.m->eval(wi, wo, N) * dotProduct(wi, N)
                        / pdf_hemi / RussianRoulette;
        }
    }
    // std::cout << "L_dir: " << L_dir << " L_indir: " << L_indir << std::endl;
    return L_dir + L_indir;
}