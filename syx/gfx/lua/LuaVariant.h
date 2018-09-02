#pragma once
#include "lua/LuaKey.h"
#include "lua/LuaNode.h"

struct lua_State;

namespace Lua {
  class Node;

  class Variant {
  public:
    Variant();
    Variant(const Key& key);
    Variant(const Variant& rhs);
    Variant(Variant&& rhs);
    ~Variant();

    Variant& operator=(const Variant& rhs);
    Variant& operator=(Variant&& rhs);
    bool operator==(const Variant& rhs) const;
    bool operator!=(const Variant& rhs) const;

    // Clear this and all children and populate from value on top of the stack
    bool readFromLua(lua_State* l);
    // Write this and all children to the top of the stack
    void writeToLua(lua_State* l) const;
    void clear();
    size_t getTypeId() const;
    const Key& getKey() const;
    const Variant* getChild(const Key& key) const;
    Variant* getChild(const Key& key);
    void forEachChild(std::function<void(const Variant&)> callback) const;
    void forEachChild(std::function<void(Variant&)> callback);

    template<typename T>
    T* get() {
      return getTypeId() == typeId<T>() && mData.size() ? reinterpret_cast<T*>(mData.data()) : nullptr;
    }
    template<typename T>
    const T* get() const {
      return getTypeId() == typeId<T>() && mData.size() ? reinterpret_cast<const T*>(mData.data()) : nullptr;
    }

  private:
    void _destructData();
    void _copyData(const std::vector<uint8_t>& from);
    void _moveData(std::vector<uint8_t>& from);

    Key mKey;
    const Node* mType;
    std::vector<uint8_t> mData;
    std::vector<Variant> mChildren;
  };

  class VariantNode : public TypedNode<Variant> {
    using TypedNode::TypedNode;
    NODE_SINGLETON(VariantNode);
    void _readFromLua(lua_State* s, void* base) const override {
      _cast(base).readFromLua(s);
    }
    void _writeToLua(lua_State* s, const void* base) const override {
      _cast(base).writeToLua(s);
    }
  };
}