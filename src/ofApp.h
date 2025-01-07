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
    
        // Initialization functions
        void guiSetup();
        void cameraSetup();
        void objectSetup();
        void lightSetup();
        
        // Object container functions
        void removeObject(SceneObject* obj);
        bool objSelected() { return !selected.empty(); };
        void clearSelectionList();
    
        // Helper functions
        bool mouseToDragPlane(int x, int y, glm::vec3& point);
        ofColor scaleColor(ofColor color, float scale);
        SceneObject* shortestIntersection(const Ray& r, glm::vec3& point, glm::vec3& normal);
        glm::vec3 reflectVector(glm::vec3 incomingDirection, glm::vec3 normal);
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
    
        // GUI functions
        void addPointLightButtonPressed();
        void addSphereButtonPressed();
        void updateParameters();
    
        // Bools for showing bojects
        bool bHide = true;
        bool bShowImage = true;

        // Cameras
        ofEasyCam  mainCam;
        ofCamera topCam;
        ofCamera sideCam;
        ofCamera previewCam;
        ofCamera* theCam;    // set to current camera either mainCam or sideCam
        RenderCam renderCam;
    
        // Pointlight for scene lighting preview
        ofLight ofPointLight;
    
        // GUI panel for rendering parameters
        ofxPanel renderParamGui;
        ofParameter<std::string> renderParamGuiLabel;
        ofParameter<float> diffuseCoefficientSlider;
        ofParameter<float> specularCoefficientSlider;
        ofParameter<float> ambientLightSlider;
        ofParameter<int> phongPowerSlider;
        ofParameter<int> lightBounceSlider;

        // GUI panel for information about an object
        ofxPanel objectGui;
        ofParameter<std::string> objectGuiLabel;
        ofParameter<glm::vec3> objectPositionSlider;
        ofParameter<ofColor> objectDiffuseColor;
        ofParameter<ofColor> objectSpecularColor;
    
        // GUI Panel for adding objects
        ofxPanel addGui;
        ofxButton addPointLightButton;
        ofxButton addSphereButton;
    
        // Vectors that hold objects and lights
        vector<SceneObject*> scene;
        vector<BaseLight*> sceneLights;
        vector<ofImage*> textures;
        vector<SceneObject*> selected;
    
        // Placeholder variables for dragging functions
        glm::vec3 lastPoint;
        glm::vec3 dragPlane;

        // Image parameters
        ofImage image;
        int imageWidth = 2400;
        int imageHeight = 1600;
    
        bool bDrag = false;
        bool bSftKeyDown = false;
    
        // Rendering parameters
        float specularCoefficient;
        float diffuseCoefficient;
        float ambientLight;
        float phongPower;
        int lightBounces;
};
