#pragma once

#include "ofMain.h"
#include "glm/gtx/intersect.hpp"


//  General Purpose Ray class 
//
class Ray {
public:
    // Methods
    //
	Ray(glm::vec3 position, glm::vec3 direction) { this->position = position; this->direction = direction; }
	void draw(float time) { ofDrawLine(position, position + time * direction); }
	glm::vec3 evalPoint(float time) { return (position + time * direction); }

    // Variables
    //
	glm::vec3 position, direction;
};

class BaseLight;
class SceneObject {
public:
    // Methods
    //
    virtual void draw() {}
    virtual bool intersect(const Ray& ray, glm::vec3& point, glm::vec3& normal) { return false; }
    virtual ofColor getDiffuseColor(glm::vec3 intersection) { return diffuseColor; }
    virtual ofColor getSpecularColor(glm::vec3 intersection) { return specularColor; }

    // Variables
    //
    glm::vec3 position = glm::vec3(0, 0, 0);
    ofColor diffuseColor = ofColor::grey;
    ofColor specularColor = ofColor::lightGray;
    ofImage* diffuseTexture = nullptr;
    ofImage* specularTexture = nullptr;
    float reflectivity = 0.05f;
    
    bool isSelectable = true;
    bool isSelected = false;
    bool celShaded = false;
};

class Sphere : public SceneObject {
public:
    // Methods
    //
    Sphere(glm::vec3 position, float radius = 1.0f, ofColor diffuse = ofColor::lightGray, float reflectivity = 0.5f, bool celShaded = false);
    bool intersect(const Ray& ray, glm::vec3& point, glm::vec3& normal) {
        return (glm::intersectRaySphere(ray.position, ray.direction, position, radius, point, normal));
    }
    void draw() { ofDrawSphere(position, radius); }
    ofColor getDiffuseColor();

    // Variables
    //
    float radius;
};

class Triangle {
public:
    // Methods
    //
    Triangle(int v1, int v2, int v3) { this->v1 = v1; this->v2 = v2; this->v3 = v3; }
    
    // Variables
    //
    int v1, v2, v3;
};

class Mesh : public SceneObject {
public:
    // Methods
    //
    Mesh(glm::vec3 position, ofColor diffuse, string filePath);
    void draw();
    bool intersect(const Ray& ray, glm::vec3& point, glm::vec3& normal);
    void addVertice(glm::vec3 vertice) { vertices.push_back(vertice); }
    void addTriangle(int v1, int v2, int v3) { triangles.push_back(Triangle(v1, v2, v3)); }
    void parseFile(string filePath);
    
    // Variables
    //
    vector<glm::vec3> vertices;
    vector<Triangle> triangles;
};
class BaseLight : public SceneObject {
public:
    // Methods
    //
    virtual float getIntensity(const Ray* ray) { return 0.0f; }
    BaseLight(glm::vec3 position, ofColor diffuse);
    // Variables
    //
    float lightRadius = 0.1f;
    float intensity = 100.0f;
    ofLight previewLight;
};
class PointLight : public BaseLight {
public:
    // Methods
    //
    PointLight(glm::vec3 position, float intensity, ofColor diffuse = ofColor::white);
    float getIntensity(const Ray* ray) { return intensity; }
    bool intersect(const Ray& ray, glm::vec3& point, glm::vec3& normal) {
        return glm::intersectRaySphere(ray.position, ray.direction, position, lightRadius, point, normal);
    }
    void draw() { ofDrawSphere(position, lightRadius); }
};
class SpotLight;
class LightAnchor : public BaseLight {
public:
    // Methods
    //
    LightAnchor(glm::vec3 position);
    float getIntensity(const Ray* ray) { return intensity; }
    void draw() { ofDrawSphere(position, 0.1); }
    bool intersect(const Ray& ray, glm::vec3& point, glm::vec3& normal) {
        return glm::intersectRaySphere(ray.position, ray.direction, position, anchorRadius, point, normal);
    }
    
    // Variables
    //
    float anchorRadius = 0.1;
};
class SpotLight : public BaseLight {
    // Spotlight should probably not exist without an anchor reference. if we try deleting one, delete both.
    // To do... that.
public:
    // Methods
    //
    SpotLight(glm::vec3 position, float intensity, ofColor diffuse, float angle, LightAnchor* anchor);
    float getIntensity(const Ray* ray);
    void draw();
    bool intersect(const Ray& ray, glm::vec3& point, glm::vec3& normal) {
        return glm::intersectRaySphere(ray.position, ray.direction, position, lightRadius, point, normal);
    }
    void setAnchor(LightAnchor* a) { anchor = a; }
    
    // Variables
    //
    float angle = 30.0;
    LightAnchor* anchor;
};
//  General purpose plane
//
//  General purpose plane
class Plane : public SceneObject {
public:
    Plane(glm::vec3 position, glm::vec3 normal = glm::vec3(0, 1, 0), ofColor diffuse = ofColor::darkOliveGreen, float width = 20, float height = 20, ofImage* diffTex = nullptr, ofImage* specTex = nullptr, int tiles = 1);
    Plane();
    bool intersect(const Ray& ray, glm::vec3& point, glm::vec3& normal);
    ofColor mapPlaneToTexture(glm::vec3 intersection, ofImage* texture);
    ofColor getDiffuseColor(glm::vec3 intersection);
    ofColor getSpecularColor(glm::vec3 intersection);
    void draw();
    
    ofPlanePrimitive plane;
    glm::vec3 normal;
    float width;
    float height;
    int tiles;
};
// view plane for render camera
class  ViewPlane : public Plane {
public:
    ViewPlane(glm::vec2 p0, glm::vec2 p1) { min = p0; max = p1; }

    ViewPlane() {                         // create reasonable defaults (6x4 aspect)
        min = glm::vec2(-3, -2);
        max = glm::vec2(3, 2);
        position = glm::vec3(0, 0, 5);
        normal = glm::vec3(0, 0, 1);      // viewplane currently limited to Z axis orientation
    }

    void setSize(glm::vec2 min, glm::vec2 max) { this->min = min; this->max = max; }
    float getAspect() { return width() / height(); }

    glm::vec3 toWorld(float u, float v);   //   (u, v) --> (x, y, z) [ world space ]

    void draw() {
        ofPushStyle();
        ofNoFill();
        ofDrawRectangle(glm::vec3(min.x, min.y, position.z), width(), height());
        ofPopStyle();
    }


    float width() {
        return (max.x - min.x);
    }
    float height() {
        return (max.y - min.y);
    }

    // some convenience methods for returning the corners
    //
    glm::vec2 topLeft() { return glm::vec2(min.x, max.y); }
    glm::vec2 topRight() { return max; }
    glm::vec2 bottomLeft() { return min; }
    glm::vec2 bottomRight() { return glm::vec2(max.x, min.y); }

    //  To define an infinite plane, we just need a point and normal.
    //  The ViewPlane is a finite plane so we need to define the boundaries.
    //  We will define this in terms of min, max  in 2D.
    //  (in local 2D space of the plane)
    //  ultimately, will want to locate the ViewPlane with RenderCam anywhere
    //  in the scene, so it is easier to define the View rectangle in a local'
    //  coordinate system.
    //
    glm::vec2 min, max;
};

//  render camera  - currently must be z axis aligned (we will improve this in project 4)
class RenderCam : public SceneObject {
public:
    RenderCam();
    Ray getRay(float u, float v);
    void draw();
    void drawFrustum();

    glm::vec3 aim;
    ViewPlane view;          // The camera viewplane, this is the view that we will render
};

