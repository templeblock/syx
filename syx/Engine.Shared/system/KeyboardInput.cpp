#include "Precompile.h"
#include "KeyboardInput.h"

#include "AppPlatform.h"

namespace {
  const std::unordered_map<Key, std::string> KEY_TO_STRING = []() {
    std::unordered_map<Key, std::string> map;
    map.reserve(static_cast<size_t>(Key::Count));
      map[Key::LeftMouse] = "lmb";
      map[Key::RightMouse] = "rmb";
      map[Key::MiddleMouse] = "mmb";
      map[Key::Backspace] = "backspace";
      map[Key::Tab] = "tab";
      map[Key::Enter] = "enter";
      map[Key::Shift] = "shift";
      map[Key::Control] = "ctrl";
      map[Key::Alt] = "alt";
      map[Key::CapsLock] = "caps";
      map[Key::Esc] = "esc";
      map[Key::Space] = "space";
      map[Key::PageUp] = "pgup";
      map[Key::PageDown] = "pgdn";
      map[Key::End] = "end";
      map[Key::Home] = "home";
      map[Key::Left] = "left";
      map[Key::Up] = "up";
      map[Key::Right] = "right";
      map[Key::Down] = "down";
      map[Key::Delete] = "del",
      map[Key::Mul] = "*";
      map[Key::Add] = "+";
      map[Key::Sub] = "-";
      map[Key::Dot] = ".";
      map[Key::FwdSlash] = "/";
      map[Key::Semicolon] = ";";
      map[Key::Comma] = ",";
      map[Key::Question] = "?";
      map[Key::Tilda] = "~";
      map[Key::LeftCurly] = "{";
      map[Key::Bar] = "|";
      map[Key::RightCurly] = "}";
      map[Key::Quote] = "\"";
      map[Key::MinusUnderLine] = "-";
      map[Key::PlusEq] = "=";
      //map[Key::F1 = 112, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24,
      map[Key::LeftShift] = "lshift";
      map[Key::RightShift] = "rshift";
      map[Key::LeftCtrl] = "lctrl";
      map[Key::RightCtrl] = "rctrl";
      for(int i = 0; i < 10; ++i) {
        std::string num = std::to_string(i);
        map[static_cast<Key>(static_cast<int>(Key::Key0) + i)] = num;
        map[static_cast<Key>(static_cast<int>(Key::Num0) + i)] = "num" + num;
      }
      for(char i = 0; i < 26; ++i) {
        map[static_cast<Key>(static_cast<int>(Key::KeyA) + i)] = std::string(static_cast<size_t>(1), 'a' + i);
      }
      for(char i = 0; i < 24; ++i) {
        map[static_cast<Key>(static_cast<int>(Key::F24) + i)] = "f" + std::string(static_cast<size_t>(1), '1' + i);
      }
      return map;
  }();
  const std::unordered_map<std::string, Key> STRING_TO_KEY = []() {
    std::unordered_map<std::string, Key> result;
    result.reserve(KEY_TO_STRING.size());
    for(const auto pair : KEY_TO_STRING)
      result[pair.second] = pair.first;
    return result;
  }();

  static Key _charToKey(char c, char baseChar, Key baseKey) {
    uint8_t offset = static_cast<uint8_t>(c - baseChar);
    return static_cast<Key>(static_cast<uint8_t>(baseKey) + offset);
  }
}

KeyboardInput::~KeyboardInput() = default;

KeyState KeyboardInput::getKeyState(const std::string& key) const {
  const auto keyIt = STRING_TO_KEY.find(key);
  return keyIt != STRING_TO_KEY.end() ? getKeyState(keyIt->second) : KeyState::Invalid;
}

KeyState KeyboardInput::getKeyState(Key key) const {
  return mPlatform->getKeyState(key);
}

bool KeyboardInput::getKeyDown(Key key) const {
  return getKeyState(key) == KeyState::Down;
}

bool KeyboardInput::getKeyUp(Key key) const {
  return getKeyState(key) == KeyState::Up;
}

bool KeyboardInput::getKeyTriggered(Key key) const {
  return getKeyState(key) == KeyState::Triggered;
}

bool KeyboardInput::getKeyReleased(Key key) const {
  return getKeyState(key) == KeyState::Released;
}

KeyState KeyboardInput::getAsciiState(char c) const {
  switch(c) {
    case '!': return _shiftAnd(Key::Key1);
    case '@': return _shiftAnd(Key::Key2);
    case '#': return _shiftAnd(Key::Key3);
    case '$': return _shiftAnd(Key::Key4);
    case '%': return _shiftAnd(Key::Key5);
    case '^': return _shiftAnd(Key::Key6);
    case '&': return _shiftAnd(Key::Key7);
    case '*': return _shiftAnd(Key::Key8);
    case '(': return _shiftAnd(Key::Key9);
    case ')': return _shiftAnd(Key::Key0);
    case '-': return _noShift(Key::MinusUnderLine);
    case '_': return _shiftAnd(Key::MinusUnderLine);
    case '+': return _shiftAnd(Key::PlusEq);
    case '=': return _noShift(Key::PlusEq);
    case '[': return _noShift(Key::LeftCurly);
    case '{': return _shiftAnd(Key::LeftCurly);
    case ']': return _noShift(Key::RightCurly);
    case '}': return _shiftAnd(Key::RightCurly);
    case '\\': return _noShift(Key::Bar);
    case '|': return _shiftAnd(Key::Bar);
    case ';': return _noShift(Key::Semicolon);
    case ':': return _shiftAnd(Key::Semicolon);
    case '\'': return _noShift(Key::Quote);
    case '"': return _shiftAnd(Key::Quote);
    case ',': return _noShift(Key::Comma);
    case '<': return _shiftAnd(Key::Comma);
    case '.': return _noShift(Key::Dot);
    case '>': return _shiftAnd(Key::Dot);
    case '/': return _noShift(Key::Question);
    case '?': return _shiftAnd(Key::Question);
    case '`': return _noShift(Key::Tilda);
    case '~': return _shiftAnd(Key::Tilda);
    case ' ': return _or(_noShift(Key::Space), _shiftAnd(Key::Space));
  }

  // Key codes numbers and characters match
  if(c >= 'a' && c <= 'z')
    return _noShift(_charToKey(c, 'a', Key::KeyA));
  if(c >= 'A' && c <= 'Z')
    return _shiftAnd(_charToKey(c, 'A', Key::KeyA));
  if(c >= '0' && c <= '9')
    return _noShift(_charToKey(c, '0', Key::Key0));
  return KeyState::Up;
}

KeyState KeyboardInput::_shiftAnd(Key key) const {
  if(getKeyDown(Key::Shift))
    return getKeyState(key);
  return KeyState::Up;
}

KeyState KeyboardInput::_noShift(Key key) const {
  if(getKeyDown(Key::Shift))
    return KeyState::Up;
  return getKeyState(key);
}

KeyState KeyboardInput::_or(KeyState a, KeyState b) const {
  if(a == KeyState::Triggered || b == KeyState::Triggered)
    return KeyState::Triggered;
  if(a == KeyState::Down || b == KeyState::Down)
    return KeyState::Down;
  return KeyState::Up;
}

Syx::Vec2 KeyboardInput::getMousePos() const {
  return mPlatform->getMousePos();
}

Syx::Vec2 KeyboardInput::getMouseDelta() const {
  return mPlatform->getMouseDelta();
}

float KeyboardInput::getWheelDelta() const {
  return mPlatform->getWheelDelta();
}

void KeyboardInput::init() {
  mPlatform = &mArgs.mAppPlatform->getKeyboardInput();
}

void KeyboardInput::update(float, IWorkerPool&, std::shared_ptr<Task>) {
  mPlatform->update();
}
