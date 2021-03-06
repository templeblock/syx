#include "Precompile.h"
#include "SyxCollider.h"
#include "SyxPhysicsSystem.h"
#include "SyxSpace.h"

namespace Syx {
  Collider::Collider(PhysicsObject* owner): mOwner(owner), mFlags(0), mBroadHandle(SyxInvalidHandle) {
  }

  void Collider::setFlag(int flag, bool value) {
    setBits(mFlags, flag, value);
  }

  bool Collider::getFlag(int flag) {
    return (mFlags & flag) != 0;
  }

  Vec3 Collider::getSupport(const Vec3& dir) {
    return mModelInst.getSupport(dir);
  }

  void Collider::updateModelInst(const Transform& parentTransform) {
    mModelInst.updateTransformers(parentTransform);
    mModelInst.updateAABB();
  }

  PhysicsObject* Collider::getOwner(void) {
    return mOwner;
  }

  void Collider::setModel(const Model& model) {
    mModelInst.setModel(model);
  }

  void Collider::setMaterial(const Material& material) {
    mModelInst.setMaterial(material);
  }

  int Collider::getModelType(void) {
    return mModelInst.getModelType();
  }

  void Collider::initialize(Space& space) {
    mBroadHandle = space.mBroadphase->insert(BoundingVolume(getAABB()), reinterpret_cast<void*>(mOwner));
  }

  void Collider::uninitialize(Space& space) {
    space.mBroadphase->remove(mBroadHandle);
    mBroadHandle = SyxInvalidHandle;
  }

#ifdef SENABLED
  SFloats Collider::sGetSupport(SFloats dir) {
    return mModelInst.sGetSupport(dir);
  }
#else

#endif
}