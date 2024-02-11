#include <algorithm>
#include <cassert>
#include "BVH.hpp"

BVHAccel::BVHAccel(std::vector<Object*> p, int maxPrimsInNode,
                   SplitMethod splitMethod)
    : maxPrimsInNode(std::min(255, maxPrimsInNode)), splitMethod(splitMethod),
      primitives(std::move(p))
{
    if (splitMethod == SplitMethod::SAH)
        std::cout << "Building SAH BVH" << std::endl;
    else
        std::cout << "Building BVH with the naive method" << std::endl; 
    auto start = std::chrono::system_clock::now();
    // time_t start, stop;
    // time(&start);
    if (primitives.empty())
        return;
    
    root = recursiveBuild(primitives);
    // time(&stop);
    // double diff = difftime(stop, start);
    // int hrs = (int)diff / 3600;
    // int mins = ((int)diff / 60) - (hrs * 60);
    // int secs = (int)diff - (hrs * 3600) - (mins * 60);

    // printf(
    //     "\rBVH Generation complete: \nTime Taken: %i hrs, %i mins, %i secs\n\n",
    //     hrs, mins, secs);
    auto stop = std::chrono::system_clock::now();

    std::cout << "Render complete: \n";
    std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::hours>(stop - start).count() << " hours\n";
    std::cout << "          : " << std::chrono::duration_cast<std::chrono::minutes>(stop - start).count() << " minutes\n";
    std::cout << "          : " << std::chrono::duration_cast<std::chrono::seconds>(stop - start).count() << " seconds\n";
    std::cout << "          : " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << " milliseconds\n";
}

BVHBuildNode* BVHAccel::recursiveBuild(std::vector<Object*> objects)
{
    BVHBuildNode* node = new BVHBuildNode();

    // Compute bounds of all primitives in BVH node
    Bounds3 bounds;
    for (int i = 0; i < objects.size(); ++i)
        bounds = Union(bounds, objects[i]->getBounds());
    if (objects.size() == 1) {
        // Create leaf _BVHBuildNode_
        node->bounds = objects[0]->getBounds();
        node->object = objects[0];
        node->left = nullptr;
        node->right = nullptr;
        return node;
    }
    else if (objects.size() == 2) {
        node->left = recursiveBuild(std::vector{objects[0]});
        node->right = recursiveBuild(std::vector{objects[1]});

        node->bounds = Union(node->left->bounds, node->right->bounds);
        return node;
    }
    else {
        Bounds3 centroidBounds;
        for (int i = 0; i < objects.size(); ++i)
            centroidBounds =
                Union(centroidBounds, objects[i]->getBounds().Centroid());
        int dim = centroidBounds.maxExtent();
        switch (dim) {
        case 0:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().x <
                       f2->getBounds().Centroid().x;
            });
            break;
        case 1:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().y <
                       f2->getBounds().Centroid().y;
            });
            break;
        case 2:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().z <
                       f2->getBounds().Centroid().z;
            });
            break;
        }

        switch (splitMethod)
        {
            case SplitMethod::NAIVE:
            {
                // std::cout << "Building BVH with the naive method" << std::endl;
                auto beginning = objects.begin();
                auto middling = objects.begin() + (objects.size() / 2);
                auto ending = objects.end();

                auto leftshapes = std::vector<Object*>(beginning, middling);
                auto rightshapes = std::vector<Object*>(middling, ending);

                assert(objects.size() == (leftshapes.size() + rightshapes.size()));

                node->left = recursiveBuild(leftshapes);
                node->right = recursiveBuild(rightshapes);

                node->bounds = Union(node->left->bounds, node->right->bounds);
                
            }
            break;

            case SplitMethod::SAH:
            {
                // std::cout << "Building BVH with the SAH method" << std::endl;
                auto beginning = objects.begin();
                auto middling = objects.begin();
                auto ending = objects.end();
                int minCostInd = 0;
                float minCost = std::numeric_limits<float>::max();
                
                Bounds3 a;
                Bounds3 b;
                for(int i =0;i < objects.size()-1; ++i)
                {
                    for(int j = 0;j<i;j++)
                    {
                        a = Union(a,objects[j]->getBounds());
                    }
                    for(int j = i+1 ;j<objects.size()-1;j++)
                    {
                        b = Union(b,objects[j]->getBounds());
                    }
                    float tempcost = (i+1)* a.SurfaceArea()/bounds.SurfaceArea() + (objects.size()-i-1)* b.SurfaceArea()/bounds.SurfaceArea();
                    if(i==0) minCost = tempcost;
                    if(tempcost < minCost)
                    {
                        middling = objects.begin()+i;
                        minCost = tempcost;
                    }
                }

                auto leftshapes = std::vector<Object*> (beginning, middling);
                auto rightshapes = std::vector<Object*> (middling, ending);

                assert(objects.size() == (leftshapes.size() + rightshapes.size()));

                node->left = recursiveBuild(leftshapes);
                node->right = recursiveBuild(rightshapes);

                node->bounds = Union(node->left->bounds, node->right->bounds);
                
            }
            break;
        }
        
    }

    return node;
}

float BVHAccel::computeSAHCost(const std::vector<Object*> left,
                               const std::vector<Object*> right,
                               float S_N)
{
    float Ctrav = 0.5, Cisec = 1;
    // Cost = Ctrav + Cisec * SA/SN * NL + SB/SN * NR
    
    Bounds3 leftBounds, rightBounds;
    for (int i=0; i<left.size(); ++i){
        leftBounds = Union(leftBounds, left[i]->getBounds());
    }
    for (int i=0; i<right.size(); ++i){
        rightBounds = Union(rightBounds, right[i]->getBounds());
    }
    float S_A = leftBounds.SurfaceArea();
    float S_B = rightBounds.SurfaceArea();
    int N_L = left.size(), N_R = right.size();
    return Ctrav + Cisec * (S_A/S_N * N_L + S_B/S_N * N_R);
}

Intersection BVHAccel::Intersect(const Ray& ray) const
{
    Intersection isect;
    if (!root)
        return isect;
    isect = BVHAccel::getIntersection(root, ray);
    return isect;
}

Intersection BVHAccel::getIntersection(BVHBuildNode* node, const Ray& ray) const
{
    // TODO Traverse the BVH to find intersection
    Intersection isect;

    std::array<int, 3> dirIsNeg = {ray.direction.x < 0, ray.direction.y < 0,
                                   ray.direction.z < 0};
    if (!node->bounds.IntersectP(ray, ray.direction_inv, dirIsNeg)){
        return isect;
    }
    // if it is not a leaf node, the object is null
    if(node->object){
        return node->object->getIntersection(ray);
    }
    
    Intersection left = getIntersection(node->left, ray);
    Intersection right = getIntersection(node->right, ray);
    return left.distance < right.distance ? left : right;
}