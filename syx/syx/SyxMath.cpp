#include "Precompile.h"

namespace Syx {
  Vec3 TriangleNormal(const Vec3& a, const Vec3& b, const Vec3& c) {
    return (b - a).Cross(c - a);
  }

  float HalfPlaneD(const Vec3& normal, const Vec3& onPlane) {
    return -(normal.Dot(onPlane));
  }

  float HalfPlaneSignedDistance(const Vec3& normal, float d, const Vec3& point) {
    return normal.Dot(point) + d;
  }

  float HalfPlaneSignedDistance(const Vec3& normal, const Vec3& onPlane, const Vec3& point) {
    return HalfPlaneSignedDistance(normal, HalfPlaneD(normal, onPlane), point);
  }

  Vec3 BarycentricToPoint(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& bary) {
    return a*bary.x + b*bary.y + c*bary.z;
  }

  Vec3 PointToBarycentric(const Vec3& aToB, const Vec3& aToC, const Vec3& aToP) {
    const Vec3& v0 = aToB;
    const Vec3& v1 = aToC;
    const Vec3& v2 = aToP;
    float d00 = v0.Dot(v0);
    float d01 = v0.Dot(v1);
    float d11 = v1.Dot(v1);
    float d20 = v2.Dot(v0);
    float d21 = v2.Dot(v1);
    float denom = d00*d11 - d01*d01;
    if(denom < SYX_EPSILON*SYX_EPSILON)
      return Vec3::Zero;
    float invDenom = 1.0f/denom;
    Vec3 result;
    //Area of cap
    result.y = (d11*d20 - d01*d21)*invDenom;
    //Area of abp
    result.z = (d00*d21 - d01*d20)*invDenom;
    //Area of bcp
    result.x = 1.0f - result.y - result.z;
    return result;
  }

  Vec3 PointToBarycentric(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& point) {
    return PointToBarycentric(b - a, c - a, point - a);
  }

  bool ValidBarycentric(const Vec3& bary) {
    float sum = 0.0f;
    for(int i = 0; i < 3; ++i) {
      float b = bary[i];
      //Greater than 1 is also invalid, but in that case there must be negatives, so we'll catch it
      if(b < 0.0f)
        return false;
      sum += b;
    }

    return std::abs(1.0f - sum) < SYX_EPSILON;
  }

  bool IsWithinTri(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& point, float epsilon) {
    Vec3 aToB = b - a;
    Vec3 bToC = c - b;
    Vec3 cToA = a - c;
    Vec3 normal = aToB.Cross(bToC);

    if(aToB.Cross(normal).Dot(point - a) > epsilon)
      return false;
    if(bToC.Cross(normal).Dot(point - b) > epsilon)
      return false;
    if(cToA.Cross(normal).Dot(point - c) > epsilon)
      return false;
    return true;
  }

  void GetOutwardTriPlanes(const Vec3& a, const Vec3& b, const Vec3& c, Vec3& resultA, Vec3& resultB, Vec3& resultC, bool normalized) {
    Vec3 aToB = b - a;
    Vec3 aToC = c - a;
    Vec3 bToC = c - b;
    Vec3 normal = aToB.Cross(aToC);

    resultA = aToB.Cross(normal);
    if(normalized)
      resultA.SafeNormalize();
    resultA.w = -resultA.Dot(a);

    resultB = bToC.Cross(normal);
    if(normalized)
      resultB.SafeNormalize();
    resultB.w = -resultB.Dot(b);

    resultC = -aToC.Cross(normal);
    if(normalized)
      resultC.SafeNormalize();
    resultC.w = -resultC.Dot(c);
  }

  size_t CombineHash(size_t lhs, size_t rhs) {
    //http://stackoverflow.com/questions/5889238/why-is-xor-the-default-way-to-combine-hashes
    lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
    return lhs;
  }

  float RandFloat(void) {
    double r = static_cast<double>(std::rand());
    r /= static_cast<double>(RAND_MAX);
    return static_cast<float>(r);
  }

  float RandFloat(float min, float max) {
    return RandFloat()*(max - min) + min;
  }

  Vec3 RandOnSphere(void) {
    //https://en.wikipedia.org/wiki/Spherical_coordinate_system
    float theta = RandFloat(0.0f, SYX_PI);
    float phi = RandFloat(0.0f, SYX_2_PI);
    float sinTheta = std::sin(theta);
    return Vec3(sinTheta*std::cos(phi), sinTheta*std::sin(phi), std::cos(theta));
  }

  Mat3 TensorTransform(const Mat3& tensor, const Vec3& toPoint, float mass) {
    //Parallel axis theorem
    float xx = toPoint.x*toPoint.x;
    float yy = toPoint.y*toPoint.y;
    float zz = toPoint.z*toPoint.z;
    float xy = -mass*toPoint.x*toPoint.y;
    float xz = -mass*toPoint.x*toPoint.z;
    float yz = -mass*toPoint.y*toPoint.z;
    return tensor + Mat3(mass*(yy + zz),             xy,             xz,
                                        xy, mass*(xx + zz),             yz,
                                        xz,             yz, mass*(xx + yy));
  }

  Mat3 TensorTransform(const Mat3& tensor, const Mat3& rotation) {
    return  rotation * tensor * rotation.Transposed();
  }

  //Given point on line p = t*V and
  //Point on ellipse e =  x^2/a^2 + y^2/b^2
  //Plug in t*Vx and t*Vy and solve for t
  float EllipseLineIntersect2d(const Vec2& line, const Vec2& ellipseScale) {
    return SafeDivide(ellipseScale.x*ellipseScale.y, std::sqrt(ellipseScale.x*ellipseScale.x*line.y*line.y + ellipseScale.y*ellipseScale.y*line.x*line.x), SYX_EPSILON);
  }

  //Given point on line p = S + t*V and
  //Point on ellipse e =  x^2/a^2 + y^2/b^2
  //Plug in Sx + t*Vx and Sy + t*Vy and solve for t
  float EllipseLineIntersect2d(const Vec2& lineStart, const Vec2& lineDir, const Vec2& ellipseScale) {
    float a2 = ellipseScale.x*ellipseScale.x;
    float b2 = ellipseScale.y*ellipseScale.y;
    float r = lineDir.x, r2 = lineDir.x*lineDir.x;
    float s = lineDir.y, s2 = lineDir.y*lineDir.y;
    float p = lineStart.x, p2 = lineStart.x*lineStart.x;
    float q = lineStart.y, q2 = lineStart.y*lineStart.y;
    return -SafeDivide(std::sqrt(a2*b2*(s2*(a2-p2)+b2*r2+2.0f*p*q*r*s-q2*r2)) + a2*q*s+b2*p*r, a2*s2+b2*r2, SYX_EPSILON);
  }

  void EllipsePointToNormal(const Vec2& point, const Vec2& ellipseScale, Vec2& normal) {
    if(std::abs(point.y) > SYX_EPSILON) {
      float slope = point.x/point.y;
      slope *= ellipseScale.y/ellipseScale.x;
      float absX = std::abs(slope * point.y);
      normal.x = point.x > 0.0f ? absX : -absX;
      normal.y = point.y;
    }
  }

#ifdef SENABLED
  SFloats STriangleNormal(SFloats a, SFloats b, SFloats c) {
    return SVec3::Cross(SSubAll(b, a), SSubAll(c, a));
  }
#endif
}