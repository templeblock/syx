#pragma once
#include "SyxConstraint.h"
#include "SyxManifold.h"
#include "SyxTransform.h"
#include "SyxPhysicsObject.h"

namespace Syx {
  //Everything needed during solving loop in one compact structure to maximize cache coherence
  SAlign struct ContactBlock {
    //These are the linear and angular terms of the Jacobian that are pre-multiplied with the appropriate masses
    SAlign Vec3 mNormal;
    //Center to contact crossed with normal for object a and b
    SAlign Vec3 mRCrossNA[4];
    SAlign Vec3 mRCrossNB[4];

    //Same terms as above multiplied by the appropriate masses
    SAlign Vec3 mNormalTMass[2];
    SAlign Vec3 mRCrossNATInertia[4];
    SAlign Vec3 mRCrossNBTInertia[4];

    //Penetration bias term ready to be applied straight to the lambda
    SAlign Vec3 mPenetrationBias;
    //Inverse mass of the Jacobians of the contact constraints
    SAlign Vec3 mContactMass;
    //Sum of lambda terms over all iterations this frame
    SAlign Vec3 mLambdaSum;

    //If false this contact is ignored during solving
    bool mEnforce[4];
  };

  SAlign struct FrictionAxisBlock {
    SAlign Vec3 mConstraintMass;
    SAlign Vec3 mLambdaSum;
    SAlign Vec3 mAxis;
    SAlign Vec3 mRCrossAxisA[4];
    SAlign Vec3 mRCrossAxisB[4];

    //Premultiplied linear and angular terms of Jacobian for a and b
    SAlign Vec3 mLinearA;
    SAlign Vec3 mLinearB;
    SAlign Vec3 mAngularA[4];
    SAlign Vec3 mAngularB[4];
  };

  SAlign struct FrictionBlock {
    //Contact constraint's sum is used to determine strength of friction
    SAlign Vec3 mContactLambdaSum;
    SAlign FrictionAxisBlock mAxes[2];
    //If false this contact is ignored during solving
    bool mEnforce[4];
  };

  SAlign class ContactConstraint: public Constraint {
  public:
    friend class LocalContactConstraint;

    DeclareIntrusiveNode(ContactConstraint);

    ContactConstraint(PhysicsObject* a = nullptr, PhysicsObject* b = nullptr, Handle handle = SyxInvalidHandle, Handle instA = SyxInvalidHandle, Handle instB = SyxInvalidHandle)
      : Constraint(ConstraintType::Contact, a, b, handle)
      , mManifold(a ? a->getCollider() : nullptr, b ? b->getCollider() : nullptr)
      , mInactiveTime(0.0f)
      , mInstA(instA) 
      , mInstB(instB) {
    }

    Handle getModelInstanceA() { return mInstA; }
    Handle getModelInstanceB() { return mInstB; }

    SAlign Manifold mManifold;
  private:
    float mInactiveTime;
    //Not used internally, but needed to identify the contact pair this constraint came from
    Handle mInstA;
    Handle mInstB;
  };

  SAlign class LocalContactConstraint: public LocalConstraint {
  public:
    static float sPositionSlop;
    static float sTimeToRemove;

    LocalContactConstraint() {}
    LocalContactConstraint(ContactConstraint& owner);

    void firstIteration();
    void lastIteration();
    float solve();
    float sSolve();
    void draw();

    //Pointer because warm starts are ultimately stored here. Only used in first and last iteration, so cache misses shouldn't hit too hard
    Manifold* mManifold;

  private:
    void _setupContactJacobian(float massA, const Mat3& inertiaA, float massB, const Mat3& inertiaB);
    void _setupFrictionJacobian(float massA, const Mat3& inertiaA, float massB, const Mat3& inertiaB);
    float _solveContact(int i);
    float _solveFriction(int i);

    SAlign ContactBlock mContactBlock;
    SAlign FrictionBlock mFrictionBlock;
    SAlign ConstraintObjBlock mBlockObjA;
    SAlign ConstraintObjBlock mBlockObjB;
    float* mInactiveTime;
    bool* mShouldRemove;
  };
}