#include "common/RayTracer.h"
#include "common/Scene/Scene.h"
#include "common/Scene/Camera/Camera.h"
#include "common/Scene/Geometry/Ray/Ray.h"
#include "common/Intersection/IntersectionState.h"
#include "common/Output/ImageWriter.h"
#include "common/Rendering/Renderer.h"

#include "common/Scene/Camera/Perspective/PerspectiveCamera.h"

#include "common/core.h"
#include "assimp/material.h"

#include "common/Scene/Geometry/Primitives/Triangle/Triangle.h"

// #define WIDTH 3840
// #define HEIGHT 2160
#define WIDTH 480
#define HEIGHT 270

#define EX 500
#define EY -9700
#define EZ -3000
#define ER 10000

#define SUN_X 0.324f
#define SUN_Y 0.229f


struct MagicIntersection {
    bool intersected;
    glm::vec3 normal;
    glm::vec2 uv;
    float atmo;
};

std::shared_ptr<Texture2D> wmask;
std::shared_ptr<Texture2D> eland;
std::shared_ptr<Texture2D> eclou[2];
std::shared_ptr<Texture2D> eclou_normal[2];

MagicIntersection magic_intersect(Ray *ray)
{
    glm::vec3 ray_pos = ray->GetRayPosition(0);
    glm::vec3 ray_dir = ray->GetRayDirection();
    glm::vec3 ray_dir_norm = glm::normalize(ray_dir);

    glm::vec3 magic_pos(EX, EY, EZ);
    glm::vec3 magic_pos_raysp = magic_pos - ray_pos;
    float ray_T = glm::dot(magic_pos_raysp, ray_dir_norm);
    glm::vec3 proj = ray_T * ray_dir_norm;
    glm::vec3 diff = magic_pos_raysp - proj;

    float difflen = glm::length(diff);
    bool intersected = ray_T > 0 && difflen < ER;
    MagicIntersection inter = {intersected};
    inter.atmo = (difflen - ER) / 100.f;
    if (intersected) {
        float otherleg = sqrt(ER * ER - difflen * difflen);
        glm::vec3 int_pt = proj - ray_dir_norm * otherleg;
        glm::vec3 rad_vec = int_pt - magic_pos_raysp;
        glm::vec3 normal = glm::normalize(rad_vec);
        inter.normal = normal;

        float uvx = atan2f(normal.z, normal.y) / 2 / M_PI;
        float uvy = asinf(normal.x) / M_PI + 0.5;
        glm::vec2 uv(fmodf(uvx + 0.235, 1.0), uvy);
        inter.uv = uv;
    }
    return inter;
}

glm::vec3 magic_hugeland(glm::vec2 uv) {
    float x = fmodf(uv.x * 4.f - 0.5f, 1.f);
    float y = fmodf(uv.y * 2.f - 0.5f, 1.f);
    glm::vec3 samp = glm::vec3(eland->Sample(glm::vec2(x, y)));
    return samp;
}

float magic_watermask(glm::vec2 uv) {
    float x = fmodf(uv.x * 4.f - 0.5f, 1.f);
    float y = fmodf(uv.y * 2.f - 0.5f, 1.f);
    glm::vec3 samp = glm::vec3(wmask->Sample(glm::vec2(x, y)));
    return (samp.r + samp.g + samp.b) / 3.f;
}

glm::vec3 magic_clouds(glm::vec2 uv) {
    int c = (int) (uv.x * 64.f) % 2;
    float x = fmodf(uv.x * 64.f, 1.f);
    float y = fmodf(uv.y * 32.f, 1.f);
    glm::vec4 samp = eclou[c]->Sample(glm::vec2(x, y));
    return glm::vec3(samp);
}

glm::vec3 magic_cloudnormal(glm::vec2 uv) {
    int c = (int) (uv.x * 64.f) % 2;
    float x = fmodf(uv.x * 64.f, 1.f);
    float y = fmodf(uv.y * 32.f, 1.f);
    glm::vec3 samp = glm::vec3(eclou_normal[c]->Sample(glm::vec2(x, y)));
    return (samp - glm::vec3(0.5f, 0.5f, 0.5f)) * glm::vec3(-1.f, 1.f, 1.f);
}

std::shared_ptr<Camera> make_camera() {
    std::shared_ptr<PerspectiveCamera> camera = std::make_shared<PerspectiveCamera>((float) WIDTH / HEIGHT, 45.f);
    camera->SetZFar(1e20);
    return camera;
}

std::shared_ptr<SceneObject> make_soyuz() {
    std::shared_ptr<BlinnPhongMaterial> material = std::make_shared<BlinnPhongMaterial>();
    material->SetDiffuse(glm::vec3(1.f, 1.f, 1.f) * .5f);
    material->SetSpecular(glm::vec3(1.f, 1.f, 1.f) * .8f, 0.5f);
    material->SetTexture("diffuseTexture", TextureLoader::LoadTexture("soyuz/Soyuz Diffuse Color.png"));

    std::vector<std::shared_ptr<MeshObject>> mesh = MeshLoader::LoadMesh("soyuz/soyuz.obj");
    for (auto m: mesh) m->SetMaterial(material);

    std::shared_ptr<SceneObject> object = std::make_shared<SceneObject>();
    for (auto m: mesh) object->AddMeshObject(m);
    object->MultScale(2.2f);
    object->SetPosition(glm::vec3(200.f, -70.f, -500.f) + glm::vec3(-50.f, 20.f, -50.f) * 2.f);
    object->Rotate(glm::vec3(1.f, 0.f, 0.f), -0.5f * PI);
    object->Rotate(glm::vec3(0.f, 1.f, 0.f), PI / 6);
    object->Rotate(glm::vec3(1.f, 0.f, 0.f), 0.05f * PI);

    object->CreateAccelerationData(AccelerationTypes::BVH);
    object->ConfigureAccelerationStructure([](AccelerationStructure* genericAccelerator) {
        BVHAcceleration* accelerator = dynamic_cast<BVHAcceleration*>(genericAccelerator);
        accelerator->SetMaximumChildren(2);
        accelerator->SetNodesOnLeaves(2);
    });

    object->ConfigureChildMeshAccelerationStructure([](AccelerationStructure* genericAccelerator) {
        BVHAcceleration* accelerator = dynamic_cast<BVHAcceleration*>(genericAccelerator);
        accelerator->SetMaximumChildren(2);
        accelerator->SetNodesOnLeaves(2);
    });

    return object;
}

std::shared_ptr<SceneObject> make_iss() {
    std::vector<std::shared_ptr<aiMaterial>> mats;
    std::vector<std::shared_ptr<MeshObject>> mesh = MeshLoader::LoadMesh("iss/ISSComplete.fbx", &mats);
    std::shared_ptr<SceneObject> object = std::make_shared<SceneObject>();
    for (int i = 0; i < mesh.size(); i += 1) {
        std::shared_ptr<BlinnPhongMaterial> mat = std::make_shared<BlinnPhongMaterial>();
        mat->SetDiffuse(glm::vec3(.5f, .5f, .5f));
        mat->LoadMaterialFromAssimp(mats[i]);
        if (i == 0 || i == 1 || i == 2) {
            mat->SetSpecular(glm::vec3(1.f, .8f, .6f), .1f);
        }
        mesh[i]->SetMaterial(mat);
        object->AddMeshObject(mesh[i]);
    }

    object->MultScale(6.2f);
    object->SetPosition(glm::vec3(180.f, -75.f, -500.f));
    object->Rotate(glm::vec3(1.f, 0.f, 0.f), -0.5f * PI);
    object->Rotate(glm::vec3(0.f, 1.f, 0.f), PI / 5);
    object->Rotate(glm::vec3(1.f, 0.f, 0.f), 0.05f * PI);

    object->CreateAccelerationData(AccelerationTypes::BVH);
    object->ConfigureAccelerationStructure([](AccelerationStructure* genericAccelerator) {
        BVHAcceleration* accelerator = dynamic_cast<BVHAcceleration*>(genericAccelerator);
        accelerator->SetMaximumChildren(2);
        accelerator->SetNodesOnLeaves(2);
    });

    object->ConfigureChildMeshAccelerationStructure([](AccelerationStructure* genericAccelerator) {
        BVHAcceleration* accelerator = dynamic_cast<BVHAcceleration*>(genericAccelerator);
        accelerator->SetMaximumChildren(2);
        accelerator->SetNodesOnLeaves(2);
    });

    return object;
}

std::shared_ptr<Scene> make_scene(glm::vec3 sunpos) {
    std::shared_ptr<SceneObject> iss = make_iss();
    std::shared_ptr<SceneObject> soyuz = make_soyuz();

    std::shared_ptr<Scene> scene = std::make_shared<Scene>();
    scene->AddSceneObject(iss);
    scene->AddSceneObject(soyuz);
    scene->GenerateAccelerationData(AccelerationTypes::BVH);

    // Lights
    std::shared_ptr<Light> pointLight = std::make_shared<PointLight>();
    pointLight->SetPosition(sunpos);
    pointLight->SetLightColor(glm::vec3(1.f, 1.f, 1.f));
    scene->AddLight(pointLight);

    std::shared_ptr<Light> pointLight2 = std::make_shared<PointLight>();
    pointLight2->SetPosition(glm::vec3(-300.f, -75.f, -500.f));
    pointLight2->SetLightColor(glm::vec3(.2f, .2f, .2f));
    scene->AddLight(pointLight2);

    scene->Finalize();

    return scene;
}

void RayTracer::Run()
{
    std::shared_ptr<Camera> camera = make_camera();

    glm::vec2 sun_coords = glm::vec2(SUN_X, SUN_Y);
    glm::vec3 sun_dir = glm::normalize(camera->GenerateRayForNormalizedCoordinates(sun_coords)->GetRayDirection());
    float sun_int = 8.f;

    std::shared_ptr<Scene> scene = make_scene(camera->GenerateRayForNormalizedCoordinates(sun_coords)->GetRayPosition(10000));
    std::shared_ptr<Renderer> renderer = std::make_shared<BackwardRenderer>(scene);

    std::cout << "land" << std::endl;
    eland = TextureLoader::LoadTexture("earth/land.jpg");
    std::cout << "water" << std::endl;
    wmask = TextureLoader::LoadTexture("earth/water.jpg");
    std::cout << "W" << std::endl;
    eclou[0] = TextureLoader::LoadTexture("earth/cloud.W.jpg" /*"earth/cloud.W.2001210.21600x21600.jpg"*/);
    eclou_normal[0] = TextureLoader::LoadTexture("earth/cloud.W.normal.png");
    std::cout << "E" << std::endl;
    eclou[1] = TextureLoader::LoadTexture("earth/cloud.E.jpg");
    eclou_normal[1] = TextureLoader::LoadTexture("earth/cloud.E.normal.png");
    std::cout << "done" << std::endl;

    // Prepare for Output
    ImageWriter imageWriter("output.png", WIDTH, HEIGHT);

    for (int r = 0; r < HEIGHT; ++r) {
        for (int c = 0; c < WIDTH; ++c) {
            glm::vec3 sampleColor;

            glm::vec2 normalizedCoordinates((float) c / WIDTH, (float) r / HEIGHT);
            std::shared_ptr<Ray> cameraRay = camera->GenerateRayForNormalizedCoordinates(normalizedCoordinates);
            assert(cameraRay);

            // Solar flare brightness. Drawn at the end.
            float x = c;
            float y = r;
            glm::vec2 flaredist = glm::abs(glm::vec2((x / WIDTH - SUN_X) * 0.8f, y / HEIGHT - SUN_Y));
            float flare = 1.f / glm::max(0.01f, powf(flaredist.x, 0.7) + powf(flaredist.y, 0.7f));
            float funnysig = 1.f - 1.f / (1.f + expf(-6 * glm::length(flaredist) + 6));
            flare *= funnysig;

            // Sample sphere.
            MagicIntersection mi = magic_intersect(cameraRay.get());

            glm::vec3 ray_dir = glm::normalize(cameraRay->GetRayDirection());

            if (mi.intersected) {
                glm::vec3 landColor = magic_hugeland(mi.uv);
                
                // exposure comp
                float expo = sun_int * glm::max(0.f, glm::dot(mi.normal, sun_dir) + 0.1f); //expf(-1.f + 0.4 * powf(1.f - glm::dot(mi.normal, -ray_dir), 3.f));
                landColor *= expo;

                float surf_dot = glm::dot(ray_dir, mi.normal);
                float spec = glm::max(0.f, powf(glm::dot(sun_dir, ray_dir - surf_dot * mi.normal), 15.f));
                float watery = magic_watermask(mi.uv);
                float fresnel = glm::max(powf(1.03f + surf_dot, 7.f), 0.f);

                sampleColor += landColor;
                sampleColor += watery * (fresnel + 0.7f * spec) * glm::vec3(0.8f, 0.9f, 1.f);

                for (int i = 1; i <= 100; i += 1) {
                    glm::vec2 ofs = (float) i * glm::vec2(0.015, 0.05);
                    glm::vec3 right(1.f, 0.f, 0.f);
                    glm::vec3 back = glm::cross(right, mi.normal);
                    glm::mat3 cloutrans = glm::mat3(back, right, mi.normal);
                    glm::vec3 cloudColor = magic_clouds(mi.uv + ofs);
                    float cloudAlpha = powf((cloudColor.r + cloudColor.g + cloudColor.b) / 3.f, 0.6f);
                    cloudColor *= glm::vec3(0.7f, 0.9f, 1.f);
                    glm::vec3 cloudNormal = cloutrans * magic_cloudnormal(mi.uv + ofs);
                    float clou_dot = glm::dot(cloudNormal, ray_dir);
                    float cloudiff = powf(glm::max(0.f, glm::dot(cloudNormal, sun_dir) + 0.5f), 2.f);
                    float clouspec = spec + glm::max(0.f, powf(glm::dot(sun_dir, ray_dir - clou_dot * mi.normal), 15.f));
                    float cloufresnel = powf(1.f + surf_dot + clou_dot, 8.f);
                    // float cloud_expo = 2.f * cloudiff * cloufresnel + 2.f * cloufresnel + fresnel + 2.f * clouspec;
                    float cloud_expo = 1.f * (spec + fresnel) * (cloudiff + cloufresnel + clouspec);
                    cloudColor *= (0.5f + 0.5f * i / 100.f) * cloud_expo;
                    cloudColor += glm::clamp(5.f * powf(spec, 0.2) * (cloudiff + cloufresnel) - 2.f, 0.f, 1.f) * glm::vec3(1.f, .85f, .6f);
                    sampleColor += cloudAlpha * (cloudColor - sampleColor);
                }

                float atmothick = expf(mi.atmo / 4.0);
                glm::vec3 batmocol = 1.4f * powf(fresnel, .4f) * glm::vec3(0.45f, 0.5f, 0.65f) + expf(mi.atmo * 8.f) * glm::vec3(1.f, 1.f, 1.f);
                glm::vec3 ratmocol = 1.4f * powf(fresnel, .4f) * glm::vec3(0.7f, 0.6f, 0.5f) + expf(mi.atmo * 8.f) * glm::vec3(1.f, 1.f, 1.f);
                float coeff = powf(spec, 0.8f);
                glm::vec3 atmocol = coeff * ratmocol + (1.f - coeff) * batmocol;
                sampleColor += atmothick * (atmocol - sampleColor);
            }

            // Halo
            if (mi.atmo > 0) {
                sampleColor += 2.f * expf(-mi.atmo * 3.f) * glm::vec3(0.3f, 0.5f, 0.8f);
            }

            // Sample scene.
            IntersectionState rayIntersection(1, 0);
            rayIntersection.remainingReflectionBounces = 5;
            bool didHitScene = scene->Trace(cameraRay.get(), &rayIntersection);

            // Use the intersection data to compute the BRDF response.
            if (didHitScene) {
                sampleColor = renderer->ComputeSampleColor(rayIntersection, *cameraRay.get());
            }

            // Sun flare.
            sampleColor += 0.2f * (flare) * glm::vec3(0.6f, 0.7f, 0.8f);

            imageWriter.SetPixelColor(sampleColor, c, r);
        }
    }

    // Now copy whatever is in the HDR data and store it in the bitmap that we will save (aka everything will get clamped to be [0.0, 1.0]).
    imageWriter.CopyHDRToBitmap();

    // Save image.
    imageWriter.SaveImage();
}

// void RayTracer::Run()
// {
//     // Scene Setup -- Generate the camera and scene.
//     std::shared_ptr<Camera> currentCamera = storedApplication->CreateCamera();
//     std::shared_ptr<Scene> currentScene = storedApplication->CreateScene();
//     std::shared_ptr<ColorSampler> currentSampler = storedApplication->CreateSampler();
//     std::shared_ptr<Renderer> currentRenderer = storedApplication->CreateRenderer(currentScene, currentSampler);
//     assert(currentScene && currentCamera && currentSampler && currentRenderer);

//     currentSampler->InitializeSampler(storedApplication.get(), currentScene.get());

//     // Scene preprocessing -- generate acceleration structures, etc.
//     // After this call, we are guaranteed that the "acceleration" member of the scene and all scene objects within the scene will be non-NULL.
//     currentScene->GenerateDefaultAccelerationData();
//     currentScene->Finalize();

//     currentRenderer->InitializeRenderer();

//     // Prepare for Output
//     const glm::vec2 currentResolution = storedApplication->GetImageOutputResolution();
//     ImageWriter imageWriter(storedApplication->GetOutputFilename(), static_cast<int>(currentResolution.x), static_cast<int>(currentResolution.y));

//     // Perform forward ray tracing
//     const int maxSamplesPerPixel = storedApplication->GetSamplesPerPixel();
//     assert(maxSamplesPerPixel >= 1);

//     for (int r = 0; r < static_cast<int>(currentResolution.y); ++r) {
//         for (int c = 0; c < static_cast<int>(currentResolution.x); ++c) {
//             imageWriter.SetPixelColor(currentSampler->ComputeSamplesAndColor(maxSamplesPerPixel, 2, [&](glm::vec3 inputSample) {
//                 const glm::vec3 minRange(-0.5f, -0.5f, 0.f);
//                 const glm::vec3 maxRange(0.5f, 0.5f, 0.f);
//                 const glm::vec3 sampleOffset = (maxSamplesPerPixel == 1) ? glm::vec3(0.f, 0.f, 0.f) : minRange + (maxRange - minRange) * inputSample;

//                 glm::vec2 normalizedCoordinates(static_cast<float>(c) + sampleOffset.x, static_cast<float>(r) + sampleOffset.y);
//                 normalizedCoordinates /= currentResolution;

//                 // Construct ray, send it out into the scene and see what we hit.
//                 std::shared_ptr<Ray> cameraRay = currentCamera->GenerateRayForNormalizedCoordinates(normalizedCoordinates);
//                 assert(cameraRay);

//                 IntersectionState rayIntersection(storedApplication->GetMaxReflectionBounces(), storedApplication->GetMaxRefractionBounces());
//                 bool didHitScene = currentScene->Trace(cameraRay.get(), &rayIntersection);

//                 // Use the intersection data to compute the BRDF response.
//                 glm::vec3 sampleColor;
//                 if (didHitScene) {
//                     sampleColor = currentRenderer->ComputeSampleColor(rayIntersection, *cameraRay.get());
//                 }
//                 return sampleColor;
//             }), c, r);
//         }
//     }

//     // Apply post-processing steps (i.e. tone-mapper, etc.).
//     storedApplication->PerformImagePostprocessing(imageWriter);

//     // Now copy whatever is in the HDR data and store it in the bitmap that we will save (aka everything will get clamped to be [0.0, 1.0]).
//     imageWriter.CopyHDRToBitmap();

//     // Save image.
//     imageWriter.SaveImage();
// }