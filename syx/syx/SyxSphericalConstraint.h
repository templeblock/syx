#pragma once
#include "SyxConstraint.h"

namespace Syx {
  //Everything needed during solving loop in one compact structure to maximize cache coherence
  SAlign struct SphericalBlock {
    void setup(const Vec3& posA, const Vec3& posB, const Vec3& anchorA, const Vec3& anchorB, float massA, float massB, const Mat3& inertiaA, const Mat3& inertiaB);
    void applyImpulse(const Vec3& lambda, ConstraintObjBlock& a, ConstraintObjBlock& b);
    float solve(ConstraintObjBlock& a, ConstraintObjBlock& b);
    float sSolve(SFloats& linVelA, SFloats& angVelA, SFloats& linVelB, SFloats& angVelB);

    //No need to store linear portion, it's always unit x,y,z
    SAlign Vec3 mAngularA[3];
    SAlign Vec3 mAngularB[3];
    SAlign Vec3 mAngularMA[3];
    SAlign Vec3 mAngularMB[3];
    SAlign Vec3 mLambdaSum;
    SAlign Vec3 mBias;
    SAlign Mat3 mConstraintMass;
    //Don't need to store premultiplied x,y,z, since they're just cardinal axes with mass as nonzero component
    float mMassA;
    float mMassB;
  };

  SAlign struct SwingTwistBlock {
    void setup(const Quat& refA, const Quat& refB,
      const Quat& rotA, const Quat& rotB,
      const Mat3& inertiaA, const Mat3& inertiaB,
      float maxSwingX, float maxSwingY,
      float minTwist, float maxTwist,
      float maxAngularImpulse);
    void applyImpulse(int index, float lambda, ConstraintObjBlock& a, ConstraintObjBlock& b);
    float solve(ConstraintObjBlock& a, ConstraintObjBlock& b);
    float sSolve(SFloats& angVelA, SFloats& angVelB);

    //Given swing frame with twist removed, computes angle anglular error within ellipse mapped on sphere surface. As such, the limite and axis are given back as they will be different if ellipse isn't a circle
    static void computeSwingError(const Quat& swingFrame, float maxSwingX, float maxSwingY, float& swingError, Vec3& swingAxis, float& swingAngle);
    static void computeTwistError(const Quat& twistFrame, float& twistAngle, Vec3& twistAxis);

    //Swing, twist, orthogonal for friction
    SAlign Vec3 mAngular[3];
    SAlign Vec3 mAngularMA[3];
    SAlign Vec3 mAngularMB[3];
    float mLambdaSum[3];
    //No bias for friction 3rd friction axis
    float mBias[2];
    float mConstraintMass[3];
    float mMaxSum[3];
    char mEnforceDir[2];
    char mPadding[2];
  };

  SAlign class SphericalConstraint: public Constraint {
  public:
    friend class LocalSphericalConstraint;

    DeclareIntrusiveNode(SphericalConstraint);

    SphericalConstraint(PhysicsObject* a = nullptr, PhysicsObject* b = nullptr, Handle handle = SyxInvalidHandle)
      : Constraint(ConstraintType::Spherical, a, b, handle)
      , mLinearWarmStart(Vec3::Zero)
      , mAngularWarmStart{0.0f} {
    }

    virtual void setLocalAnchor(const Vec3& anchor, ConstraintObj obj) override {
      if(obj == ConstraintObj::A)
        mAnchorA = anchor;
      else
        mAnchorB = anchor;
    }
    virtual const Vec3& getLocalAnchor(ConstraintObj obj) const override {
      return obj == ConstraintObj::A ? mAnchorA : mAnchorB;
    }
    //Set swing axis given in a's local space
    void setSwingFrame(const Quat& aFrame);
    void getAngularReferences(Quat& ra, Quat& rb) const;
    void getSwingLimits(float& maxRadsX, float& maxRadsY) const;
    void setSwingLimits(float maxRadsX, float maxRadsY);
    void getTwistLimits(float& minRads, float& maxRads) const;
    void setTwistLimits(float minRads, float maxRads);
    void setMaxAngularImpulse(float max);
    float getMaxAngularImpulse() const;

  private:
    SAlign Vec3 mAnchorA;
    SAlign Vec3 mAnchorB;
    SAlign Vec3 mLinearWarmStart;
    //Local space reference for a and b
    SAlign Quat mRefA;
    SAlign Quat mRefB;
    float mMaxSwingRadsX;
    float mMaxSwingRadsY;
    float mMinTwistRads;
    float mMaxTwistRads;
    float mMaxAngularImpulse;
    float mAngularWarmStart[3];
  };

  SAlign class LocalSphericalConstraint: public LocalConstraint {
  public:
    void firstIteration();
    void lastIteration();
    float solve();
    float sSolve();
    void draw();

    static const float sLinearSlop;
    static const float sAngularSlop;

  private:

    SAlign SphericalBlock mLinearBlock;
    SAlign SwingTwistBlock mAngularBlock;
    SAlign ConstraintObjBlock mBlockObjA;
    SAlign ConstraintObjBlock mBlockObjB;
  };
}