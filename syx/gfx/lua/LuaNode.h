#pragma once
//Nodes used to specify a structure to bind data to, allowing it to be
//read and written to lua. All nodes take a reference to the type to bind,
//which will be written or read. Overload makeNode for new node types.

struct lua_State;

namespace Syx {
  struct Vec3;
  struct Quat;
}

namespace Lua {
  class Node;

  struct NodeOps {
    NodeOps(Node& parent, std::string&& name, size_t offset);
    NodeOps(std::string&& name);
    NodeOps(const std::string& name);

    Node* mParent;
    std::string mName;
    size_t mOffset;
  };

  //read/write take base pointer so one scheme can be used between all instances of the class
  //members are then accessed through pointer offsets
  class Node {
  public:
    enum class SourceType {
      FromGlobal,
      FromStack,
      FromParent,
      Default = FromParent
    };
    Node(NodeOps&& ops);
    virtual ~Node();
    Node(const Node&) = delete;
    Node(Node&&) = delete;
    Node& operator=(const Node&) = delete;

    //Read state from lua object(s) on stack or global
    void readFromLua(lua_State* s, void* base, SourceType source = SourceType::Default) const;
    //Write state to new lua object(s) on stack or global
    void writeToLua(lua_State* s, const void* base, SourceType source = SourceType::Default) const;
    //Read state from lua object(s) on stack or global into flat buffer. Values are default constructed into buffer then assigned. Caller must ensure buffer has size() bytes
    void readFromLuaToBuffer(lua_State* s, void* buffer, SourceType source = SourceType::Default) const;

    const Node* getChild(const char* child) const;
    void addChild(std::unique_ptr<Node> child);

    //Total size in bytes of tree
    size_t size() const;
    //Copy construct each node in tree into a flatttened buffer. Caller must ensure buffer has at least size() bytes left
    void copyConstructToBuffer(const void* base, void* buffer) const;
    void copyConstructFromBuffer(void* base, const void* buffer) const;
    //Copy each node in tree. Caller must ensure buffer has at least size() bytes left
    void copyToBuffer(const void* base, void* buffer) const;
    void copyFromBuffer(void* base, const void* buffer) const;
    //Copy construct each value in buffer to new location
    void copyConstructBufferToBuffer(const void* from, void* to) const;
    void copyBufferToBuffer(const void* from, void* to) const;
    //Call destructor on each value in buffer
    void destructBuffer(void* buffer) const;

    //Size of this node in bytes
    virtual size_t _size() const { return 0; }
    //Default construct node at location
    virtual void _defaultConstruct(void* to) const {}
    //Copy construct node
    virtual void _copyConstruct(const void* from, void* to) const {}
    //Copy node
    virtual void _copy(const void* from, void* to) const {}
    //Call destructor on node
    virtual void _destruct(void* base) const {}

    const std::string& getName() const;

  protected:
    virtual void _readFromLua(lua_State* s, void* base) const = 0;
    virtual void _writeToLua(lua_State* s, const void* base) const = 0;

    //Traverse the tree and use func to construct/move/copy to/from buffer
    void _funcToBuffer(void (Node::* func)(const void*, void*) const, const void* base, void* buffer) const;
    void _funcFromBuffer(void (Node::* func)(const void*, void*) const, void* base, const void* buffer) const;
    //Traverse buffer and use func on each leaf node
    void _funcBufferToBuffer(void (Node::* func)(const void*, void*) const, const void* from, void* to) const;

    //Translate the location of base based on this node type. Usually nothing but can be used to follow pointers
    virtual void _translateBase(const void*& base) const {}
    void _translateBase(void*& base) const { _translateBase(const_cast<const void*&>(base)); }

    //Push stack[top][field] onto top of stack, or global[field] if root node
    void getField(lua_State* s, const std::string& field, SourceType source = SourceType::Default) const;
    //stack[top - 1][field] = stack[top]
    void setField(lua_State* s, const std::string& field, SourceType source = SourceType::Default) const;
    //Same as above but uses mName
    void getField(lua_State* s, SourceType source = SourceType::Default) const;
    void setField(lua_State* s, SourceType source = SourceType::Default) const;

    NodeOps mOps;
    std::vector<std::unique_ptr<Node>> mChildren;
    //Size of tree from here down
    size_t mSize;
  };

  class RootNode : public Node {
  public:
    using Node::Node;
    void _readFromLua(lua_State* s, void* base) const override {}
    void _writeToLua(lua_State* s, const void* base) const override {}
  };

  inline std::unique_ptr<Node> makeRootNode(NodeOps&& ops) {
    return std::make_unique<RootNode>(std::move(ops));
  }

  template<typename T>
  T& makeNode(NodeOps&& ops) {
    auto newNode = std::make_unique<T>(std::move(ops));
    auto& result = *newNode;
    ops.mParent->addChild(std::move(newNode));
    return result;
  }

  template<typename T>
  class TypedNode : public Node {
  public:
    using Node::Node;
  protected:
    size_t _size() const override {
      return sizeof(T);
    }
    void _defaultConstruct(void* to) const override {
      new (to) T();
    }
    void _copyConstruct(const void* from, void* to) const override {
      new (to) T(_cast(from));
    }
    void _copy(const void* from, void* to) const override {
      _cast(to) = _cast(from);
    }
    void _destruct(void* base) const override {
      _cast(base).~T();
    }
    T& _cast(void* value) const {
      return *static_cast<T*>(value);
    }
    const T& _cast(const void* value) const {
      return *static_cast<const T*>(value);
    }
  };

  class IntNode : public TypedNode<int> {
  public:
    using WrappedType = int;
    using TypedNode::TypedNode;
    void _readFromLua(lua_State* s, void* base) const override;
    void _writeToLua(lua_State* s, const void* base) const override;
  };

  class StringNode : public TypedNode<std::string> {
  public:
    using WrappedType = std::string;
    using TypedNode::TypedNode;
    void _readFromLua(lua_State* s, void* base) const override;
    void _writeToLua(lua_State* s, const void* base) const override;
  };

  class FloatNode : public TypedNode<float> {
  public:
    using WrappedType = float;
    using TypedNode::TypedNode;
    void _readFromLua(lua_State* s, void* base) const override;
    void _writeToLua(lua_State* s, const void* base) const override;
  };

  class LightUserdataNode : public TypedNode<void*> {
  public:
    using WrappedType = void*;
    using TypedNode::TypedNode;
    void _readFromLua(lua_State* s, void* base) const override;
    void _writeToLua(lua_State* s, const void* base) const override;
  };

  class LightUserdataSizetNode : public TypedNode<size_t> {
  public:
    using WrappedType = size_t;
    using TypedNode::TypedNode;
    void _readFromLua(lua_State* s, void* base) const override;
    void _writeToLua(lua_State* s, const void* base) const override;
  };

  class BoolNode : public TypedNode<bool> {
  public:
    using WrappedType = bool;
    using TypedNode::TypedNode;
    void _readFromLua(lua_State* s, void* base) const override;
    void _writeToLua(lua_State* s, const void* base) const override;
  };

  class Vec3Node : public TypedNode<Syx::Vec3> {
  public:
    using WrappedType = Syx::Vec3;
    using TypedNode::TypedNode;
    void _readFromLua(lua_State* s, void* base) const override;
    void _writeToLua(lua_State* s, const void* base) const override;
  };

  class QuatNode : public TypedNode<Syx::Quat> {
  public:
    using WrappedType = Syx::Quat;
    using TypedNode::TypedNode;
    void _readFromLua(lua_State* s, void* base) const override;
    void _writeToLua(lua_State* s, const void* base) const override;
  };

  class Mat4Node : public TypedNode<Syx::Mat4> {
  public:
    using WrappedType = Syx::Mat4;
    using TypedNode::TypedNode;
    void _readFromLua(lua_State* s, void* base) const override;
    void _writeToLua(lua_State* s, const void* base) const override;
  };
}
