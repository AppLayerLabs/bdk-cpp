#include "snailtracer.h"

SnailTracer::SnailTracer(
  int256_t w, int256_t h, const Address& address, const Address& creator, const uint64_t& chainId
) : DynamicContract("SnailTracer", address, creator, chainId), width_(w), height_(h) {
  // Initialize the image and rendering parameters
  camera_ = Ray(Vector(50000000, 52000000, 295600000), norm(Vector(0, -42612, -1000000)), 0, false);
  deltaX_ = Vector(width_.get() * 513500 / height_.get(), 0, 0);
  deltaY_ = div(mul(norm(cross(deltaX_.raw(), get<1>(camera_))), 513500), 1000000); // camera.direction

  // Initialize the scene bounding boxes
  spheres_.push_back(Sphere(100000000000, Vector(100001000000, 40800000, 81600000), Vector(0, 0, 0), Vector(750000, 250000, 250000), Material::Diffuse));
  spheres_.push_back(Sphere(100000000000, Vector(-99901000000, 40800000, 81600000), Vector(0, 0, 0), Vector(250000, 250000, 750000), Material::Diffuse));
  spheres_.push_back(Sphere(100000000000, Vector(50000000, 40800000, 100000000000), Vector(0, 0, 0), Vector(750000, 750000, 750000), Material::Diffuse));
  spheres_.push_back(Sphere(100000000000, Vector(50000000, 40800000, -99830000000), Vector(0, 0, 0), Vector(0, 0, 0), Material::Diffuse));
  spheres_.push_back(Sphere(100000000000, Vector(50000000, 100000000000, 81600000), Vector(0, 0, 0), Vector(750000, 750000, 750000), Material::Diffuse));
  spheres_.push_back(Sphere(100000000000, Vector(50000000, -99918400000, 81600000), Vector(0, 0, 0), Vector(750000, 750000, 750000), Material::Diffuse));

  // Initialize the reflective sphere and the light source
  spheres_.push_back(Sphere(16500000, Vector(27000000, 16500000, 47000000), Vector(0, 0, 0), Vector(999000, 999000, 999000), Material::Specular));
  spheres_.push_back(Sphere(600000000, Vector(50000000, 681330000, 81600000), Vector(12000000, 12000000, 12000000), Vector(0, 0, 0), Material::Diffuse));
  //spheres_.push_back(Sphere(16500000, Vector(73000000, 16500000, 78000000), Vector(0, 0, 0), Vector(999000, 999000, 999000), Material::Refractive));
  // NOTE: this last line is commented in the original code, keeping it 1:1

  // Ethereum logo front triangles
  triangles_.push_back(Triangle(Vector(56500000, 25740000, 78000000), Vector(73000000, 25740000, 94500000), Vector(73000000, 49500000, 78000000), Vector(0, 0, 0), Vector(0, 0, 0), Vector(999000, 999000, 999000), Material::Specular));
  triangles_.push_back(Triangle(Vector(56500000, 23760000, 78000000), Vector(73000000,        0, 78000000), Vector(73000000, 23760000, 94500000), Vector(0, 0, 0), Vector(0, 0, 0), Vector(999000, 999000, 999000), Material::Specular));
  triangles_.push_back(Triangle(Vector(89500000, 25740000, 78000000), Vector(73000000, 49500000, 78000000), Vector(73000000, 25740000, 94500000), Vector(0, 0, 0), Vector(0, 0, 0), Vector(999000, 999000, 999000), Material::Specular));
  triangles_.push_back(Triangle(Vector(89500000, 23760000, 78000000), Vector(73000000, 23760000, 94500000), Vector(73000000,        0, 78000000), Vector(0, 0, 0), Vector(0, 0, 0), Vector(999000, 999000, 999000), Material::Specular));

  // Ethereum logo back triangles
  triangles_.push_back(Triangle(Vector(56500000, 25740000, 78000000), Vector(73000000, 49500000, 78000000), Vector(73000000, 25740000, 61500000), Vector(0, 0, 0), Vector(0, 0, 0), Vector(999000, 999000, 999000), Material::Specular));
  triangles_.push_back(Triangle(Vector(56500000, 23760000, 78000000), Vector(73000000, 23760000, 61500000), Vector(73000000,        0, 78000000), Vector(0, 0, 0), Vector(0, 0, 0), Vector(999000, 999000, 999000), Material::Specular));
  triangles_.push_back(Triangle(Vector(89500000, 25740000, 78000000), Vector(73000000, 25740000, 61500000), Vector(73000000, 49500000, 78000000), Vector(0, 0, 0), Vector(0, 0, 0), Vector(999000, 999000, 999000), Material::Specular));
  triangles_.push_back(Triangle(Vector(89500000, 23760000, 78000000), Vector(73000000,        0, 78000000), Vector(73000000, 23760000, 61500000), Vector(0, 0, 0), Vector(0, 0, 0), Vector(999000, 999000, 999000), Material::Specular));

  // Ethereum logo middle rectangles
  triangles_.push_back(Triangle(Vector(56500000, 25740000, 78000000), Vector(73000000, 25740000, 61500000), Vector(89500000, 25740000, 78000000), Vector(0, 0, 0), Vector(0, 0, 0), Vector(999000, 999000, 999000), Material::Specular));
  triangles_.push_back(Triangle(Vector(56500000, 25740000, 78000000), Vector(89500000, 25740000, 78000000), Vector(73000000, 25740000, 94500000), Vector(0, 0, 0), Vector(0, 0, 0), Vector(999000, 999000, 999000), Material::Specular));
  triangles_.push_back(Triangle(Vector(56500000, 23760000, 78000000), Vector(89500000, 23760000, 78000000), Vector(73000000, 23760000, 61500000), Vector(0, 0, 0), Vector(0, 0, 0), Vector(999000, 999000, 999000), Material::Specular));
  triangles_.push_back(Triangle(Vector(56500000, 23760000, 78000000), Vector(73000000, 23760000, 94500000), Vector(89500000, 23760000, 78000000), Vector(0, 0, 0), Vector(0, 0, 0), Vector(999000, 999000, 999000), Material::Specular));

  // Calculate all the triangle surface normals
  for (std::size_t i = 0; i < triangles_.size(); i++) { // NOTE: original code uses int for i
    Triangle& tri = triangles_[i];
    Vector& triA = std::get<0>(tri);
    Vector& triB = std::get<1>(tri);
    Vector& triC = std::get<2>(tri);
    Vector& triNor = std::get<3>(tri);
    triNor = norm(cross(sub(triB, triA), sub(triC, triA)));
  }

  // Commit and register vars and functions
  width_.commit();
  height_.commit();
  camera_.commit();
  deltaX_.commit();
  deltaY_.commit();
  spheres_.commit();
  triangles_.commit();
  registerContractFunctions();
  width_.enableRegister();
  height_.enableRegister();
  camera_.enableRegister();
  deltaX_.enableRegister();
  deltaY_.enableRegister();
  spheres_.enableRegister();
  triangles_.enableRegister();
}

SnailTracer::SnailTracer(
  const Address& address,
  const DB& db
) : DynamicContract(address, db) {
  // TODO (optional): load vars from DB (see dump())
  width_.commit();
  height_.commit();
  camera_.commit();
  deltaX_.commit();
  deltaY_.commit();
  spheres_.commit();
  triangles_.commit();
  registerContractFunctions();
  width_.enableRegister();
  height_.enableRegister();
  camera_.enableRegister();
  deltaX_.enableRegister();
  deltaY_.enableRegister();
  spheres_.enableRegister();
  triangles_.enableRegister();
}

SnailTracer::~SnailTracer() {};

std::tuple<uint8_t, uint8_t, uint8_t> SnailTracer::TracePixel(const int256_t& x, const int256_t& y, const uint256_t& spp) {
  Vector color = trace(x, y, spp);
  auto& [colorX, colorY, colorZ] = color;
  return std::make_tuple(uint8_t(colorX), uint8_t(colorY), uint8_t(colorZ));
}

Bytes SnailTracer::TraceScanline(const int256_t& y, const int256_t& spp) {
  for (int256_t x = 0; x < width_.get(); x++) {
    Vector color = trace(x, y, spp);
    auto& [colorX, colorY, colorZ] = color;
    buffer_.push_back(uint8_t(colorX));
    buffer_.push_back(uint8_t(colorY));
    buffer_.push_back(uint8_t(colorZ));
  }
  return buffer_.get();
}

Bytes SnailTracer::TraceImage(const int256_t& spp) {
  for (int256_t y = height_.get() - 1; y >= 0; y--) {
    for (int256_t x = 0; x < width_.get(); x++) {
      Vector color = trace(x, y, spp);
      auto& [colorX, colorY, colorZ] = color;
      buffer_.push_back(uint8_t(colorX));
      buffer_.push_back(uint8_t(colorY));
      buffer_.push_back(uint8_t(colorZ));
    }
  }
  return buffer_.get();
}

std::tuple<uint8_t, uint8_t, uint8_t> SnailTracer::Benchmark() {
  // Configure scene for benchmarking
  width_ = 1024; height_ = 768;
  deltaX_ = Vector(width_.get() * 513500 / height_.get(), 0, 0);
  deltaY_ = div(mul(norm(cross(deltaX_.raw(), get<1>(camera_))), 513500), 1000); // camera.direction
  // Trace a few pixels and collect their colors (sanity check)
  Vector color;
  color = add(color, trace(512, 384, 8)); // Flat diffuse surface, opposite wall
  color = add(color, trace(325, 540, 8)); // Reflective surface mirroring left wall
  color = add(color, trace(600, 600, 8)); // Refractive surface reflecting right wall
  color = add(color, trace(522, 524, 8)); // Reflective surface mirroring the refractive surface reflecting the light
  color = div(color, 4);
  auto& [x, y, z] = color;
  return std::make_tuple(uint8_t(x), uint8_t(y), uint8_t(z));
}

Vector SnailTracer::trace(const int256_t& x, const int256_t& y, const int256_t& spp) {
  seed_ = uint32_t(y * width_.get() + x); // Deterministic image irrelevant of render chunks
  Vector color;
  for (int256_t k = 0; k < spp; k++) {
    Vector pixel = add(div(add(
      mul(deltaX_.raw(), (1000000 * x + rand() % 500000) / width_.get() - 500000),
      mul(deltaY_.raw(), (1000000 * y + rand() % 500000) / height_.get() - 500000)
    ), 1000000), get<1>(camera_)); // camera.direction
    Ray ray = Ray(add(get<0>(camera_), mul(pixel, 140)), norm(pixel), 0, false); // camera.origin
    color = add(color, div(radiance(ray), spp));
  }
  return div(mul(clamp(color), 255), 1000000);
}

uint32_t SnailTracer::rand() {
  seed_ = 1103515245 * seed_.get() + 12345;
  return seed_.get();
}

int256_t SnailTracer::clamp(const int256_t& x) {
  if (x < 0) return 0;
  if (x > 1000000) return 1000000;
  return x;
}

int256_t SnailTracer::sqrt(const int256_t& x) {
  int256_t z = (x + 1) / 2;
  int256_t y = x;
  while (z < y) {
    y = z;
    z = (x/z + z) / 2;
  }
  return y;
}

int256_t SnailTracer::sin(int256_t x) {
  // Ensure x is between [0, 2PI) (Taylor expansion is picky with large numbers)
  while (x < 0) x += 6283184;
  while (x >= 6283184) x -= 6283184;
  // Calculate the sin based on the Taylor series
  int256_t s = 1;
  int256_t n = x;
  int256_t d = 1;
  int256_t f = 2;
  int256_t y;
  while (n > d) {
    y += s * n / d;
    n = n * x * x / 1000000 / 1000000;
    d *= f * (f + 1);
    s *= -1;
    f += 2;
  }
  return y;
}

int256_t SnailTracer::cos(const int256_t& x) {
  int256_t s = sin(x);
  return sqrt(1000000000000 - s*s);
}

int256_t SnailTracer::abs(const int256_t& x) {
  if (x > 0) return x;
  return -x;
}

Vector SnailTracer::add(const Vector& u, const Vector& v) {
  auto& [ux, uy, uz] = u;
  auto& [vx, vy, vz] = v;
  return Vector(ux+vx, uy+vy, uz+vz);
}

Vector SnailTracer::sub(const Vector& u, const Vector& v) {
  auto& [ux, uy, uz] = u;
  auto& [vx, vy, vz] = v;
  return Vector(ux-vx, uy-vy, uz-vz);
}

Vector SnailTracer::mul(const Vector& u, const Vector& v) {
  auto& [ux, uy, uz] = u;
  auto& [vx, vy, vz] = v;
  return Vector(ux*vx, uy*vy, uz*vz);
}

Vector SnailTracer::mul(const Vector& v, const int256_t& m) {
  auto& [vx, vy, vz] = v;
  return Vector(m*vx, m*vy, m*vz);
}

Vector SnailTracer::div(const Vector& v, const int256_t& d) {
  auto& [vx, vy, vz] = v;
  return Vector(vx/d, vy/d, vz/d);
}

int256_t SnailTracer::dot(const Vector& u, const Vector& v) {
  auto& [ux, uy, uz] = u;
  auto& [vx, vy, vz] = v;
  return ux*vx + uy*vy + uz*vz;
}

Vector SnailTracer::cross(const Vector& u, const Vector& v) {
  auto& [ux, uy, uz] = u;
  auto& [vx, vy, vz] = v;
  return Vector(uy*vz - uz*vy, uz*vx - ux*vz, ux*vy - uy*vx);
}

Vector SnailTracer::norm(const Vector& v) {
  auto& [vx, vy, vz] = v;
  int256_t length = sqrt(vx*vx + vy*vy + vz*vz);
  return Vector(vx * 1000000 / length, vy * 1000000 / length, vz * 1000000 / length);
}

Vector SnailTracer::clamp(const Vector& v) {
  auto& [vx, vy, vz] = v;
  return Vector(clamp(vx), clamp(vy), clamp(vz));
}

int256_t SnailTracer::intersect(const Sphere& s, const Ray& r) {
  const int256_t& sRad = std::get<0>(s);
  const Vector& sPos = std::get<1>(s);
  const Vector& rOri = std::get<0>(r);
  const Vector& rDir = std::get<1>(r);

  Vector op = sub(sPos, rOri);
  int256_t b = dot(op, rDir) / 1000000;
  // Bail out if ray misses the sphere
  int256_t det = b*b - dot(op, op) + sRad*sRad;
  if (det < 0) return 0;
  // Calculate the closer intersection point
  det = sqrt(det);
  if (b - det > 1000) return b - det;
  if (b + det > 1000) return b + det;
  return 0;
}

int256_t SnailTracer::intersect(const Triangle& t, const Ray& r) {
  const Vector& tA = std::get<0>(t);
  const Vector& tB = std::get<1>(t);
  const Vector& tC = std::get<2>(t);
  const Vector& rOri = std::get<0>(r);
  const Vector& rDir = std::get<1>(r);

  Vector e1 = sub(tB, tA);
  Vector e2 = sub(tC, tA);
  Vector p = cross(rDir, e2);
  // Bail out if ray is parallel to the triangle
  int256_t det = dot(e1, p) / 1000000;
  if (det > -1000 && det < 1000) return 0;
  // Calculate and test the 'u' parameter
  Vector d = sub(rOri, tA);
  int256_t u = dot(d, p) / det;
  if (u < 0 || u > 1000000) return 0;
  // Calculate and test the 'v' parameter
  Vector q = cross(d, e1);
  int256_t v = dot(rDir, q) / det;
  if (v < 0 || u + v > 1000000) return 0;
  // Calculate and return the distance
  int256_t dist = dot(e2, q) / det;
  if (dist < 1000) return 0;
  return dist;
}

Vector SnailTracer::radiance(Ray& ray) {
  // Place a limit on the depth to prevent stack overflows
  auto& [rOri, rDir, rDep, rRef] = ray;
  if (rDep > 10) return Vector(0,0,0);
  // Find the closest object of intersection
  auto [dist, p, id] = traceray(ray);
  if (dist == 0) return Vector(0,0,0);
  Sphere sphere;
  Triangle triangle;
  Vector color;
  Vector emission;
  if (p == Primitive::PSphere) {
    sphere = spheres_[std::size_t(id)]; // NOTE: original code uses int as id
    color = std::get<3>(sphere); // sphere.color
    emission = std::get<2>(sphere); // sphere.emission
  } else {
    triangle = triangles_[std::size_t(id)]; // NOTE: original code uses int as id
    color = std::get<5>(triangle); // triangle.color
    emission = std::get<4>(triangle); // triangle.emission
  }
  // After a number of reflections, randomly stop radiance calculation
  auto& [colorX, colorY, colorZ] = color;
  int256_t ref = 1;
  if (colorZ > ref) ref = colorZ; // original code checks Z twice instead of X, not sure if typo or intended, keeping it 1:1
  if (colorY > ref) ref = colorY;
  if (colorZ > ref) ref = colorZ;
  rDep++;
  if (rDep > 5) {
    if (rand() % 1000000 < ref) {
      color = div(mul(color, 1000000), ref);
    } else {
      return emission;
    }
  }
  // Calculate the primitive dependent radiance
  Vector result;
  if (p == Primitive::PSphere) {
    result = radiance(ray, sphere, dist);
  } else {
    result = radiance(ray, triangle, dist);
  }
  return add(emission, div(mul(color, result), 1000000));
}

Vector SnailTracer::radiance(const Ray& ray, const Sphere& obj, const int256_t& dist) {
  const Vector& rOri = std::get<0>(ray);
  const Vector& rDir = std::get<1>(ray);
  const Vector& sPos = std::get<1>(obj);
  const Material& sRef = std::get<4>(obj);

  // Calculate the sphere intersection point and normal vectors for recursion
  Vector intersect = add(rOri, div(mul(rDir, dist), 1000000));
  Vector normal = norm(sub(intersect, sPos));
  if (sRef == Material::Diffuse) { // For diffuse reflectivity
    if (dot(normal, rDir) >= 0) {
      normal = mul(normal, -1);
    }
    return diffuse(ray, intersect, normal);
  } else { // For specular reflectivity
    return specular(ray, intersect, normal);
  }
}

Vector SnailTracer::radiance(const Ray& ray, const Triangle& obj, const int256_t& dist) {
  const Vector& rOri = std::get<0>(ray);
  const Vector& rDir = std::get<1>(ray);
  const bool& rRef = std::get<3>(ray);
  const Vector& tNor = std::get<3>(obj);

  // Calculate the triangle intersection point for refraction
  // We're cheating here, we don't have diffuse triangles :P
  Vector intersect = add(rOri, div(mul(rDir, dist), 1000000));
  // Calculate the refractive indices based on whether we're in or out
  int256_t nnt = 666666; // (1 air / 1.5 glass)
  if (rRef) nnt = 1500000; // (1.5 glass / 1 air)
  int256_t ddn = dot(tNor, rDir) / 1000000;
  if (ddn >= 0) ddn = -ddn;
  // If the angle is too shallow, all light is reflected
  int256_t cos2t = 1000000000000 - nnt * nnt * (1000000000000 - ddn * ddn) / 1000000000000;
  if (cos2t < 0) return specular(ray, intersect, tNor);
  return refractive(ray, intersect, tNor, nnt, ddn, cos2t);
}

Vector SnailTracer::diffuse(const Ray& ray, const Vector& intersect, const Vector& normal) {
  const int256_t& normalX = std::get<0>(normal);
  const int256_t& rDep = std::get<2>(ray);
  const bool& rRef = std::get<3>(ray);

  // Generate a random angle and distance from center
  int256_t r1 = int256_t(6283184) * (rand() % 1000000) / 1000000;
  int256_t r2 = rand() % 1000000;
  int256_t r2s = sqrt(r2) * 1000;
  // Create orthonormal coordinate frame
  Vector u = (abs(normalX) > 100000) ? Vector(0, 1000000, 0) : Vector(1000000, 0, 0);
  u = norm(cross(u, normal));
  Vector v = norm(cross(normal, u));
  // Generate the random reflection ray and continue path tracing
  u = norm(add(add(mul(u, cos(r1) * r2s / 1000000), mul(v, sin(r1) * r2s / 1000000)), mul(normal, sqrt(1000000 - r2) * 1000)));
  Ray rayy(intersect, u, rDep, rRef);
  return radiance(rayy);
}

Vector SnailTracer::specular(const Ray& ray, const Vector& intersect, const Vector& normal) {
  const Vector& rDir = std::get<1>(ray);
  const int256_t& rDep = std::get<2>(ray);
  const bool& rRef = std::get<3>(ray);

  Vector reflection = norm(sub(rDir, mul(normal, 2 * dot(normal, rDir) / 1000000)));
  Ray rayy(intersect, reflection, rDep, rRef);
  return radiance(rayy);
}

Vector SnailTracer::refractive(
  const Ray& ray, const Vector& intersect, const Vector& normal,
  const int256_t& nnt, const int256_t& ddn, const int256_t& cos2t
) {
  const Vector& rDir = std::get<1>(ray);
  const int256_t& rDep = std::get<2>(ray);
  const bool& rRef = std::get<3>(ray);

  // Calculate the refraction rays for fresnel effects
  int256_t sign = (rRef) ? 1 : -1;
  Vector refraction = norm(div(sub(mul(rDir, nnt), mul(normal, sign * (ddn * nnt / 1000000 + sqrt(cos2t)))), 1000000));
  // Calculate the fresnel probabilities
  int256_t c = (!rRef) ? 1000000 - dot(refraction, normal) / 1000000 : 1000000 + ddn;
  int256_t re = 40000 + (1000000 - 40000) * c * c * c * c * c / int256_t("1000000000000000000000000000000");
  // Split a direct hit, otherwise trace only one ray
  if (rDep <= 2) {
    Ray ray(intersect, refraction, rDep, !rRef);
    refraction = mul(radiance(ray), 1000000 - re); // Reuse refraction variable (lame)
    refraction = add(refraction, mul(specular(ray, intersect, normal), re));
    return div(refraction, 1000000);
  }
  if (rand() % 1000000 < 250000 + re / 2) {
    return div(mul(specular(ray, intersect, normal), re), 250000 + re / 2);
  }
  Ray rayy(intersect, refraction, rDep, !rRef);
  return div(mul(radiance(rayy), 1000000 - re), 750000 - re / 2);
}

std::tuple<int256_t, Primitive, uint256_t> SnailTracer::traceray(const Ray& ray) {
  int256_t dist = 0; Primitive p; uint256_t id;

  // Intersect the ray with all the spheres
  for (std::size_t i = 0; i < spheres_.size(); i++) { // NOTE: original code uses int for i
    int256_t d = intersect(spheres_[i], ray);
    if (d > 0 && (dist == 0 || d < dist)) {
      dist = d; p = Primitive::PSphere; id = i;
    }
  }
  // Intersect the ray with all the triangles
  for (std::size_t i = 0; i < triangles_.size(); i++) { // NOTE: original code uses int for i
    int256_t d = intersect(triangles_[i], ray);
    if (d > 0 && (dist == 0 || d < dist)) {
      dist = d; p = Primitive::PTriangle; id = i;
    }
  }

  return std::make_tuple(dist, p, id);
}

void SnailTracer::registerContractFunctions() {
  registerContract();
  this->registerMemberFunction("TracePixel", &SnailTracer::TracePixel, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("TraceScanline", &SnailTracer::TraceScanline, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("TraceImage", &SnailTracer::TraceImage, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("Benchmark", &SnailTracer::Benchmark, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("trace", &SnailTracer::trace, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("rand", &SnailTracer::rand, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("clamp", static_cast<int256_t(SnailTracer::*)(const int256_t&)>(&SnailTracer::clamp), FunctionTypes::NonPayable, this);
  this->registerMemberFunction("sqrt", &SnailTracer::sqrt, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("sin", &SnailTracer::sin, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("cos", &SnailTracer::cos, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("abs", &SnailTracer::abs, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("add", &SnailTracer::add, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("sub", &SnailTracer::sub, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("mul", static_cast<Vector(SnailTracer::*)(const Vector&, const Vector&)>(&SnailTracer::mul), FunctionTypes::NonPayable, this);
  this->registerMemberFunction("mul", static_cast<Vector(SnailTracer::*)(const Vector&, const int256_t&)>(&SnailTracer::mul), FunctionTypes::NonPayable, this);
  this->registerMemberFunction("div", &SnailTracer::div, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("dot", &SnailTracer::dot, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("cross", &SnailTracer::cross, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("norm", &SnailTracer::norm, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("clamp", static_cast<Vector(SnailTracer::*)(const Vector&)>(&SnailTracer::clamp), FunctionTypes::NonPayable, this);
  this->registerMemberFunction("intersect", static_cast<int256_t(SnailTracer::*)(const Sphere&, const Ray&)>(&SnailTracer::intersect), FunctionTypes::NonPayable, this);
  this->registerMemberFunction("intersect", static_cast<int256_t(SnailTracer::*)(const Triangle&, const Ray&)>(&SnailTracer::intersect), FunctionTypes::NonPayable, this);
  this->registerMemberFunction("radiance", static_cast<Vector(SnailTracer::*)(Ray&)>(&SnailTracer::radiance), FunctionTypes::NonPayable, this);
  this->registerMemberFunction("radiance", static_cast<Vector(SnailTracer::*)(const Ray&, const Sphere&, const int256_t&)>(&SnailTracer::radiance), FunctionTypes::NonPayable, this);
  this->registerMemberFunction("radiance", static_cast<Vector(SnailTracer::*)(const Ray&, const Triangle&, const int256_t&)>(&SnailTracer::radiance), FunctionTypes::NonPayable, this);
  this->registerMemberFunction("diffuse", &SnailTracer::diffuse, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("specular", &SnailTracer::specular, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("refractive", &SnailTracer::refractive, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("traceray", &SnailTracer::traceray, FunctionTypes::NonPayable, this);
}

DBBatch SnailTracer::dump() const {
  DBBatch dbBatch = BaseContract::dump();
  // TODO (optional): save internal vars to DB
  // width_, height_, camera_, deltaX_, deltaY_, spheres_ and triangles_
  return dbBatch;
}

