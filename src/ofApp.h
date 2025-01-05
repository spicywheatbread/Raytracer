#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "Primitives.h"


class ofApp : public ofBaseApp {

    public:
        void setup();
        void update();
        void draw();

        void keyPressed(int key);
        void keyReleased(int key);
        void mouseMoved(int x, int y );
        void mouseDragged(int x, int y, int button);
        void mousePressed(int x, int y, int button);
        void mouseReleased(int x, int y, int button);
        void mouseEntered(int x, int y);
        void mouseExited(int x, int y);
        void mouseScrolled(int x, int y, float scrollX, float scrollY);
        void windowResized(int w, int h);
        void dragEvent(ofDragInfo dragInfo);
        void gotMessage(ofMessage msg);
    
        void drawGrid();
        void drawAxis(glm::vec3 position);
    
        void guiSetup();
        void cameraSetup();
        void objectSetup();
        void lightSetup();
    
        void updateParameters();
        void removeObject(SceneObject* obj);
        bool mouseToDragPlane(int x, int y, glm::vec3& point);
        bool objSelected() { return !selected.empty(); };
        void clearSelectionList();
        ofColor scaleColor(ofColor color, float scale);
        SceneObject* shortestIntersection(const Ray& r, glm::vec3& point, glm::vec3& normal);
        glm::vec3 reflectVector(const Ray& inRay, glm::vec3 normal);
        bool isShadow(const Ray& shadowRay, BaseLight& light);

    
        // Raytracing functions
        ofColor shade(const Ray &incomingRay, BaseLight& light, int iterations);
        ofColor ambient(const Ray& incomingRay);
        ofColor lambert(const glm::vec3& point, const glm::vec3& normal, const ofColor& diffuse, BaseLight& light, bool celShade);
        ofColor phong(const Ray& ray, const glm::vec3 &point, const glm::vec3 &normal, const ofColor diffuse, float power, BaseLight& light);
        ~ofApp();
        bool shadowPass(Ray& cameraRay);
        void rayTracePixel(ofImage& img, const int u, const int v);
        void rayTrace(ofImage& img);
    
        void addPointLightButtonPressed();
        void addSphereButtonPressed();
    
        bool bHide = true;
        bool bShowImage = true;

        // Cameras
        ofEasyCam  mainCam;
        ofCamera topCam;
        ofCamera sideCam;
        ofCamera previewCam;
        ofCamera* theCam;    // set to current camera either mainCam or sideCam
        RenderCam renderCam;
    
        ofLight ofPointLight;
    
        // UI elements
        ofxPanel renderParamGui;
        ofParameter<std::string> renderParamGuiLabel;
        ofParameter<float> diffuseCoefficientSlider;
        ofParameter<float> specularCoefficientSlider;
        ofParameter<float> ambientLightSlider;
        ofParameter<int> phongPowerSlider;
        ofParameter<int> lightBounceSlider;

        ofxPanel objectGui;
        ofParameter<std::string> objectGuiLabel;
        ofParameter<glm::vec3> objectPositionSlider;
        ofParameter<ofColor> objectDiffuseColor;
        ofParameter<ofColor> objectSpecularColor;
    
        ofxPanel addGui;
        ofxButton addPointLightButton;
        ofxButton addSphereButton;
        // Vectors that hold objects and lights
        vector<SceneObject*> scene;
        vector<BaseLight*> sceneLights;
        vector<ofImage*> textures;
    
        vector<SceneObject*> selected;
    
        // Variables I guess!
        glm::vec3 lastPoint;
        glm::vec3 dragPlane;

        // Image parameters
        ofImage image;
        int imageWidth = 2400;
        int imageHeight = 1600;
    
        bool bDrag = false;
        bool bSftKeyDown = false;
    
        float specularCoefficient = 0.0;
        float diffuseCoefficient = 0.0;
        float ambientLight;
        float phongPower = 1;
        int lightBounces = 2;
};
