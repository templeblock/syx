#include "Precompile.h"
#include "system/GraphicsSystem.h"
#include "Model.h"
#include "Shader.h"
#include "Camera.h"
#include "DebugDrawer.h"
#include "ModelLoader.h"
#include "TextureLoader.h"
#include "Texture.h"
#include "App.h"
#include "Space.h"
#include "Gameobject.h"
#include "ImGuiImpl.h"
#include "component/Renderable.h"
#include "system/MessagingSystem.h"
#include "system/KeyboardInput.h"
#include "event/TransformEvent.h"
#include "event/BaseComponentEvents.h"

using namespace Syx;

RegisterSystemCPP(GraphicsSystem);

static void readFile(const std::string& path, std::string& buffer) {
  std::ifstream file(path, std::ifstream::in | std::ifstream::binary);
  if(file.good()) {
    file.seekg(0, file.end);
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0, file.beg);
    buffer.resize(size + 1);
    file.read(&buffer[0], size);
    buffer[size] = 0;
  }
}

GraphicsSystem::LocalRenderable::LocalRenderable(Handle h)
  : mHandle(h)
  , mModel(nullptr)
  , mDiffTex(nullptr)
  , mTransform(Syx::Mat4::identity()) {
}

Handle GraphicsSystem::LocalRenderable::getHandle() const {
  return mHandle;
}

std::unique_ptr<Shader> GraphicsSystem::_loadShadersFromFile(const std::string& vsPath, const std::string& psPath) {
  mVSBuffer.clear();
  mPSBuffer.clear();
  std::unique_ptr<Shader> result = std::make_unique<Shader>();
  readFile(vsPath, mVSBuffer);
  readFile(psPath, mPSBuffer);
  if(mVSBuffer.size() && mPSBuffer.size()) {
    result->load(mVSBuffer, mPSBuffer);
  }
  else {
    if(mVSBuffer.empty())
      printf("Vertex shader not found at %s\n", vsPath.c_str());
    if(mPSBuffer.empty())
      printf("Pixel shader not found at %s\n", psPath.c_str());
  }
  return result;
}

GraphicsSystem::~GraphicsSystem() {
}

GraphicsSystem::GraphicsSystem(App& app)
  : System(app) {
}

void GraphicsSystem::init() {
  mCamera = std::make_unique<Camera>(CameraOps(1.396f, 1.396f, 0.1f, 100.0f));
  mGeometry = _loadShadersFromFile("shaders/phong.vs", "shaders/phong.ps");

  Mat4 ct = mCamera->getTransform();
  ct.setTranslate(Vec3(0.0f, 0.0f, -3.0f));
  ct.setRot(Quat::lookAt(-Vec3::UnitZ));
  mCamera->setTransform(ct);
  mDebugDrawer = std::make_unique<DebugDrawer>(*this);
  mImGui = std::make_unique<ImGuiImpl>();

  mModelLoader = std::make_unique<ModelLoader>();
  mTextureLoader = std::make_unique<TextureLoader>();

  mTransformListener = std::make_unique<TransformListener>();
  mEventListener = std::make_unique<EventListener>(EventFlag::Component | EventFlag::Graphics);
  MessagingSystem* msg = mApp.getSystem<MessagingSystem>();
  msg->addTransformListener(*mTransformListener);
  msg->addEventListener(*mEventListener);
}

void GraphicsSystem::update(float dt, IWorkerPool& pool, std::shared_ptr<Task> frameTask) {
  _processEvents();
  //Can't really do anything on background threads at the moment because this one has the context.
  _render(dt);
  if(mImGui) {
    mImGui->render(dt, mScreenSize);
    mImGui->updateInput(*mApp.getSystem<KeyboardInput>());
  }
}

void GraphicsSystem::uninit() {
  mApp.getSystem<MessagingSystem>()->removeTransformListener(*mTransformListener);
}

Camera& GraphicsSystem::getPrimaryCamera() {
  return *mCamera;
}

DebugDrawer& GraphicsSystem::getDebugDrawer() {
  return *mDebugDrawer;
}

Handle GraphicsSystem::addModel(Model& model) {
  model.mHandle = mModelGen.next();
  Model& added = mHandleToModel[model.mHandle] = model;
  //Ultimately this should be a separate step as needed
  added.loadGpu();
  return added.mHandle;
}

Handle GraphicsSystem::addModel(const std::string& filePath) {
  std::unique_ptr<Model> model = mModelLoader->loadModel(filePath);
  if(model) {
    return addModel(*model);
  }
  return InvalidHandle;
}

template<typename Resource>
void removeResource(Handle handle, std::unordered_map<Handle, Resource>& resMap, const char* errorMsg) {
  auto it = resMap.find(handle);
  if(it == resMap.end()) {
    printf(errorMsg);
    return;
  }
  it->second.unloadGpu();
  resMap.erase(it);
}

void GraphicsSystem::removeModel(Handle model) {
  removeResource(model, mHandleToModel, "Tried to remove model that didn't exist");
}

void GraphicsSystem::removeTexture(Handle texture) {
  removeResource(texture, mHandleToTexture, "Tried to remove texture that didn't exist");
}

Handle GraphicsSystem::addTexture(const std::string& filePath) {
  Handle handle = mTextureGen.next();
  Texture& t = mHandleToTexture[handle];
  t.mFilename = filePath;
  t.mHandle = handle;
  t.loadGpu(*mTextureLoader);
  return t.mHandle;
}

void GraphicsSystem::_processEvents() {
  mTransformListener->updateLocal();
  mEventListener->updateLocal();

  for(const std::unique_ptr<Event>& e : mEventListener->mLocalEvents) {
    switch(static_cast<EventType>(e->getHandle())) {
      case EventType::AddComponent: _processAddEvent(static_cast<const ComponentEvent&>(*e)); break;
      case EventType::RemoveComponent: _processRemoveEvent(static_cast<const ComponentEvent&>(*e)); break;
      case EventType::RenderableUpdate: _processRenderableEvent(static_cast<const RenderableUpdateEvent&>(*e)); break;
    }
  }
  mEventListener->mLocalEvents.clear();

  for(const TransformEvent& e : mTransformListener->mLocalEvents)
    _processTransformEvent(e);
  mTransformListener->mLocalEvents.clear();
}

void GraphicsSystem::_processAddEvent(const ComponentEvent& e) {
  if(static_cast<ComponentType>(e.mCompType) == ComponentType::Graphics)
    mLocalRenderables.pushBack(LocalRenderable(e.mObj));
}

void GraphicsSystem::_processRemoveEvent(const ComponentEvent& e) {
  if(static_cast<ComponentType>(e.mCompType) == ComponentType::Graphics)
    mLocalRenderables.erase(e.mObj);
}

void GraphicsSystem::_processTransformEvent(const TransformEvent& e) {
  LocalRenderable* obj = mLocalRenderables.get(e.mHandle);
  if(obj) {
    obj->mTransform = e.mTransform;
  }
}

void GraphicsSystem::_processRenderableEvent(const RenderableUpdateEvent& e) {
  LocalRenderable* obj = mLocalRenderables.get(e.mObj);
  if(obj) {
    auto mod = mHandleToModel.find(e.mData.mModel);
    if(mod != mHandleToModel.end())
      obj->mModel = &mod->second;

    auto tex = mHandleToTexture.find(e.mData.mDiffTex);
    if(tex != mHandleToTexture.end())
      obj->mDiffTex = &tex->second;
  }
}

void GraphicsSystem::_render(float dt) {
  glClearColor(0.0f, 0.0f, 1.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  mDebugDrawer->_render(mCamera->getWorldToView());

  {
    Texture emptyTexture;
    Shader::Binder sb(*mGeometry);
    Vec3 camPos = mCamera->getTransform().getTranslate();
    Vec3 mDiff(1.0f);
    Vec3 mSpec(0.6f, 0.6f, 0.6f, 2.5f);
    Vec3 mAmb(0.22f, 0.22f, 0.22f);
    Vec3 sunDir = -Vec3::Identity.normalized();
    Vec3 sunColor = Vec3::Identity;
    Mat4 wvp = mCamera->getWorldToView();

    {
      Vec3 p(3.0f);
      mDebugDrawer->drawLine(p, p + sunDir, sunColor);
      mDebugDrawer->drawLine(p + sunDir, p + sunDir - Vec3(0.1f));
    }

    glUniform3f(mGeometry->getUniform("uCamPos"), camPos.x, camPos.y, camPos.z);
    glUniform3f(mGeometry->getUniform("uDiffuse"), mDiff.x, mDiff.y, mDiff.z);
    glUniform3f(mGeometry->getUniform("uAmbient"), mAmb.x, mAmb.y, mAmb.z);
    glUniform4f(mGeometry->getUniform("uSpecular"), mSpec.x, mSpec.y, mSpec.z, mSpec.w);
    glUniform3f(mGeometry->getUniform("uSunDir"), sunDir.x, sunDir.y, sunDir.z);
    glUniform3f(mGeometry->getUniform("uSunColor"), sunColor.x, sunColor.y, sunColor.z);

    for(LocalRenderable& obj : mLocalRenderables.getBuffer()) {
      if(!obj.mModel)
        continue;

      Mat4 mw = obj.mTransform;
      Mat4 mvp = wvp * mw;
      Vec3 camPos = mCamera->getTransform().getTranslate();

      {
        Texture::Binder tb(obj.mDiffTex ? *obj.mDiffTex : emptyTexture, 0);
        //Tell the sampler uniform to use the given texture slot
        glUniform1i(mGeometry->getUniform("uTex"), 0);
        {
          Model::Binder mb(*obj.mModel);

          glUniformMatrix4fv(mGeometry->getUniform("uMVP"), 1, GL_FALSE, mvp.mData);
          glUniformMatrix4fv(mGeometry->getUniform("uMW"), 1, GL_FALSE, mw.mData);
          obj.mModel->draw();
        }
      }
    }
  }
}

void GraphicsSystem::onResize(int width, int height) {
  glViewport(0, 0, width, height);
  mScreenSize = Syx::Vec2(static_cast<float>(width), static_cast<float>(height));
}
