#include "common/Rendering/Renderer/Photon/PhotonMappingRenderer.h"
#include "common/Scene/Scene.h"
#include "common/Sampling/ColorSampler.h"
#include "common/Scene/Lights/Light.h"
#include "common/Scene/Geometry/Primitives/Primitive.h"
#include "common/Scene/Geometry/Mesh/MeshObject.h"
#include "common/Rendering/Material/Material.h"
#include "common/Intersection/IntersectionState.h"
#include "common/Scene/SceneObject.h"
#include "common/Scene/Geometry/Mesh/MeshObject.h"
#include "common/Rendering/Material/Material.h"
#include "glm/gtx/component_wise.hpp"

#define VISUALIZE_PHOTON_MAPPING 1

PhotonMappingRenderer::PhotonMappingRenderer(std::shared_ptr<class Scene> scene, std::shared_ptr<class ColorSampler> sampler):
    BackwardRenderer(scene, sampler), 
    diffusePhotonNumber(1000000),
    maxPhotonBounces(1000)
{
    srand(static_cast<unsigned int>(time(NULL)));
}

void PhotonMappingRenderer::InitializeRenderer()
{
    // Generate Photon Maps
    GenericPhotonMapGeneration(diffuseMap, diffusePhotonNumber);
    diffuseMap.optimise();
}

void PhotonMappingRenderer::GenericPhotonMapGeneration(PhotonKdtree& photonMap, int totalPhotons)
{
    float totalLightIntensity = 0.f;
    size_t totalLights = storedScene->GetTotalLights();
    for (size_t i = 0; i < totalLights; ++i) {
        const Light* currentLight = storedScene->GetLightObject(i);
        if (!currentLight) {
            continue;
        }
        totalLightIntensity += glm::length(currentLight->GetLightColor());
    }

    // Shoot photons -- number of photons for light is proportional to the light's intensity relative to the total light intensity of the scene.
    for (size_t i = 0; i < totalLights; ++i) {
        const Light* currentLight = storedScene->GetLightObject(i);
        if (!currentLight) {
            continue;
        }

        const float proportion = glm::length(currentLight->GetLightColor()) / totalLightIntensity;
        const int totalPhotonsForLight = static_cast<const int>(proportion * totalPhotons);
        const glm::vec3 photonIntensity = currentLight->GetLightColor() / static_cast<float>(totalPhotonsForLight);
        for (int j = 0; j < totalPhotonsForLight; ++j) {
            Ray photonRay;
            std::vector<char> path;
            path.push_back('L');
            currentLight->GenerateRandomPhotonRay(photonRay);
            TracePhoton(photonMap, &photonRay, photonIntensity, path, 1.f, maxPhotonBounces);
        }
    }
}

void PhotonMappingRenderer::TracePhoton(PhotonKdtree& photonMap, Ray* photonRay, glm::vec3 lightIntensity, std::vector<char>& path, float currentIOR, int remainingBounces)
{
    /*
     * Assignment 8 TODO: Trace a photon into the scene and make it bounce.
     *    
     *    How to insert a 'Photon' struct into the photon map.
     *        Photon myPhoton;
     *        ... set photon properties ...
     *        photonMap.insert(myPhoton);
     */
    if (remainingBounces < 0) return;

    assert(photonRay);
    IntersectionState state(0, 0);
    state.currentIOR = currentIOR;
    if (!storedScene->Trace(photonRay, &state)) return;

    if (path.size() != 1) {
        Photon photon;
        photon.position = state.intersectionRay.GetRayPosition(state.intersectionT);
        photon.intensity = lightIntensity;
        photon.toLightRay = Ray(photon.position, -state.intersectionRay.GetRayDirection());
        photonMap.insert(photon);
    }

    const MeshObject *hitMeshObject = state.intersectedPrimitive->GetParentMeshObject();
    const Material *hitMaterial = hitMeshObject->GetMaterial();
    const glm::vec3 diffuse = hitMaterial->GetBaseDiffuseReflection();
    const float Pr = glm::max(glm::max(diffuse.r, diffuse.g), diffuse.b);

    if ((double) rand() / RAND_MAX >= Pr) return;

    const float u1 = (double) rand() / RAND_MAX;
    const float u2 = (double) rand() / RAND_MAX;
    const float r = glm::sqrt(u1);
    const float Th = 2 * PI * u2;
    const float x = r * glm::cos(Th);
    const float y = r * glm::sin(Th);
    const float z = glm::sqrt(1 - u1);

    const glm::vec3 n = state.ComputeNormal();
    const glm::vec3 t = glm::abs(glm::dot(n, glm::vec3(1, 0, 0))) < 0.8 ?
        glm::cross(n, glm::vec3(1, 0, 0)) : glm::cross(n, glm::vec3(0, 1, 0));
    const glm::vec3 b = glm::cross(n, t);
    const glm::mat3 transform = glm::mat3(t, b, n);

    path.push_back('b');
    photonRay->SetRayPosition(state.intersectionRay.GetRayPosition(state.intersectionT));
    photonRay->SetRayDirection(glm::normalize(transform * glm::vec3(x, y, z)));
    TracePhoton(photonMap, photonRay, lightIntensity, path, currentIOR, remainingBounces - 1);
    path.pop_back();
}

glm::vec3 PhotonMappingRenderer::ComputeSampleColor(const struct IntersectionState& intersection, const class Ray& fromCameraRay) const
{
    glm::vec3 finalRenderColor = BackwardRenderer::ComputeSampleColor(intersection, fromCameraRay);
#if VISUALIZE_PHOTON_MAPPING
    Photon intersectionVirtualPhoton;
    intersectionVirtualPhoton.position = intersection.intersectionRay.GetRayPosition(intersection.intersectionT);

    std::vector<Photon> foundPhotons;
    diffuseMap.find_within_range(intersectionVirtualPhoton, 0.003f, std::back_inserter(foundPhotons));
    if (!foundPhotons.empty()) {
        finalRenderColor += glm::vec3(1.f, 0.f, 0.f);
    }
#endif
    return finalRenderColor;
}

void PhotonMappingRenderer::SetNumberOfDiffusePhotons(int diffuse)
{
    diffusePhotonNumber = diffuse;
}
