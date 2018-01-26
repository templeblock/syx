#include "Precompile.h"
#include "App.h"
#include "system/GraphicsSystem.h"
#include "threading/WorkerPool.h"
#include "threading/Task.h"
#include "EditorNavigator.h"
#include "Space.h"
#include "ImGuiImpl.h"
#include "system/MessagingSystem.h"
#include "event/Event.h"

#include "AppPlatform.h"

App::App(std::unique_ptr<AppPlatform> appPlatform)
  : mDefaultSpace(std::make_unique<Space>(*this))
  , mWorkerPool(std::make_unique<WorkerPool>(4))
  , mAppPlatform(std::move(appPlatform)) {
  System::Registry::getSystems(*this, mSystems);
}

App::~App() {
}

#include <lua.hpp>

int luaToCFunc(lua_State* lua) {
  printf("Called C function\n");
  double input = lua_tonumber(lua, 1);
  lua_pushnumber(lua, input + 10);
  return 1;
}

void luaTest() {
  lua_State* lua = luaL_newstate();
  luaL_openlibs(lua);

  lua_pushcfunction(lua, luaToCFunc);
  lua_setglobal(lua, "testC");

  int status = luaL_loadfile(lua, "scripts/test.lua");
  if(status) {
    printf("Couldn't load file: %s\n", lua_tostring(lua, -1));
    return;
  }

  lua_newtable(lua);
  for(int i = 1; i <= 5; ++i) {
    lua_pushnumber(lua, i);
    lua_pushnumber(lua, i*2);
    lua_rawset(lua, -3);
  }
  lua_setglobal(lua, "foo");

  int result = lua_pcall(lua, 0, LUA_MULTRET, 0);
  if(result) {
    printf("Failed to run script: %s\n", lua_tostring(lua, -1));
    return;
  }
  double returnValue = lua_tonumber(lua, -1);
  printf("Script returned: %.0f\n", returnValue);
  lua_pop(lua, -1);

  lua_getglobal(lua, "func");
  lua_pushnumber(lua, 7);
  lua_call(lua, 1, 0);

  lua_close(lua);
}

class AppFocusListener : public FocusEvents {
  void onFocusGained() override {
    printf("focus gained\n");
  }
  void onFocusLost() override {
    printf("focus lost\n");
  }
};

class AppDirectoryWatcher : public DirectoryWatcher {
  void onFileChanged(const std::string& filename) override {
    printf("File changed: %s\n", filename.c_str());
  }
  void onFileAdded(const std::string& filename) {
    printf("File added: %s\n", filename.c_str());
  }
  void onFileRemoved(const std::string& filename) {
    printf("File removed: %s\n", filename.c_str());
  }
  void onFileRenamed(const std::string& oldName, const std::string& newName) {
    printf("File renamed from: %s to %s\n", oldName.c_str(), newName.c_str());
  }
};

void App::init() {
  luaTest();

  for(auto& system : mSystems) {
    if(system)
      system->init();
  }

  mDefaultSpace->init();

  static FocusEvents::ObserverType o(std::make_unique<AppFocusListener>());
  if(!o.hasSubject())
    getAppPlatform().addFocusObserver(o);
  static DirectoryWatcher::ObserverType d(std::make_unique<AppDirectoryWatcher>());
  if(!d.hasSubject())
    getAppPlatform().addDirectoryObserver(d);
}

#include "imgui/imgui.h"

void App::update(float dt) {

  auto frameTask = std::make_shared<Task>();

  mDefaultSpace->update(dt);
  EventListener& events = getSystem<MessagingSystem>()->getListener();
  for(auto& system : mSystems) {
    if(system) {
      if(EventListener* systemListener = system->getListener())
        events.appendTo(*systemListener);
      system->update(dt, *mWorkerPool, frameTask);
    }
  }
  events.clear();
  mWorkerPool->queueTask(frameTask);

  if(ImGuiImpl::enabled()) {
    static float f = 0.0f;
    ImGui::Text("Hello, world!");
    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    static Syx::Vec3 clear_color(0.0f);
    static bool show_test_window = true;
    static bool show_another_window = true;
    ImGui::ColorEdit3("clear color", (float*)&clear_color);
    if(ImGui::Button("Test Window")) show_test_window ^= 1;
    if(ImGui::Button("Another Window")) show_another_window ^= 1;
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    const int buffSize = 100;
    static char buff[buffSize] = { 0 };
    ImGui::InputText("Text In", buff, buffSize);
  }

  std::weak_ptr<Task> weakFrame = frameTask;
  frameTask = nullptr;
  mWorkerPool->sync(weakFrame);
}

void App::uninit() {
  mDefaultSpace->uninit();
  for(auto& system : mSystems) {
    if(system)
      system->uninit();
  }
}

Space& App::getDefaultSpace() {
  return *mDefaultSpace;
}

IWorkerPool& App::getWorkerPool() {
  return *mWorkerPool;
}

AppPlatform& App::getAppPlatform() {
  return *mAppPlatform;
}
