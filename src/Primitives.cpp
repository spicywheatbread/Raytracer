//
//  Primitives.h - Simple 3D Primitives with with Hierarchical Transformations
//
//  
//  (c) Kevin M. Smith  - 24 September 2018
//

#include "ofApp.h"
#include "Primitives.h"

Sphere::Sphere(glm::vec3 position, float radius, ofColor diffuse, float reflectivity, bool celShaded) {
    this->position = position;
    this->radius = radius;
    this->reflectivity = reflectivity;
    this->celShaded = celShaded;
    diffuseColor = diffuse;
}
Mesh::Mesh(glm::vec3 position, ofColor diffuse, string filePath) {
    this->position = position;
    diffuseColor = diffuse;
    vertices.push_back(glm::vec3(0, 0, 0));
    parseFile(filePath);
}
// Similar to above shortest intersection, iterates through every triangle in a mesh to find shortest intersection.
bool Mesh::intersect(const Ray& ray, glm::vec3& point, glm::vec3& normal) {
    bool hit = false;
    // These vars will store vertices of triangle closest to ray
    glm::vec3 v1, v2, v3;
    glm::vec2 bary;
    float distance; // Distance to that triangle
    float shortest = std::numeric_limits<float>::max();
    
    for(int i = 0; i < triangles.size(); i++) {
        if(glm::intersectRayTriangle(ray.position, ray.direction, vertices[triangles[i].v1], vertices[triangles[i].v2], vertices[triangles[i].v3], bary, distance)) {
            hit = true;
            if(distance > 0.001f && distance < shortest) {
                v1 = vertices[triangles[i].v1];
                v2 = vertices[triangles[i].v2];
                v3 = vertices[triangles[i].v3];
                shortest = distance;
            }
        }
    }
    if(hit) {
        Ray r = ray;
        point = r.evalPoint(shortest);
        normal = glm::normalize(glm::cross(v2 - v1, v3 - v1));
    }
    return hit;
}
void Mesh::draw() {
    for(Triangle tri: triangles) {
        ofDrawTriangle(vertices[tri.v1], vertices[tri.v2], vertices[tri.v3]);
    }
}
void Mesh::parseFile(string filePath) {
    ofFile file;
    if(!file.open(ofToDataPath(filePath), ofFile::ReadOnly)) {
        cout << "Could not open file";
    } else {
        ofBuffer buffer = file.readToBuffer();
        for(auto line : buffer.getLines()) {
            // Algorithm from Geeks for Geeks on splitting string by space
            stringstream stream(line);
            string s;
            vector<string> vec;
            if(!line.empty()) {
                while (getline(stream, s, ' ')) {
                    vec.push_back(s);
                }
                // Determine if we're parsing a vertice or a face
                if(vec.at(0) == "v") {
                    glm::vec3 vector = glm::vec3(std::stof(vec[1]), std::stof(vec[2]), std::stof(vec[3]));
                    this->addVertice(vector);
                } else if(vec.at(0) == "f") {
                    int indices[3] = {std::stoi(vec[1]), std::stoi(vec[2]), std::stoi(vec[3])};
                    this->addTriangle(indices[0], indices[1], indices[2]);
                }
            }
        }
    }
}
BaseLight::BaseLight(glm::vec3 position, ofColor diffuse) {
    previewLight.setup();
    previewLight.enable();
    previewLight.setDiffuseColor(diffuse);
    previewLight.setSpecularColor(diffuse);
    previewLight.setAttenuation(1.0f, 0, 0);
    previewLight.setPosition(position);
}
PointLight::PointLight(glm::vec3 position, float intensity, ofColor diffuse) : BaseLight(position, diffuse) {
    this->position = position;
    this->intensity = intensity;
    diffuseColor = diffuse;
    isSelectable = true;
    previewLight.setPointLight();
}
LightAnchor::LightAnchor(glm::vec3 position) : BaseLight(position, ofColor::white){
    this->position = position;
    isSelectable = true;
    intensity = 0.0f;
    previewLight.disable();
}
SpotLight::SpotLight(glm::vec3 position, float intensity, ofColor diffuse, float angle, LightAnchor* anchor) : BaseLight(position, diffuse) {
    this->position = position;
    this->intensity = intensity;
    diffuseColor = diffuse;
    this->angle = angle;
    isSelectable = true;
    this->anchor = anchor;
    previewLight.setSpotlight();
    previewLight.setSpotlightCutOff(angle);
}
float SpotLight::getIntensity(const Ray* ray) {
    glm::vec3 normalizedDirection = glm::normalize(anchor->position - position);
    float dot = glm::dot(normalizedDirection, -ray->direction);
    float cos = std::max(0.0f, dot);
    float spot_cos = std::cos(angle);
    if(cos < spot_cos) return 0.0f;
    return intensity;
}
void SpotLight::draw() {
    
}
Plane::Plane(glm::vec3 position, glm::vec3 normal, ofColor diffuse, float width, float height, ofImage* diffTex, ofImage* specTex, int tiles) {
    this->position = position;
    this->normal = normal;
    this->width = width;
    this->height = height;
    diffuseColor = diffuse;
    diffuseTexture = diffTex;
    specularTexture = specTex;
    this->tiles = tiles;
    
    plane.lookAt(normal);
}
Plane::Plane() {}

ofColor Plane::mapPlaneToTexture(glm::vec3 intersection, ofImage* texture) {
    /* To find the range of width and height, take the original width and height of plane, and divide by tiles
    to obtain the width and height of a sub tile. */
    
    float rangeW = width / tiles;
    float rangeH = height / tiles;
    float widthCoord = intersection.x;
    float heightCoord = intersection.y; // This is because we have hardcoded horizontal and vertical planes.
    
    if(normal == glm::vec3(0, 1, 0)) heightCoord = intersection.z; // Use the z instead of y if we have a horizontal plane.
    if(normal == glm::vec3(-1, 0, 0)) widthCoord = intersection.z;
    if(normal == glm::vec3(1, 0, 0)) widthCoord = -intersection.z;
        
    if(heightCoord < 0) rangeH = -rangeH; // Do this negative b/c of modulus:
    if(widthCoord < 0) rangeW = -rangeW; // If the intersection point has negative coordinates, adjust mapping range

    // Map the intersection point coordinates to the u, v normalized points.
    float u = ofMap(fmod(widthCoord, width / tiles), 0, rangeW, 0, 1);
    float v = ofMap(fmod(heightCoord, width / tiles), 0, rangeH, 0, 1);

    // Convert (u, v) points to points in the image
    float i = u * texture->getWidth() - 1;
    float j = v * texture->getHeight() - 1;
    
    return texture->getColor(i, j); // Return the mapped pixel's color
}

ofColor Plane::getDiffuseColor(glm::vec3 intersection) {
    if(diffuseTexture == nullptr) return diffuseColor;
    return mapPlaneToTexture(intersection, diffuseTexture);
}
ofColor Plane::getSpecularColor(glm::vec3 intersection) {
    if(specularTexture == nullptr) return specularColor;
    return mapPlaneToTexture(intersection, specularTexture);
}
void Plane::draw() {
    plane.setPosition(position);
    plane.setWidth(width);
    plane.setHeight(height);
    plane.setResolution(4, 4);
    plane.draw();
}

// Intersect Ray with Plane  (wrapper on glm::intersect*)
bool Plane::intersect(const Ray& ray, glm::vec3& point, glm::vec3& normalAtIntersect) {
    float dist;
    bool hit = glm::intersectRayPlane(ray.position, ray.direction, position, normal, dist);
    if (hit) {
        // If we find a hit but the distance is negative, then the ray had to travel backwards. No hit.
        if(dist < 0) hit = false;
        
        Ray r = ray;
        point = r.evalPoint(dist);
        float half_w = width / 2;
        float half_h = height / 2;
        
        if(normal == glm::vec3(0, 1, 0)) { // Horizontal Plane
            if(point.x < position.x - half_w || point.x > position.x + half_w || point.z < position.z - half_h || point.z > position.z + half_h) {
                hit = false;
            }
        } else if(normal == glm::vec3(0, 0, 1)) { // Vertical plane facing cam
            if(point.x < position.x - half_w || point.x > position.x + half_w || point.y < position.y - half_h || point.y > position.y + half_h) {
                hit = false;
            }
        } else { // vertical plane facing sides
            if(point.z < position.z - half_w || point.z > position.z + half_w || point.y < position.y - half_h || point.y > position.y + half_h) {
                hit = false;
            }
        }
        
    }
    
    normalAtIntersect = this->normal; // Update normal
    return (hit);
}
// Convert (u, v) to (x, y, z)
// We assume u,v is in [0, 1]
glm::vec3 ViewPlane::toWorld(float u, float v) {
    float w = width();
    float h = height();
    return (glm::vec3((u * w) + min.x, (v * h) + min.y, position.z));
}
RenderCam::RenderCam()  {
    position = glm::vec3(0, 0, 10);
    aim = glm::vec3(0, 0, -1);
}
// Get a ray from the current camera position to the (u, v) position on
// the ViewPlane
Ray RenderCam::getRay(float u, float v) {
    glm::vec3 pointOnPlane = view.toWorld(u, v);
    return(Ray(position, glm::normalize(pointOnPlane - position)));
}

// This could be drawn a lot simpler but I wanted to use the getRay call
// to test it at the corners.
void RenderCam::drawFrustum() {
    Ray r1 = getRay(0, 0);
    Ray r2 = getRay(0, 1);
    Ray r3 = getRay(1, 1);
    Ray r4 = getRay(1, 0);
    float dist = glm::length((view.toWorld(0, 0) - position));
    r1.draw(dist);
    r2.draw(dist);
    r3.draw(dist);
    r4.draw(dist);
}
void RenderCam::draw() {
    ofPushStyle();
    ofNoFill();
    ofDrawBox(position, 1.0);
    ofPopStyle();
}

