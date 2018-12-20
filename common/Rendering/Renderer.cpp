#include "common/Rendering/Renderer.h"
#include "common/Scene/Scene.h"

Renderer::Renderer(std::shared_ptr<Scene> scene) :
    storedScene(scene)
{
}

Renderer::~Renderer()
{
}