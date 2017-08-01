#pragma once
#include "System.h"

//Same as windows keys, but this way you don't have to include windows.h to query key states
enum class Key : uint8_t {
  LeftMouse = 1,
  RightMouse = 2,
  MiddleMouse = 4,
  Back = 8,
  Tab = 9,
  Enter = 13,
  Shift = 16,
  Control = 17,
  Alt = 18,
  CapsLock = 20,
  Esc = 27,
  Space = 32,
  PageUp = 33,
  PageDown = 34,
  End = 35,
  Home = 36,
  Left = 37,
  Up = 38,
  Right = 39,
  Down = 40,
  //Not numpad
  Key0 = 48, Key1, Key2, Key3, Key4, Key5, Key6, Key7, Key8, Key9,
  KeyA = 65, KeyB, KeyC, KeyD, KeyE, KeyF, KeyG, KeyH, KeyI, KeyJ, KeyK, KeyL, KeyM, KeyN, KeyO, KeyP, KeyQ, KeyR, KeyS, KeyT, KeyU, KeyV, KeyW, KeyX, KeyY, KeyZ,
  //Numpad
  Num0 = 96, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
  F1 = 112, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24,
  LeftShift = 160,
  RightShift,
  LeftCtrl,
  RightCtrl,
};

enum class KeyState : uint8_t {
  Up,
  Down,
  OnUp,
  OnDown
};

class KeyboardInput : public System {
public:
  SystemId getId() const override {
    return SystemId::KeyboardInput;
  }

  void init() override;
  void update(float dt) override;
  void uninit() override;

  KeyState getKeyState(Key key) const;
  bool getKeyDown(Key key) const;
  bool getKeyUp(Key key) const;
  //Get mouse information in pixels
  Syx::Vec2 getMousePos() const;
  Syx::Vec2 getMouseDelta() const;
private:
  static const size_t sKeyCount = 256;

  unsigned char mPrevState[sKeyCount];
  unsigned char mCurState[sKeyCount];
  Syx::Vec2 mPrevMouse;
  Syx::Vec2 mCurMouse;
};