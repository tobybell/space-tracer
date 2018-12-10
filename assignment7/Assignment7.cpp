#include "assignment7/Assignment7.h"
#include "common/core.h"
#include "assimp/material.h"

std::shared_ptr<Camera> make_space_camera(glm::vec2 resolution) {
    std::shared_ptr<PerspectiveCamera> camera = std::make_shared<PerspectiveCamera>(resolution.x / resolution.y, 60.f);
    camera->SetZFar(1e20);

    camera->Translate(glm::vec3(-200.f, -100.f, 300.f));
    camera->Rotate(glm::vec3(camera->GetRightDirection()), 0.25f);
    camera->Rotate(glm::vec3(camera->GetUpDirection()), -0.85f);
    camera->Rotate(glm::vec3(camera->GetRightDirection()), 0.11f);
    camera->Translate(100.f * glm::vec3(camera->GetForwardDirection()));

    return camera;
}

std::shared_ptr<Camera> Assignment7::CreateCamera() const
{
    const glm::vec2 resolution = GetImageOutputResolution();
    return make_space_camera(resolution);

    std::shared_ptr<Camera> camera = std::make_shared<PerspectiveCamera>(resolution.x / resolution.y, 26.6f);
    camera->SetPosition(glm::vec3(0.f, -4.1469f, 0.73693f));
    camera->Rotate(glm::vec3(1.f, 0.f, 0.f), PI / 2.f);
    return camera;
}


// Assignment 7 Part 1 TODO: Change the '1' here.
// 0 -- Naive.
// 1 -- BVH.
// 2 -- Grid.
#define ACCELERATION_TYPE 1

std::shared_ptr<SceneObject> make_iss() {
    std::shared_ptr<BlinnPhongMaterial> material = std::make_shared<BlinnPhongMaterial>();
    material->SetDiffuse(glm::vec3(0.8f, 0.8f, 0.8f));
    material->SetSpecular(glm::vec3(1.f, 1.f, 1.f), 0.5);

    std::vector<std::shared_ptr<MeshObject>> mesh = MeshLoader::LoadMesh("ISSComplete1.fbx");
    for (auto m: mesh) m->SetMaterial(material);

    std::shared_ptr<SceneObject> object = std::make_shared<SceneObject>();
    for (auto m: mesh) object->AddMeshObject(m);
    object->MultScale(6.2f);

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

std::shared_ptr<SceneObject> make_soyuz() {
    std::shared_ptr<BlinnPhongMaterial> material = std::make_shared<BlinnPhongMaterial>();
    material->SetDiffuse(glm::vec3(1.f, 1.f, 1.f));
    material->SetTexture("diffuseTexture", TextureLoader::LoadTexture("soyuz/Soyuz Diffuse Color.png"));
    material->SetTexture("specularTexture", TextureLoader::LoadTexture("soyuz/Soyuz Diffuse Color.png"));

    std::vector<std::shared_ptr<MeshObject>> mesh = MeshLoader::LoadMesh("soyuz/soyuz.obj");
    for (auto m: mesh) m->SetMaterial(material);

    std::shared_ptr<SceneObject> object = std::make_shared<SceneObject>();
    for (auto m: mesh) object->AddMeshObject(m);
    object->MultScale(2.2f);
    object->Translate(glm::vec3(50.0f, -50.0f, 150.0f));
    object->Rotate(glm::vec3(0.0f, 0.0f, 1.0f), PI / 2);

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

std::shared_ptr<SceneObject> make_earth() {
    std::shared_ptr<BlinnPhongMaterial> material = std::make_shared<BlinnPhongMaterial>();
    material->SetDiffuse(glm::vec3(2.f, 2.f, 2.f));
    material->SetSpecular(glm::vec3(.1f, .1f, .1f), .1f);
    material->SetTexture("diffuseTexture", TextureLoader::LoadTexture("earth/earth10.jpg"));
    // material->SetTexture("specularTexture", TextureLoader::LoadTexture("earth/earth10.jpg"));

    std::vector<std::shared_ptr<MeshObject>> mesh = MeshLoader::LoadMesh("earth/earth.obj");
    for (auto m: mesh) m->SetMaterial(material);

    std::shared_ptr<SceneObject> object = std::make_shared<SceneObject>();
    for (auto m: mesh) object->AddMeshObject(m);
    object->MultScale(1500.0f);
    object->Rotate(glm::vec3(SceneObject::GetWorldRight()), PI / 3);
    object->Translate(glm::vec3(0.f, -1600.f, 0.f));

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

std::shared_ptr<Scene> make_space_scene() {
    std::shared_ptr<SceneObject> iss = make_iss();
    std::shared_ptr<SceneObject> soyuz = make_soyuz();
    std::shared_ptr<SceneObject> earth = make_earth();

    std::shared_ptr<Scene> scene = std::make_shared<Scene>();
    scene->AddSceneObject(iss);
    scene->AddSceneObject(soyuz);
    scene->AddSceneObject(earth);
    scene->GenerateAccelerationData(AccelerationTypes::BVH);

    // Lights
    std::shared_ptr<Light> pointLight = std::make_shared<PointLight>();
    pointLight->SetPosition(glm::vec3(0.01909f, 100.f, 100.f));
    pointLight->SetLightColor(glm::vec3(1.f, 1.f, 1.f));
    scene->AddLight(pointLight);

    // Lights
    std::shared_ptr<Light> pointLight2 = std::make_shared<PointLight>();
    pointLight2->SetPosition(glm::vec3(0.01909f, 500.f, -100.f));
    pointLight2->SetLightColor(glm::vec3(1.f, 1.f, 1.f));
    scene->AddLight(pointLight2);

    return scene;
}


std::shared_ptr<Scene> Assignment7::CreateScene() const
{
    return make_space_scene();
    std::shared_ptr<Scene> newScene = std::make_shared<Scene>();

    // Material
    std::shared_ptr<BlinnPhongMaterial> cubeMaterial = std::make_shared<BlinnPhongMaterial>();
    cubeMaterial->SetDiffuse(glm::vec3(1.f, 1.f, 1.f));
    cubeMaterial->SetSpecular(glm::vec3(0.6f, 0.6f, 0.6f), 40.f);
    cubeMaterial->SetReflectivity(0.3f);

    // Objects
    std::vector<std::shared_ptr<aiMaterial>> loadedMaterials;
    std::vector<std::shared_ptr<MeshObject>> cubeObjects = MeshLoader::LoadMesh("CornellBox/CornellBox-Assignment7-Alt.obj", &loadedMaterials);
    for (size_t i = 0; i < cubeObjects.size(); ++i) {
        std::shared_ptr<Material> materialCopy = cubeMaterial->Clone();
        materialCopy->LoadMaterialFromAssimp(loadedMaterials[i]);
        cubeObjects[i]->SetMaterial(materialCopy);

        std::shared_ptr<SceneObject> cubeSceneObject = std::make_shared<SceneObject>();
        cubeSceneObject->AddMeshObject(cubeObjects[i]);
        cubeSceneObject->Rotate(glm::vec3(1.f, 0.f, 0.f), PI / 2.f);

        cubeSceneObject->CreateAccelerationData(AccelerationTypes::BVH);
        cubeSceneObject->ConfigureAccelerationStructure([](AccelerationStructure* genericAccelerator) {
            BVHAcceleration* accelerator = dynamic_cast<BVHAcceleration*>(genericAccelerator);
            accelerator->SetMaximumChildren(2);
            accelerator->SetNodesOnLeaves(2);
        });

        cubeSceneObject->ConfigureChildMeshAccelerationStructure([](AccelerationStructure* genericAccelerator) {
            BVHAcceleration* accelerator = dynamic_cast<BVHAcceleration*>(genericAccelerator);
            accelerator->SetMaximumChildren(2);
            accelerator->SetNodesOnLeaves(2);
        });
        newScene->AddSceneObject(cubeSceneObject);
    }

    // Lights
    std::shared_ptr<Light> pointLight = std::make_shared<PointLight>();
    pointLight->SetPosition(glm::vec3(0.01909f, 0.0101f, 1.97028f));
    pointLight->SetLightColor(glm::vec3(1.f, 1.f, 1.f));

#if ACCELERATION_TYPE == 0
    newScene->GenerateAccelerationData(AccelerationTypes::NONE);
#elif ACCELERATION_TYPE == 1
    newScene->GenerateAccelerationData(AccelerationTypes::BVH);
#else
    UniformGridAcceleration* accelerator = dynamic_cast<UniformGridAcceleration*>(newScene->GenerateAccelerationData(AccelerationTypes::UNIFORM_GRID));
    assert(accelerator);
    // Assignment 7 Part 2 TODO: Change the glm::ivec3(10, 10, 10) here.
    accelerator->SetSuggestedGridSize(glm::ivec3(3, 3, 3));
#endif
    newScene->AddLight(pointLight);

    return newScene;

}
std::shared_ptr<ColorSampler> Assignment7::CreateSampler() const
{
    std::shared_ptr<JitterColorSampler> jitter = std::make_shared<JitterColorSampler>();
    jitter->SetGridSize(glm::ivec3(1.f, 1.f, 1.f));
    return jitter;

    std::shared_ptr<SimpleAdaptiveSampler> sampler = std::make_shared<SimpleAdaptiveSampler>();
    sampler->SetInternalSampler(jitter);
    sampler->SetEarlyExitParameters(1.f * SMALL_EPSILON, 16);
    return sampler;
}

std::shared_ptr<class Renderer> Assignment7::CreateRenderer(std::shared_ptr<Scene> scene, std::shared_ptr<ColorSampler> sampler) const
{
    return std::make_shared<BackwardRenderer>(scene, sampler);
}

int Assignment7::GetSamplesPerPixel() const
{
    return 16;
}

bool Assignment7::NotifyNewPixelSample(glm::vec3 inputSampleColor, int sampleIndex)
{
    return true;
}

int Assignment7::GetMaxReflectionBounces() const
{
    return 2;
}

int Assignment7::GetMaxRefractionBounces() const
{
    return 4;
}

glm::vec2 Assignment7::GetImageOutputResolution() const
{
    return glm::vec2(640.f, 480.f);
}
