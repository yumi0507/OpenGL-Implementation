#include "headers.h"
#include "trianglemesh.h"
#include "camera.h"
#include "shaderprog.h"
#include "light.h"
#include "imagetexture.h"
#include "skybox.h"


// Global variables.
int screenWidth = 600;
int screenHeight = 600;
// Triangle mesh.
TriangleMesh* mesh = nullptr;
// Lights.
DirectionalLight* dirLight = nullptr;
PointLight* pointLight = nullptr;
SpotLight* spotLight = nullptr;
glm::vec3 dirLightDirection = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 dirLightRadiance = glm::vec3(0.6f, 0.6f, 0.6f);
glm::vec3 pointLightPosition = glm::vec3(0.8f, 0.0f, 0.8f);
glm::vec3 pointLightIntensity = glm::vec3(0.1f, 0.1f, 0.1f);
glm::vec3 spotLightPosition = glm::vec3(0.0f, 1.0f, 1.0f);
glm::vec3 spotLightDirection = glm::vec3(0.0f, -1.0f, -1.0f);
glm::vec3 spotLightIntensity = glm::vec3(0.3f, 0.3f, 0.3f);
float spotLightCutoffStartInDegree = 30.0f;
float spotLightTotalWidthInDegree = 45.0f;
glm::vec3 ambientLight = glm::vec3(0.2f, 0.2f, 0.2f);
// Camera.
Camera* camera = nullptr;
glm::vec3 cameraPos = glm::vec3(0.0f, 1.0f, 5.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float fovy = 30.0f;
float zNear = 0.1f;
float zFar = 1000.0f;
// Shader.
FillColorShaderProg* fillColorShader = nullptr;
PhongShadingDemoShaderProg* phongShadingShader = nullptr;
SkyboxShaderProg* skyboxShader = nullptr;
// UI.
const float lightMoveSpeed = 0.2f;
// Skybox.
Skybox* skybox = nullptr;
// Rotate.
bool rotSkybox = false;
bool rotModel = false;

// SceneObject.
struct SceneObject
{
    SceneObject() {
        mesh = nullptr;
        worldMatrix = glm::mat4x4(1.0f);
    }
    TriangleMesh* mesh;
    glm::mat4x4 worldMatrix;
};
SceneObject sceneObj;

// ScenePointLight (for visualization of a point light).
struct ScenePointLight
{
    ScenePointLight() {
        light = nullptr;
        worldMatrix = glm::mat4x4(1.0f);
        visColor = glm::vec3(1.0f, 1.0f, 1.0f);
    }
    PointLight* light;
    glm::mat4x4 worldMatrix;
    glm::vec3 visColor;
};
ScenePointLight pointLightObj;
ScenePointLight spotLightObj;

// Function prototypes.
void ReleaseResources();
// Callback functions.
void RenderSceneCB();
void ReshapeCB(int, int);
void ProcessSpecialKeysCB(int, int, int);
void ProcessKeysCB(unsigned char, int, int);
void SetupRenderState();
void LoadObjects(const std::string&);
void CreateCamera();
void CreateSkybox(const std::string);
void CreateShaderLib();



void ReleaseResources()
{
    // Delete scene objects and lights.
    if (mesh != nullptr) {
        delete mesh;
        mesh = nullptr;
    }
    if (pointLight != nullptr) {
        delete pointLight;
        pointLight = nullptr;
    }
    if (dirLight != nullptr) {
        delete dirLight;
        dirLight = nullptr;
    }
    if (spotLight != nullptr) {
        delete spotLight;
        spotLight = nullptr;
    }
    // Delete camera.
    if (camera != nullptr) {
        delete camera;
        camera = nullptr;
    }
    // Delete shaders.
    if (fillColorShader != nullptr) {
        delete fillColorShader;
        fillColorShader = nullptr;
    }
    if (phongShadingShader != nullptr) {
        delete phongShadingShader;
        phongShadingShader = nullptr;
    }
    if (skyboxShader != nullptr) {
        delete skyboxShader;
        skyboxShader = nullptr;
    }
}

static float curObjRotationY = 0.0f;
static float curSkyboxRotationY = 249.0f;
const float rotStep = 0.005f;
bool objClockwise = true;
bool skyboxClockwise = true;
void RenderSceneCB()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    TriangleMesh* pMesh = sceneObj.mesh;
    if (pMesh != nullptr) {
        // Update transform.
        if (rotModel) {
            if (objClockwise){                
                 curObjRotationY += rotStep;
            }
            else{
                curObjRotationY -= rotStep;
            }
        }
        glm::mat4x4 S = glm::scale(glm::mat4x4(1.0f), glm::vec3(1.5f, 1.5f, 1.5f));
        glm::mat4x4 R = glm::rotate(glm::mat4x4(1.0f), glm::radians(curObjRotationY), glm::vec3(0, 1, 0));
        sceneObj.worldMatrix = S * R;
        // -------------------------------------------------------
		// Note: if you want to compute lighting in the View Space, 
        //       you might need to change the code below.
		// -------------------------------------------------------
        glm::mat4x4 normalMatrix = glm::transpose(glm::inverse(camera->GetViewMatrix() * sceneObj.worldMatrix));
        glm::mat4x4 MVP = camera->GetProjMatrix() * camera->GetViewMatrix() * sceneObj.worldMatrix;
        
        // -------------------------------------------------------
        phongShadingShader->Bind();
        // Transformation matrix.
        glUniformMatrix4fv(phongShadingShader->GetLocM(), 1, GL_FALSE, glm::value_ptr(sceneObj.worldMatrix));
        glUniformMatrix4fv(phongShadingShader->GetLocNM(), 1, GL_FALSE, glm::value_ptr(normalMatrix));
        glUniformMatrix4fv(phongShadingShader->GetLocMVP(), 1, GL_FALSE, glm::value_ptr(MVP));
        glUniform3fv(phongShadingShader->GetLocCameraPos(), 1, glm::value_ptr(camera->GetCameraPos()));
        for (const auto& subMesh : mesh->GetSubMeshes()) {
            // Material properties.
            glUniform3fv(phongShadingShader->GetLocKa(), 1, glm::value_ptr(subMesh.material->GetKa()));
            glUniform3fv(phongShadingShader->GetLocKd(), 1, glm::value_ptr(subMesh.material->GetKd()));
            glUniform3fv(phongShadingShader->GetLocKs(), 1, glm::value_ptr(subMesh.material->GetKs()));
            glUniform1f(phongShadingShader->GetLocNs(), subMesh.material->GetNs());
            // Light data.
            if (dirLight != nullptr) {
                glUniform3fv(phongShadingShader->GetLocDirLightDir(), 1, glm::value_ptr(dirLight->GetDirection()));
                glUniform3fv(phongShadingShader->GetLocDirLightRadiance(), 1, glm::value_ptr(dirLight->GetRadiance()));
            }
            if (pointLight != nullptr) {
                glUniform3fv(phongShadingShader->GetLocPointLightPos(), 1, glm::value_ptr(pointLight->GetPosition()));
                glUniform3fv(phongShadingShader->GetLocPointLightIntensity(), 1, glm::value_ptr(pointLight->GetIntensity()));
            }
            if (spotLight != nullptr) {
                glUniform3fv(phongShadingShader->GetLocSpotLightPos(), 1, glm::value_ptr(spotLight->GetPosition()));
                glUniform3fv(phongShadingShader->GetLocSpotLightDir(), 1, glm::value_ptr(spotLight->GetDirection()));
                glUniform3fv(phongShadingShader->GetLocSpotLightIntensity(), 1, glm::value_ptr(spotLight->GetIntensity()));
                glUniform1f(phongShadingShader->GetLocSpotLightTotalWidth(), spotLight->GetTotalWidth());
                glUniform1f(phongShadingShader->GetLocSpotLightCutoffStart(), spotLight->GetCutoffStart());
            }
            glUniform3fv(phongShadingShader->GetLocAmbientLight(), 1, glm::value_ptr(ambientLight));
            if (subMesh.material->GetMapKd() != nullptr) {
                subMesh.material->GetMapKd()->Bind(GL_TEXTURE0);
                glUniform1i(phongShadingShader->GetLocMapKd(), 0);
                glUniform1i(phongShadingShader->GetLocExist(), 1);
            }
            else {
                glUniform1i(phongShadingShader->GetLocExist(), 0);
            }

            mesh->RenderSubMesh(subMesh);
        }
        phongShadingShader->UnBind();
        // -------------------------------------------------------
    }
    // -------------------------------------------------------------------------------------------

    // Visualize the light with fill color. ------------------------------------------------------
    PointLight* pointLight = pointLightObj.light;
    if (pointLight != nullptr) {
        glm::mat4x4 T = glm::translate(glm::mat4x4(1.0f), pointLight->GetPosition());
        pointLightObj.worldMatrix = T;
        glm::mat4x4 MVP = camera->GetProjMatrix() * camera->GetViewMatrix() * pointLightObj.worldMatrix;
        fillColorShader->Bind();
        glUniformMatrix4fv(fillColorShader->GetLocMVP(), 1, GL_FALSE, glm::value_ptr(MVP));
        glUniform3fv(fillColorShader->GetLocFillColor(), 1, glm::value_ptr(pointLightObj.visColor));
        // Render the point light.
        pointLight->Draw();
        fillColorShader->UnBind();
    }
    SpotLight* spotLight = (SpotLight*)(spotLightObj.light);
    if (spotLight != nullptr) {
        glm::mat4x4 T = glm::translate(glm::mat4x4(1.0f), spotLight->GetPosition());
        spotLightObj.worldMatrix = T;
        glm::mat4x4 MVP = camera->GetProjMatrix() * camera->GetViewMatrix() * spotLightObj.worldMatrix;
        fillColorShader->Bind();
        glUniformMatrix4fv(fillColorShader->GetLocMVP(), 1, GL_FALSE, glm::value_ptr(MVP));
        glUniform3fv(fillColorShader->GetLocFillColor(), 1, glm::value_ptr(spotLightObj.visColor));
        // Render the spot light.
        spotLight->Draw();
        fillColorShader->UnBind();
    }
    // -------------------------------------------------------------------------------------------

    // Render skybox. ----------------------------------------------------------------------------
    if (skybox != nullptr) {
        if (rotSkybox) {
            if (skyboxClockwise)
                curSkyboxRotationY += rotStep;
            else
                curSkyboxRotationY -= rotStep;
        }
        skybox->SetRotationX(-3.0f);
        skybox->SetRotationY(curSkyboxRotationY);
        skybox->Render(camera, skyboxShader);
    }
    // -------------------------------------------------------------------------------------------

    glutSwapBuffers();
}

void ReshapeCB(int w, int h)
{
    // Update viewport.
    screenWidth = w;
    screenHeight = h;
    glViewport(0, 0, screenWidth, screenHeight);
    // Adjust camera and projection.
    float aspectRatio = (float)screenWidth / (float)screenHeight;
    camera->UpdateProjection(fovy, aspectRatio, zNear, zFar);
}

void ProcessSpecialKeysCB(int key, int x, int y)
{
    // Handle special (functional) keyboard inputs such as F1, spacebar, page up, etc. 
    switch (key) {
    // Rendering mode.
    case GLUT_KEY_F1:
        // Render with point mode.
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        break;
    case GLUT_KEY_F2:
        // Render with line mode.
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;
    case GLUT_KEY_F3:
        // Render with fill mode.
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    
    // Light control.
    case GLUT_KEY_LEFT:
        if (pointLight != nullptr)
            pointLight->MoveLeft(lightMoveSpeed);
        break;
    case GLUT_KEY_RIGHT:
        if (pointLight != nullptr)
            pointLight->MoveRight(lightMoveSpeed);
        break;
    case GLUT_KEY_UP:
        if (pointLight != nullptr)
            pointLight->MoveUp(lightMoveSpeed);
        break;
    case GLUT_KEY_DOWN:
        if (pointLight != nullptr)
            pointLight->MoveDown(lightMoveSpeed);
        break;
    default:
        break;
    }
}

void ProcessKeysCB(unsigned char key, int x, int y)
{
    // Handle other keyboard inputs those are not defined as special keys.
    int mode = glutGetModifiers();
    if (key == 27) {
        // Release memory allocation if needed.
        ReleaseResources();
        exit(0);
    }

    // Rotation control. 
    // Set the typing language into English(US) first or it won't get the shift/atl key.
    if(key == 13) {
        if (mode & GLUT_ACTIVE_ALT) {
            if (rotSkybox) {
                skyboxClockwise = !skyboxClockwise;
                std::cout << "------------------------------" << std::endl;
                std::cout << "Skybox Rotation Dir.: ";
                if (skyboxClockwise)    std::cout << "Clockwise." << std::endl;
                else                    std::cout << "Counterclockwise." << std::endl;
                std::cout << "------------------------------" << std::endl;
            }
        }
        else if (rotModel){
            objClockwise = !objClockwise;
            std::cout << "------------------------------" << std::endl;
            std::cout << "Model Rotation Dir.: ";
            if (objClockwise)   std::cout << "Clockwise." << std::endl;
            else                std::cout << "Counterclockwise." << std::endl;
            std::cout << "------------------------------" << std::endl;
        }
    }
    if (key == 32) {
        if (mode & GLUT_ACTIVE_ALT) {
            rotSkybox = !rotSkybox;
            std::cout << "------------------------------" << std::endl;
            std::cout << "Skybox Rotation: ";
            if (rotSkybox)  std::cout << "Rotate." << std::endl;
            else            std::cout << "Pause." << std::endl;
            std::cout << "------------------------------" << std::endl;
        }
        else {
            rotModel = !rotModel;
            std::cout << "------------------------------" << std::endl;
            std::cout << "Model Rotation: ";
            if (rotModel)   std::cout << "Rotate." << std::endl;
            else            std::cout << "Pause." << std::endl;
            std::cout << "------------------------------" << std::endl;
        }
    }
    

    // Spot light control.
    if (spotLight != nullptr) {
        if (key == 'a')
            spotLight->MoveLeft(lightMoveSpeed);
        if (key == 'd')
            spotLight->MoveRight(lightMoveSpeed);
        if (key == 'w')
            spotLight->MoveUp(lightMoveSpeed);
        if (key == 's')
            spotLight->MoveDown(lightMoveSpeed);
    }
}

void SetupRenderState()
{
    glEnable(GL_DEPTH_TEST);

    glm::vec4 clearColor = glm::vec4(0.44f, 0.57f, 0.75f, 1.00f);
    glClearColor(
        (GLclampf)(clearColor.r), 
        (GLclampf)(clearColor.g), 
        (GLclampf)(clearColor.b), 
        (GLclampf)(clearColor.a)
    );
}

void LoadObjects(const std::string& modelPath)
{
    // -------------------------------------------------------
	// Note: you can change the code below if you want to load
    //       the model dynamically.
	// -------------------------------------------------------

    mesh = new TriangleMesh();
    mesh->LoadFromFile(modelPath, true);
    mesh->ShowInfo();
    mesh->CreateBuffers();
    sceneObj.mesh = mesh;    
}

void CreateLights()
{
    // Create a directional light.
    dirLight = new DirectionalLight(dirLightDirection, dirLightRadiance);
    /*    
    // Create a point light.
    pointLight = new PointLight(pointLightPosition, pointLightIntensity);
    pointLightObj.light = pointLight;
    pointLightObj.visColor = glm::normalize((pointLightObj.light)->GetIntensity());
    */
    // Create a spot light.
    spotLight = new SpotLight(spotLightPosition, spotLightIntensity, spotLightDirection, 
            spotLightCutoffStartInDegree, spotLightTotalWidthInDegree);
    spotLightObj.light = spotLight;
    spotLightObj.visColor = glm::normalize((spotLightObj.light)->GetIntensity());
}

void CreateCamera()
{
    // Create a camera and update view and proj matrices.
    camera = new Camera((float)screenWidth / (float)screenHeight);
    camera->UpdateView(cameraPos, cameraTarget, cameraUp);
    float aspectRatio = (float)screenWidth / (float)screenHeight;
    camera->UpdateProjection(fovy, aspectRatio, zNear, zFar);
}

void CreateSkybox(const std::string skyboxPath)
{
    // -------------------------------------------------------
	// Note: you can change the code below if you want to change
    //       the skybox texture dynamically.
	// -------------------------------------------------------
    if (skybox != nullptr)
        skybox->~Skybox();

    std::cout << "------------------------------" << std::endl;
    std::cout << "Skybox backgroud: " << skyboxPath << "." << std::endl;
    std::cout << "------------------------------" << std::endl;
    std::string texFilePath = "./TestTextures_HW3/" + skyboxPath;
    const int numSlices = 36;
    const int numStacks = 18;
    const float radius = 50.0f;
    skybox = new Skybox(texFilePath, numSlices, numStacks, radius);
}

void CreateShaderLib()
{
    fillColorShader = new FillColorShaderProg();
    if (!fillColorShader->LoadFromFiles("shaders/fixed_color.vs", "shaders/fixed_color.fs"))
        exit(1);

    phongShadingShader = new PhongShadingDemoShaderProg();
    if (!phongShadingShader->LoadFromFiles("shaders/phong_shading_demo.vs", "shaders/phong_shading_demo.fs"))
        exit(1);

    skyboxShader = new SkyboxShaderProg();
    if (!skyboxShader->LoadFromFiles("shaders/skybox.vs", "shaders/skybox.fs"))
        exit(1);
}

// Menu events.
void processModelMenuEvents(int option) {
    std::string model;
    switch (option) {
    case 1:
        model = "Arcanine";
        break;
    case 2:
        model = "Ferrari";
        break;
    case 3:
        model = "Gengar";
        break;
    case 4:
        model = "Ivysaur";
        break;
    case 5:
        model = "Koffing";
        break;
    case 6:
        model = "MagikarpF";
        break;
    case 7:
        model = "Rose";
        break;
    case 8:
        model = "Slowbro";
        break;
    case 9:
        model = "TexCube";
        break;
    case 10:
        model = "AnyaForger";
        break;
    }

    LoadObjects(model);
}

void processSkyboxMenuEvents(int option) {
    std::string skybox;
    switch (option) {
    case 1:
        skybox = "photostudio_02_2k.png";
        break;
    case 2:
        skybox = "sunflowers_2k.png";
        break;
    case 3:
        skybox = "veranda_2k.png";
        break;
    case 4:
        skybox = "ntpu_EECSBuilding.png";
        break;
    }

    CreateSkybox(skybox);
}

// Create menu.
void createModelMenus() {

    int menu;

    // create the menu and tell glut that "processModelMenuEvents" will handle the events
    menu = glutCreateMenu(processModelMenuEvents);

    //add entries to our menu
    glutAddMenuEntry("Arcanine", 1);
    glutAddMenuEntry("Ferrari", 2);
    glutAddMenuEntry("Gengar", 3);
    glutAddMenuEntry("Ivysaur", 4);
    glutAddMenuEntry("Koffing", 5);
    glutAddMenuEntry("MagikarpF", 6);
    glutAddMenuEntry("Rose", 7);
    glutAddMenuEntry("Slowbro", 8);
    glutAddMenuEntry("TexCube", 9);
    glutAddMenuEntry("AnyaForger", 10);

    // attach the menu to the left button
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void createSkyboxMenus() {

    int menu;

    // create the menu and tell glut that "processSkyboxMenuEvents" will handle the events
    menu = glutCreateMenu(processSkyboxMenuEvents);

    //add entries to our menu
    glutAddMenuEntry("Photo Studio", 1);
    glutAddMenuEntry("Sunflowers", 2);
    glutAddMenuEntry("Veranda", 3);
    glutAddMenuEntry("NTPU EECS Building", 4);

    // attach the menu to the left button
    glutAttachMenu(GLUT_LEFT_BUTTON);
}

int main(int argc, char** argv)
{
    // Setting window properties.
    glutInit(&argc, argv);
    glutSetOption(GLUT_MULTISAMPLE, 4);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(screenWidth, screenHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Texture Mapping");

    // Initialize GLEW.
    // Must be done after glut is initialized!
    GLenum res = glewInit();
    if (res != GLEW_OK) {
        std::cerr << "GLEW initialization error: " 
                  << glewGetErrorString(res) << std::endl;
        return 1;
    }

    // Initialization.
    SetupRenderState();
    CreateLights();
    CreateCamera();
    CreateSkybox("ntpu_EECSBuilding.png");
    CreateShaderLib();
    LoadObjects("AnyaForger");
    createModelMenus();     // Click right mouse button.
    createSkyboxMenus();    // Click left mouse button.

    // Register callback functions.
    glutDisplayFunc(RenderSceneCB);
    glutIdleFunc(RenderSceneCB);
    glutReshapeFunc(ReshapeCB);
    glutSpecialFunc(ProcessSpecialKeysCB);
    glutKeyboardFunc(ProcessKeysCB);

    // Start rendering loop.
    glutMainLoop();

    return 0;
}
