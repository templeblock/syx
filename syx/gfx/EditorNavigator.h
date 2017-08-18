#pragma once
#include "system/System.h"

class EditorNavigator : public System {
public:
  SystemId getId() const override {
    return SystemId::EditorNavigator;
  }

  void update(float dt) override;
};