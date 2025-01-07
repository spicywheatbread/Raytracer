#include "ofApp.h"

#define SHADOWOFFSET 50

// Implementation of vector reflection formula
glm::vec3 ofApp::reflectVector(glm::vec3 incomingDirection, glm::vec3 normal) {
    glm::vec3 projection = 2 * glm::dot(incomingDirection, normal) * normal;
    glm::vec3 reflection = incomingDirection - projection;

    return reflection;
}
void ofApp::clearSelectionList() {
    for (int i = 0; i < selected.size(); i++) 
    {
        selected[i]->isSelected = false;
    }
    selected.clear();
}
// Function used in raytrace() to find the closest object to a ray.
SceneObject* ofApp::shortestIntersection(const Ray& r, glm::vec3& point, glm::vec3& normal) {
    SceneObject* obj = nullptr;
    float shortest = std::numeric_limits<float>::max();
    for(int i = 0; i < scene.size(); i++) {
        // Point and normal of each intersect() call gets stored here
        glm::vec3 p;
        glm::vec3 n;
        if(scene[i]->intersect(r, p, n)) {
            // If we find an intersection, check to see if it is shorter than the last stored
            float dist = glm::distance(r.position, p);
            if(dist > 0.001f && dist < shortest) {
                // If it's shorter, then store the intersection point, object normal, and object reference
                point = p;
                normal = n;
                shortest = dist;
                obj = scene[i];
            }
        }
    }
    return obj;
}

// Helper function to scale a color without altering the alpha value
ofColor ofApp::scaleColor(ofColor color, float scale) {
    return ofColor(color.r * scale, color.g * scale, color.b * scale, color.a);
}
// Helper function to determine whether a ray will cause a shadow with a light
bool ofApp::isShadow(const Ray& shadowRay, BaseLight& light) {
    if(light.getIntensity(&shadowRay) == 0.0) { // This is mostly for spotlight, check if ray is within spotlight bound
        return true;
    }
    glm::vec3 intersectionPoint; // Placeholder variables to store function results
    glm::vec3 intersectionNormal;
    
    /*
     Two reasons for things to be in shadow:
     1. There's an object between the light and the intersection point
     2. We have a spotlight, and the angle between the light normal and the shadow ray is greater than the light angle
     */
    
    float distanceToLight = glm::abs(glm::distance(shadowRay.position, light.position));
    for(int i = 0; i < scene.size(); i++) {
        if(scene[i]->intersect(shadowRay, intersectionPoint, intersectionNormal)) { // If we find an intersection with the shadow ray
            if(glm::distance(intersectionPoint, shadowRay.position) < distanceToLight) { // And its between the obj and the light
                return true;
            }
        }
    }
    return false;
}

// Base function for raytracing
ofColor ofApp::shade(const Ray& incomingRay, BaseLight& light, int iterations) {
    // Start with a base color to be added onto with shading algorithm
    ofColor shadedColor = ofColor(0, 0, 0);
    if(iterations == 0) {
        return shadedColor;
    }
    // Check for intersection with this ray and any objects in the scene
    glm::vec3 intersectionPoint, intersectionNormal;
    SceneObject* intersectedObject = shortestIntersection(incomingRay, intersectionPoint, intersectionNormal);
    if(intersectedObject == nullptr) {
        return shadedColor;
    }
    // Create a ray originating from the intersection point (offset slightly for floating point error), pointing toward the light to detect shadows
    Ray* shadowRay = new Ray(intersectionPoint + intersectionNormal / SHADOWOFFSET, glm::normalize(light.position - intersectionPoint));
    if(isShadow(*shadowRay, light)) {
        delete shadowRay;
        return shadedColor;
    }
    delete shadowRay; // No shadow, calculate color value using specular and diffuse lighitng
    
    // this mess is because i added on cel shading at the end of my project lmao
    shadedColor += lambert(intersectionPoint, intersectionNormal, intersectedObject->getDiffuseColor(intersectionPoint), light, intersectedObject->celShaded);
    if(!intersectedObject->celShaded) {
        shadedColor += phong(incomingRay, intersectionPoint, intersectionNormal, intersectedObject->getSpecularColor(intersectionPoint), phongPower, light);
    }
    
    glm::vec3 reflection = reflectVector(incomingRay.direction, intersectionNormal);
    Ray* bounceRay = new Ray(intersectionPoint, reflection);
    shadedColor += shade(*bounceRay, light, iterations - 1) * intersectedObject->reflectivity;
    
    delete bounceRay;
    return shadedColor;
}
// Ambient Lighting, adds a baseline intensity to the color.
ofColor ofApp::ambient(const Ray& incomingRay) {
    ofColor diffuse;
    glm::vec3 intersectionPoint, intersectionNormal;
    SceneObject* intersectedObject = shortestIntersection(incomingRay, intersectionPoint, intersectionNormal);
    if(intersectedObject == nullptr) {
        diffuse = ofColor::lightGrey;
    } else {
        diffuse = intersectedObject->getDiffuseColor(intersectionPoint);
    }
    float intensity =  ambientLightSlider / 255;
    ofColor ambientColor = ofColor(diffuse.r * intensity, diffuse.g * intensity, diffuse.b * intensity, diffuse.r);
    
    return ambientColor;
}
// Diffuse lighting, adds lighting that is independent of the view direction.
ofColor ofApp::lambert(const glm::vec3& point, const glm::vec3& normal, const ofColor& diffuse, BaseLight& light, bool celShade) {
    ofColor lambertColor;
    
    // Find the cosine between the light and the normal
    float dot = glm::dot(glm::normalize(light.position - point), normal);
    float cos = std::max(0.0f, dot);
    
    Ray* lightRay = new Ray(light.position, glm::normalize(point - light.position));
    // Scale rbg components by these components
    float scale = diffuseCoefficient * cos * light.getIntensity(lightRay) / pow(glm::distance(point, light.position), 2);
    if(celShade) { // Try mapping to hard values? guessin here
        if(scale > 0.5) {
            scale = 1.0;
        } else {
            scale = 0.3;
        }
    }
    lambertColor = scaleColor(diffuse, scale);
    
    delete lightRay;
    return lambertColor;
}
// Specular lighting, adds highlights based on the view direction
ofColor ofApp::phong(const Ray& ray, const glm::vec3 &point, const glm::vec3 &normal, const ofColor specular, float power, BaseLight& light) {
    
    // Calculate bisector
    glm::vec3 added = glm::normalize(light.position - point) + glm::normalize(ray.position - point);
    glm::vec3 h = glm::normalize(added);
    
    // Calculate cosine between bisector and normal to determine strength of highlight
    float dot = glm::dot(h, normal);
    float cos = std::max(0.0f, dot);
    float distance = pow(glm::distance(point, light.position), 2); // Inverse power law or something
    Ray* lightRay = new Ray(light.position, glm::normalize(point - light.position));
    
    float scale = specularCoefficient * pow(cos, power) * light.getIntensity(lightRay) / distance;
    
    // Scale color by our phong highlights
    ofColor phongColor = scaleColor(specular, scale);
    delete lightRay;
    return phongColor;
}

// Draw an XYZ axis in RGB at world (0,0,0) for reference.
//
void ofApp::drawAxis(glm::vec3 position) {

    ofPushMatrix();
    ofTranslate(position);

    ofSetLineWidth(1.0);

    // X Axis
    ofSetColor(ofColor(255, 0, 0));
    ofDrawLine(ofPoint(0, 0, 0), ofPoint(1, 0, 0));


    // Y Axis
    ofSetColor(ofColor(0, 255, 0));
    ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 1, 0));

    // Z Axis
    ofSetColor(ofColor(0, 0, 255));
    ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 0, 1));

    ofPopMatrix();
}


void ofApp::drawGrid() {

    float u = 0;
    float v = 0;
    float pixelWidth = 1.0 / imageWidth;
    float pixelHeight = 1.0 / imageHeight;
    for (int x = 0; x < imageWidth; x++) {
        glm::vec3 p1 = renderCam.view.toWorld(u, 0);
        glm::vec3 p2 = renderCam.view.toWorld(u, 1);
        ofDrawLine(p1, p2);
        u += pixelWidth;
    }
    for (int y = 0; y < imageHeight; y++) {
        glm::vec3 p1 = renderCam.view.toWorld(0, v);
        glm::vec3 p2 = renderCam.view.toWorld(1, v);
        ofDrawLine(p1, p2);
        v += pixelHeight;
    }
}
bool ofApp::outlinePass(Ray& cameraRay) {
    Ray* outlineRay = new Ray(cameraRay.position, glm::normalize(renderCam.aim));
    glm::vec3 intersectionPoint, intersectionNormal;
    SceneObject* intersectedObject = shortestIntersection(cameraRay, intersectionPoint, intersectionNormal);
    if(intersectedObject == nullptr) {
        return false;
    }
    // just gonna hard code out planes.frick planes.
    Sphere* sphere = dynamic_cast<Sphere*>(intersectedObject);
    if(!sphere) return false;
    float angle = glm::abs(glm::dot(intersectionNormal, cameraRay.direction));
    if(angle < 0.30) {
        return true;
    }
    return false;
}
void ofApp::rayTrace(ofImage& img) {
    /* For each pixel of our view plane:
     1. Cast a ray from our camera to that pixel
     2. Check intersection with all objects
     3. Get object that has the shortest distance
     4. Shade pixel in image to that object's color
     */
    for(int u = 0; u < img.getWidth(); u++) {
        for(int v = 0; v < img.getHeight(); v++) {

            Ray cameraRay = renderCam.getRay(float(u + 0.5) / img.getWidth(), float(v + 0.5) / img.getHeight());  // getRay uses normalized coordinates, so we need to offset the pixel to the center as well as divide it by the image dimension

            if(outlinePass(cameraRay)) {
                img.setColor(u, img.getHeight() - 1 - v, ofColor::black);
            } else {
                ofColor totalColor = ambient(cameraRay);
                for(int i = 0; i < sceneLights.size(); i++) {
                    totalColor += shade(cameraRay, *sceneLights[i], lightBounces) ;
                }
                img.setColor(u, img.getHeight() - 1 - v, totalColor);
            }
        }
    }
    // Update img and save to disk
    img.update();
    img.save("render.jpg");
}
void ofApp::addPointLightButtonPressed() {
    sceneLights.push_back(new PointLight(glm::vec3(0, 5, 0), 100, ofColor::white));
}
void ofApp::addSphereButtonPressed() {
    scene.push_back(new Sphere(glm::vec3(0, 0, 0), 1, ofColor::white, false));
}
// For organization. Put renderParamGui setup stuff here
void ofApp::guiSetup() {
    renderParamGui.clear();
    renderParamGui.setup();
    renderParamGui.setPosition(ofGetWidth() - 200, 50);
    
    renderParamGuiLabel.set("Render Parameters");
    renderParamGui.add(renderParamGuiLabel);
    renderParamGui.add(diffuseCoefficientSlider.set("Diffuse Coefficient", 0.05, 0, 1.0));
    renderParamGui.add(specularCoefficientSlider.set("Specular Coefficient", 0.05, 0, 1.0));
    renderParamGui.add(ambientLightSlider.set("Ambient Light", 80, 0, 255));
    renderParamGui.add(phongPowerSlider.set("Phong Exponent", 20, 1, 64));
    renderParamGui.add(lightBounceSlider.set("Light Bounces", 2, 1, 5));
    
    objectGui.clear();
    objectGui.setup();
    objectGui.setPosition(0, 50);
    
    addPointLightButton.addListener(this, &ofApp::addPointLightButtonPressed);
    addSphereButton.addListener(this, &ofApp::addSphereButtonPressed);
    objectGui.add(objectGuiLabel.set("Selected Object"));
    objectGui.add(addPointLightButton.setup("Add Point Light"));
    objectGui.add(addSphereButton.setup("Add Sphere"));
    objectGui.add(objectPositionSlider.set("World position", glm::vec3(0, 0, 0), glm::vec3(-50, -50, -50), glm::vec3(50, 50, 50)));
    objectGui.add(objectDiffuseColor.set("Diffuse color", ofColor(255, 255, 255), ofColor(0, 0, 0), ofColor(255, 255, 255)));
    objectGui.add(objectSpecularColor.set("Specular color", ofColor(255, 255, 255)));
}
void ofApp::cameraSetup() {
    // Initialize cameras
    mainCam.setDistance(15);
    mainCam.setNearClip(.1);
    mainCam.disableMouseInput();
    
    sideCam.setPosition(25, 0, 0);
    sideCam.lookAt(glm::vec3(0, 0, 0));
    
    topCam.setPosition(0, 25, 0);
    topCam.lookAt(glm::vec3(0, -1, 0));

    previewCam.setPosition(renderCam.position);
    previewCam.setNearClip(.1);
    previewCam.lookAt(glm::vec3(0, 0, 0));
    
    theCam = &mainCam;
}
void ofApp::objectSetup() {
    // Initialize objects in the scene
    // Mesh scene.push_back(new Mesh(glm::vec3(4, -1, -5), ofColor::gray, "polygon.obj"));
    scene.push_back(new Sphere(glm::vec3(2, 1, -8), 2, ofColor(168, 220, 255), 0.2f, true));
    scene.push_back(new Sphere(glm::vec3(-2, 0, -8), 1.5, ofColor(168, 220, 205), 0.2f, true));
    scene.push_back(new Sphere(glm::vec3(-1, 0, -8), 1, ofColor::grey, 0.5f));

    // Planes
    textures.push_back(new ofImage("woodfloor/woodfloor.jpg"));
    textures.push_back(new ofImage("woodfloor/woodfloor_spec.jpg"));
    
    textures.push_back(new ofImage("floral/floral.jpg"));
    textures.push_back(new ofImage("floral/floral_spec.jpg"));

    scene.push_back(new Plane(glm::vec3(0, -2, 0), glm::vec3(0, 1, 0), // Floor
                              ofColor::brown, 50, 50, textures[0], textures[1], 4));
    scene.push_back(new Plane(glm::vec3(0, 0, -20), glm::vec3(0, 0, 1),
                              ofColor::gold, 50, 50, textures[2], textures[3], 1)); // Back

    // Lights
    sceneLights.push_back(new PointLight(glm::vec3(1, 8, 0), 400, ofColor::white));
    sceneLights.push_back(new PointLight(glm::vec3(-10, 2, 0), 500, ofColor::white));
    sceneLights.push_back(new PointLight(glm::vec3(3, 2, -3), 200, ofColor::white));
    

}

//--------------------------------------------------------------
void ofApp::setup(){

    ofSetBackgroundColor(40, 40, 40);
    
    guiSetup();
    cameraSetup();
    objectSetup();
    
    // Allocate space for image and set initial color
    image.allocate(imageWidth, imageHeight, OF_IMAGE_COLOR);
    image.setColor(ofColor::black);
    image.update();

}
void ofApp::updateParameters() {
    diffuseCoefficient = diffuseCoefficientSlider;
    specularCoefficient = specularCoefficientSlider;
    ambientLight = ambientLightSlider;
    phongPower = phongPowerSlider;
    lightBounces = lightBounceSlider;
}
//--------------------------------------------------------------
void ofApp::update(){
    updateParameters();
    if(objSelected()) {
        if(bDrag) {
            objectPositionSlider.set(selected[0]->position);
        } else {
            selected[0]->position = objectPositionSlider.get();
        }
        selected[0]->diffuseColor = objectDiffuseColor.get();
        selected[0]->specularColor = objectSpecularColor.get();
    } else {
        objectPositionSlider.set("World Position", glm::vec3(0, 0, 0), glm::vec3(-10, -10, -10), glm::vec3(10, 10, 10));
        objectDiffuseColor = ofColor::white;
        objectSpecularColor = ofColor::white;
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofEnableDepthTest();
    theCam->begin();
    ofEnableLighting();

    drawAxis(glm::vec3(0, 0, 0));;

    // Draw objs in the scene
    for (int i = 0; i < scene.size(); i++) {
        if(scene[i]->isSelected) {
            ofSetColor(ofColor::lightGray);
        } else {
            ofSetColor(scene[i]->diffuseColor);
        }
        scene[i]->draw();
    }

    // Draw representations of camera and viewplane
    ofSetColor(ofColor::lightSkyBlue);
    renderCam.drawFrustum();
    renderCam.view.draw();
    
    ofSetColor(ofColor::blue);
    renderCam.draw();
    
    ofDisableLighting();
    if(bShowImage) {
        ofSetColor(ofColor::white);
        image.draw(glm::vec3(-3, -2, 5), 6, 4);
    }
    // Draw lights in the scene
    for(int i = 0; i < sceneLights.size(); i++) {
        if(sceneLights[i]->isSelected) {
            ofSetColor(ofColor::lightGray);
        } else {
            ofSetColor(sceneLights[i]->diffuseColor);
        }        sceneLights[i]->draw();
    }
    theCam->end();
    ofDisableDepthTest();
    renderParamGui.draw();
    objectGui.draw();

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    switch(key) {
        case 'c':
            clearSelectionList();
            break;
        case OF_KEY_SHIFT:
            bSftKeyDown = true;
            if (!mainCam.getMouseInputEnabled()) mainCam.enableMouseInput();
            break;
        case OF_KEY_BACKSPACE:
            if (objSelected()) {
                for(int i = 0; i < selected.size(); i++) {
                    removeObject(selected[i]);
                }
                clearSelectionList();
            }
            break;
        default:
            break;
    }
}


void ofApp::keyReleased(int key) {
    switch (key) {
    case OF_KEY_SHIFT:
        bSftKeyDown = false;
        mainCam.disableMouseInput();
        break;
    case 'b':
        bShowImage = !bShowImage;
        break;
    case 'f':
        ofToggleFullscreen();
        break;
    case 'h':
        bHide = !bHide;
        break;
    case 's':
        image.save("render.jpg");
        break;
    case 'n':
        scene.push_back(new Sphere(glm::vec3(0, 0, 0), 1.0, ofColor::violet));
        break;
    case 'r':
        rayTrace(image);
        cout << "done..." << endl;
        break;
    case OF_KEY_F1:
        theCam = &mainCam;
        break;
    case OF_KEY_F2:
        theCam = &sideCam;
        break;
    case OF_KEY_F3:
        theCam = &previewCam;
        break;
    case OF_KEY_F4:
        theCam = &topCam;
        break;
    default:
        break;
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
    if (bDrag && objSelected()) {
        glm::vec3 point;
        mouseToDragPlane(x, y, point);
        
        selected[0]->position += (point - lastPoint);

        lastPoint = point;
    }

}
bool ofApp::mouseToDragPlane(int x, int y, glm::vec3& point) {
    glm::vec3 p = theCam->screenToWorld(glm::vec3(x, y, 0));
    glm::vec3 d = p - theCam->getPosition();
    glm::vec3 dn = glm::normalize(d);

    float dist;
    glm::vec3 pos;
    if(objSelected()) {
        pos = selected[0]->position;
    } else {
        pos = glm::vec3(0, 0, 0);
    }
    
    if (glm::intersectRayPlane(p, dn, pos, glm::normalize(theCam->getZAxis()), dist)) {
        point = p + dn * dist;
        return true;
    }
    return false;
}
//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    
    if (mainCam.getMouseInputEnabled()) return;
    
    glm::vec3 p = theCam->screenToWorld(glm::vec3(x, y, 0));
    glm::vec3 d = p - theCam->getPosition();
    glm::vec3 dn = glm::normalize(d);

    // check for selection of scene objects
    SceneObject* selectedObj = NULL;
    float nearestDist = std::numeric_limits<float>::infinity();

    // Iterate through scene objects
    for (int i = 0; i < scene.size(); i++) {
        glm::vec3 point, norm;

        if (scene[i]->isSelectable && scene[i]->intersect(Ray(p, dn), point, norm)) {
            float dist = glm::length(point - theCam->getPosition());
            if (dist < nearestDist && dist > .001) {
                nearestDist = dist;
                selectedObj = scene[i];
            }
        }
    }
    // Iterate through lights (for selecting spot lights)
    for (int i = 0; i < sceneLights.size(); i++) {
        glm::vec3 point, norm;

        if (sceneLights[i]->isSelectable && sceneLights[i]->intersect(Ray(p, dn), point, norm)) {
            float dist = glm::length(point - theCam->getPosition());
            if (dist < nearestDist && dist > .001) {
                nearestDist = dist;
                selectedObj = sceneLights[i];
            }
        }
    }
    
    if (selectedObj) {
        // intersection
        bDrag = true;
        
        selected.clear(); // For now im not gonna allow selecting multiple objects.
        selectedObj->isSelected = true;
        selected.push_back(selectedObj);
        
        mouseToDragPlane(x, y, lastPoint);
        
        objectPositionSlider.set("World Position", selected[0]->position, selected[0]->position - glm::vec3(10, 10, 10), selected[0]->position + glm::vec3(10, 10, 10));
        objectDiffuseColor = selected[0]->diffuseColor;
        objectSpecularColor = selected[0]->specularColor;
    } else {
        // no intersection
        clearSelectionList();
    }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    bDrag = false;
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}
void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY) {
    if(selected.size() > 0) {
        SpotLight* a = dynamic_cast<SpotLight*>(selected[0]);
        if(a) {
            if(scrollY > 0) {
                a->angle += (PI / 100);
            } else if(scrollY < 0) {
                a->angle -= (PI / 100);
            }
        }
    }
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}
void ofApp::removeObject(SceneObject* obj) {
    // remove from scene list;
    for (int i = 0; i < scene.size(); i++) {
        if (scene[i] == obj) {
            scene.erase(scene.begin() + i);
            break;
        }
    }
    for (int i = 0; i < sceneLights.size(); i++) {
        if (sceneLights[i] == obj) {
            sceneLights.erase(sceneLights.begin() + i);
            break;
        }
    }
    // delete object;  sceneObject destructor will handle cleaning up parent and child lists.
    delete obj;
}
//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
ofApp::~ofApp() {
    cout << "Destructor called" << endl;
    for(auto obj : scene) {
        delete obj;
    }
    scene.clear();
    for(auto light : sceneLights) {
        delete light;
    }
    for(auto tex : textures) {
        delete tex;
    }
    textures.clear();
    sceneLights.clear();
    image.clear();
}

