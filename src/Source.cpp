

#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

// include the provided basic shape meshes code
#include "meshes.h"

#include <camera.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
	const char* const WINDOW_TITLE = "CS 330 Final Project - Jonathon Smith"; // Macro for window title

	// Variables for window width and height
	const int WINDOW_WIDTH = 1200;
	const int WINDOW_HEIGHT = 1200;

	// Stores the GL data relative to a given mesh
	struct GLMesh
	{
		GLuint vao;         // Handle for the vertex array object
		GLuint vbos[2];     // Handles for the vertex buffer objects
		GLuint nIndices;    // Number of indices of the mesh
	};

	// Main GLFW window
	GLFWwindow* gWindow = nullptr;
	
	//textures
	GLuint gRedWoodGrain;
	GLuint gGreenWoodGrain;
	GLuint gBoard;
	GLuint gTable;
	GLuint gWoodGrain;
	GLuint gNoise;
	GLuint gStitch;
	GLuint gDots;
	GLuint gCardStack;
	GLuint gChanceCard;
	GLuint gCommunityChestCard;
	GLuint gBoardwalk;
	GLuint gParkPlace;
	GLuint gSmudge;
	GLuint gDottedMetal;
	GLuint gPaper;
	GLuint g500;
	GLuint g100;
	GLuint g50;
	GLuint g20;
	GLuint g10;
	GLuint g5;
	GLuint g1;

	//assign these to x,y,z vals of any object for testing
	//uses up, down, left right, 7, 8 for .1 increments
	float xTest = 0.f;
	float yTest = 0.f;
	float zTest = 0.f;

	//tiling
	static glm::vec2 gUVScale(1.0f, 1.0f);

	// Shader program
	GLuint gProgramId;

	// camera
	Camera gCamera(glm::vec3(-3.5f, 5.0f, 15.0f));
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	//Shape Meshes from Professor Brian
	Meshes meshes;

	float gDeltaTime = 0.0f; // Time between current frame and last frame
	float gLastFrame = 0.0f;

	//flag for projection type
	bool isOrthographic = false;
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UPKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
bool UCreateTexture(const char* filename, GLuint& textureId);
void flipImageVertically(unsigned char* image, int width, int height, int channels);
void UDestroyTexture(GLuint& textureId);
void renderMoneyDenomination(GLuint texture, glm::vec3 translation, float rotationAngle, GLint modelLoc);


/* Surface Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 vertexPosition; // VAP position 0 for vertex position data
	layout(location = 1) in vec3 vertexNormal; // VAP position 1 for normals
	layout(location = 2) in vec2 textureCoordinate;  // VAP position 2 for texture coordinates

	out vec3 vertexFragmentNormal; // For outgoing normals to fragment shader
	out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
	out vec2 vertexTextureCoordinate;

	//Uniform / Global variables for the  transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main()
	{
		gl_Position = projection * view * model * vec4(vertexPosition, 1.0f); // Transforms vertices into clip coordinates

		vertexFragmentPos = vec3(model * vec4(vertexPosition, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

		vertexFragmentNormal = mat3(transpose(inverse(model))) * vertexNormal; // get normal vectors in world space only and exclude normal translation properties
		vertexTextureCoordinate = textureCoordinate;
	}
);

/* Surface Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,

	in vec3 vertexFragmentNormal; // For incoming normals
	in vec3 vertexFragmentPos; // For incoming fragment position
	in vec2 vertexTextureCoordinate;

	out vec4 fragmentColor; // For outgoing cube color to the GPU

	// Uniforms for the ambient light
	uniform vec4 objectColor;
	uniform vec3 ambientColor;
	uniform float ambientStrength;

	// Uniforms for light colors and positions
	uniform vec3 light1Color;
	uniform vec3 light1Position;
	uniform vec3 light2Color;
	uniform vec3 light2Position;

	// Uniforms for lioghting details
	uniform float specularIntensity1;
	uniform float highlightSize1;
	uniform float specularIntensity2; 
	uniform float highlightSize2; 

	// Camera position for specular calculation
	uniform vec3 viewPosition;

	// Texture uniforms
	uniform sampler2D uTexture;
	uniform vec2 uvScale; 
	uniform sampler2D uSecondTexture; 
	uniform vec2 UvScale2; 
	uniform float blendFactor;

	// Texture boolean
	uniform bool ubHasTexture;

	void main() {

		// Ambient component
		vec3 ambient = ambientStrength * ambientColor;

		//**Calculate Diffuse lighting**
		vec3 norm = normalize(vertexFragmentNormal); // Normalize vectors to 1 unit
		vec3 light1Direction = normalize(light1Position - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
		float impact1 = max(dot(norm, light1Direction), 0.0);// Calculate diffuse impact by generating dot product of normal and light
		vec3 diffuse1 = impact1 * light1Color; // Generate diffuse light color
		vec3 light2Direction = normalize(light2Position - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
		float impact2 = max(dot(norm, light2Direction), 0.0);// Calculate diffuse impact by generating dot product of normal and light
		vec3 diffuse2 = impact2 * light2Color; // Generate diffuse light color

		//**Calculate Specular lighting**
		vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // // Calculate the view direction vector from the fragment position to the camera
		vec3 reflectDir1 = reflect(-light1Direction, norm);// Calculate reflection vector	

		// Calculate the specular component by taking the dot product of the view direction and the reflection vector,
		// raising it to the power of the highlight size, and then multiplying by the specular intensity and light color.
		float specularComponent1 = pow(max(dot(viewDir, reflectDir1), 0.0), highlightSize1);
		vec3 specular1 = specularIntensity1 * specularComponent1 * light1Color;

		// Repeat the specular calculation for the second light source.
		vec3 reflectDir2 = reflect(-light2Direction, norm);
		float specularComponent2 = pow(max(dot(viewDir, reflectDir2), 0.0), highlightSize2);
		vec3 specular2 = specularIntensity2 * specularComponent2 * light2Color;

		// Texture Colors
		
		// Sample the texture color from the first texture using the UV coordinates, scaled by uvScale
		vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

		// repeat for second texture
		vec4 textureColor2 = texture(uSecondTexture, vertexTextureCoordinate * UvScale2);

		// Blend the two textures based on the blend factor
		vec4 combinedTextureColor = mix(textureColor, textureColor2, blendFactor);
		vec3 phong1;
		vec3 phong2;

		//Allows textures or colors 
		if (ubHasTexture == true) 
		{
			phong1 = (ambient + diffuse1 + specular1) * combinedTextureColor.xyz;
			phong2 = (ambient + diffuse2 + specular2) * combinedTextureColor.xyz;
		}
		else
		{
			phong1 = (ambient + diffuse1 + specular1) * objectColor.xyz;
			phong2 = (ambient + diffuse2 + specular2) * objectColor.xyz;
		}

		fragmentColor = vec4(phong1 + phong2, 1.0);
});

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; ++j)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;

		for (int i = width * channels; i > 0; --i)
		{
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
	}
}
///////////////////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[])
{
	if (!UInitialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	meshes.CreateMeshes();
	// Create the shader program
	if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
		return EXIT_FAILURE;

	// Load textures

	// Load the red wood texture
	if (!UCreateTexture("resources/red-wood.jpg", gRedWoodGrain))
	{
		cout << "Failed to load texture red wood texture" << endl;
		return EXIT_FAILURE;
	}

	// Load the green wood texture
	if (!UCreateTexture("resources/green-wood.jpg", gGreenWoodGrain))
	{
		cout << "Failed to load texture green wood texture" << endl;
		return EXIT_FAILURE;
	}

	// Load the Monopoly board texture
	if (!UCreateTexture("resources/monopoly_board.jpg", gBoard)) {
		cout << "Failed to load Monopoly board texture" << endl;
		return EXIT_FAILURE;
	}

	// Load the table texture
	if (!UCreateTexture("resources/table.jpg", gTable)) {
		cout << "Failed to load table texture" << endl;
		return EXIT_FAILURE;
	}

	// Load the wood grain texture
	if (!UCreateTexture("resources/wood-grain.jpg", gWoodGrain)) {
		cout << "Failed to load wood grain texture" << endl;
		return EXIT_FAILURE;
	}

	// Load the noise texture
	if (!UCreateTexture("resources/noise.jpg", gNoise)) {
		cout << "Failed to load noise texture" << endl;
		return EXIT_FAILURE;
	}

	// Load the stitch texture
	if (!UCreateTexture("resources/stitch.jpg", gStitch)) {
		cout << "Failed to load stitch texture" << endl;
		return EXIT_FAILURE;
	}

	// Load the dots texture
	if (!UCreateTexture("resources/dice_dots.png", gDots)) {
		cout << "Failed to load dots texture" << endl;
		return EXIT_FAILURE;
	}

	// Load the card stack texture
	if (!UCreateTexture("resources/card-stack.jpg", gCardStack)) {
		cout << "Failed to load card stack texture" << endl;
		return EXIT_FAILURE;
	}

	// Load the chance card texture
	if (!UCreateTexture("resources/chance_card.jpg", gChanceCard)) {
		cout << "Failed to load chance card texture" << endl;
		return EXIT_FAILURE;
	}

	// Load the community chest card texture
	if (!UCreateTexture("resources/community_chest_card.jpg", gCommunityChestCard)) {
		cout << "Failed to load community chest card texture" << endl;
		return EXIT_FAILURE;
	}

	// Load the boardwalk texture
	if (!UCreateTexture("resources/boardwalk.jpg", gBoardwalk)) {
		cout << "Failed to load boardwalk texture" << endl;
		return EXIT_FAILURE;
	}

	// Load the park place texture
	if (!UCreateTexture("resources/park_place.jpg", gParkPlace)) {
		cout << "Failed to load park place texture" << endl;
		return EXIT_FAILURE;
	}

	// Load the smudge texture
	if (!UCreateTexture("resources/smudge.jpg", gSmudge)) {
		cout << "Failed to smudge texture" << endl;
		return EXIT_FAILURE;
	}
	// Load the dotted_metal texture
	if (!UCreateTexture("resources/dotted_metal.jpg", gDottedMetal)) {
		cout << "Failed to dotted_metal texture" << endl;
		return EXIT_FAILURE;
	}

	// Load the paper texture
	if (!UCreateTexture("resources/paper.jpg", gPaper)) {
		cout << "Failed to paper texture" << endl;
		return EXIT_FAILURE;
	}

	// Load the 500 texture
	if (!UCreateTexture("resources/500.jpg", g500)) {
		cout << "Failed to 500 texture" << endl;
		return EXIT_FAILURE;
	}
	// Load the 100 texture
	if (!UCreateTexture("resources/100.jpg", g100)) {
		cout << "Failed to 100 texture" << endl;
		return EXIT_FAILURE;
	}
	// Load the 50 texture
	if (!UCreateTexture("resources/50.jpg", g50)) {
		cout << "Failed to 50 texture" << endl;
		return EXIT_FAILURE;
	}
	// Load the 10 texture
	if (!UCreateTexture("resources/10.jpg", g10)) {
		cout << "Failed to 10 texture" << endl;
		return EXIT_FAILURE;
	}
	// Load the 5 texture
	if (!UCreateTexture("resources/5.jpg", g5)) {
		cout << "Failed to 5 texture" << endl;
		return EXIT_FAILURE;
	}
	// Load the 1 texture
	if (!UCreateTexture("resources/1.jpg", g1)) {
		cout << "Failed to 1 texture" << endl;
		return EXIT_FAILURE;
	}

	// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	glUseProgram(gProgramId);


	// Sets the background color of the window to black (it will be implicitely used by glClear)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(gWindow))
	{

		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		// input
		// -----
		UProcessInput(gWindow);

		// Render this frame
		URender();

		glfwPollEvents();
	}

	// Release mesh data
	meshes.DestroyMeshes();

	// Release textures
	UDestroyTexture(gRedWoodGrain);
	UDestroyTexture(gGreenWoodGrain);
	UDestroyTexture(gBoard);
	UDestroyTexture(gTable);
	UDestroyTexture(gWoodGrain);
	UDestroyTexture(gNoise);
	UDestroyTexture(gStitch);
	UDestroyTexture(gDots);
	UDestroyTexture(gCardStack);
	UDestroyTexture(gChanceCard);
	UDestroyTexture(gCommunityChestCard);
	UDestroyTexture(gBoardwalk);
	UDestroyTexture(gParkPlace);
	UDestroyTexture(gSmudge);
	UDestroyTexture(gDottedMetal);
	UDestroyTexture(gPaper);
	UDestroyTexture(g500);
	UDestroyTexture(g100);
	UDestroyTexture(g50);
	UDestroyTexture(g10);
	UDestroyTexture(g5);
	UDestroyTexture(g1);

	// Release shader program
	UDestroyShaderProgram(gProgramId);

	exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
	// GLFW: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// GLFW: window creation
	// ---------------------
	* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	if (*window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);

	// Set up mouse event callbacks
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);
	glfwSetKeyCallback(*window, UPKeyCallback);

	// Calculate the center coordinates
	double centerX = static_cast<double>(WINDOW_WIDTH) / 2.0;
	double centerY = static_cast<double>(WINDOW_HEIGHT) / 2.0;

	// Set the mouse cursor position to the center
	glfwSetCursorPos(*window, centerX, centerY);

	// Disables cursor capture
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// GLEW: initialize
	// ----------------
	// Note: if using GLEW version 1.13 or earlier
	glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();

	if (GLEW_OK != GlewInitResult)
	{
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}

	// Displays GPU OpenGL version
	cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

	return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
	bool pKeyWasPressed = false;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		gCamera.ProcessKeyboard(LEFT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		gCamera.ProcessKeyboard(UP, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		gCamera.ProcessKeyboard(DOWN, gDeltaTime);

	//Testing code for x,y,z positioning

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		yTest += 0.01f;
		cout << "y = " << yTest << endl;
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		yTest -= 0.01f;
		cout << "y = " << yTest << endl;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		xTest += 0.01f;
		cout << "x = " << xTest << endl;
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		xTest -= 0.01f;
		cout << "x = " << xTest << endl;
	}
	if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) {
		zTest += 0.01f;
		cout << "z = " << zTest << endl;
	}
	if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) {
		zTest -= 0.01f;
		cout << "z = " << zTest << endl;
	}
		
}


// glfw: Whenever the mouse moves, this callback is called.
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{

	if (gFirstMouse)
	{
		gLastX = xpos;
		gLastY = ypos;
		gFirstMouse = false;
	}

	float xoffset = xpos - gLastX;
	float yoffset = gLastY - ypos; // Reversed since y-coordinates go from bottom to top

	gLastX = xpos;
	gLastY = ypos;

	gCamera.ProcessMouseMovement(xoffset, yoffset);

}

void UPKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Toggle perspective projection
	if (key == GLFW_KEY_P && action == GLFW_RELEASE)
	{
		isOrthographic = false; // Set to false to use perspective projection
	}

	// Toggle orthographic projection
	if (key == GLFW_KEY_O && action == GLFW_RELEASE)
	{
		isOrthographic = true; // Set to true to use orthographic projection
	}
}


// glfw: Whenever the mouse scroll wheel scrolls, this callback is called.
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	gCamera.ProcessMouseScroll(yoffset);
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}


// Functioned called to render a frame
void URender()
{
	GLint modelLoc;
	GLint viewLoc;
	GLint projLoc;
	GLint viewPosLoc;
	GLint ambStrLoc;
	GLint ambColLoc;
	GLint light1ColLoc;
	GLint light1PosLoc;
	GLint light1Direction;
	GLint light2ColLoc;
	GLint light2PosLoc;
	GLint objColLoc;
	GLint specInt1Loc;
	GLint highlghtSz1Loc;
	GLint specInt2Loc;
	GLint highlghtSz2Loc;
	GLint uHasTextureLoc;
	bool ubHasTextureVal;
	glm::mat4 scale;
	glm::mat4 rotation;
	glm::mat4 rotation1;
	glm::mat4 rotation2;
	glm::mat4 rotation3;
	glm::mat4 translation;
	glm::mat4 objectTranslation;
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
	glm::vec2 uvScale;
	glm::vec2 uvScale2;

	// Enable z-depth
	glEnable(GL_DEPTH_TEST);

	// Clear the frame and z buffers
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Transforms the camera
	// Calculate camera's position in polar coordinates
	view = gCamera.GetViewMatrix();

	// Creates a projection using isOrthographic value
	if (isOrthographic)
	{
		projection = glm::ortho(-15.f, 15.f, -15.f, 15.f, 0.1f, 100.f);
	}
	else
	{
		projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}

	// Set the shader to be used
	glUseProgram(gProgramId);

	// Default blend factor 
	float defaultBlendFactor = 0.0f;
	//blend factor for the secondary texture
	float noiseBlendFactor = 0.f; 

	// Get the location of the blendFactor uniform
	GLint blendFactorLocation = glGetUniformLocation(gProgramId, "blendFactor");

	// Set default blend factor before rendering any object
	glUniform1f(blendFactorLocation, defaultBlendFactor);

	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(gProgramId, "model");
	viewLoc = glGetUniformLocation(gProgramId, "view");
	projLoc = glGetUniformLocation(gProgramId, "projection");
	viewPosLoc = glGetUniformLocation(gProgramId, "viewPosition");
	ambStrLoc = glGetUniformLocation(gProgramId, "ambientStrength");
	ambColLoc = glGetUniformLocation(gProgramId, "ambientColor");
	light1ColLoc = glGetUniformLocation(gProgramId, "light1Color");
	light1PosLoc = glGetUniformLocation(gProgramId, "light1Position");
	light2ColLoc = glGetUniformLocation(gProgramId, "light2Color");
	light2PosLoc = glGetUniformLocation(gProgramId, "light2Position");
	objColLoc = glGetUniformLocation(gProgramId, "objectColor");
	specInt1Loc = glGetUniformLocation(gProgramId, "specularIntensity1");
	highlghtSz1Loc = glGetUniformLocation(gProgramId, "highlightSize1");
	specInt2Loc = glGetUniformLocation(gProgramId, "specularIntensity2");
	highlghtSz2Loc = glGetUniformLocation(gProgramId, "highlightSize2");
	uHasTextureLoc = glGetUniformLocation(gProgramId, "ubHasTexture");

	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(gProgramId, "model");
	viewLoc = glGetUniformLocation(gProgramId, "view");
	projLoc = glGetUniformLocation(gProgramId, "projection");

	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	
	//set the camera view location
	glUniform3f(viewPosLoc, gCamera.Position.x, gCamera.Position.y, gCamera.Position.z);

	GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

	///// set ambient lighting for entire scene---------------------
	glUniform1f(ambStrLoc, .8f);                //ambient lighting strength
	glUniform3f(ambColLoc, .5f, .5f, .5f);    //ambient lighting color
	/////----------------------------------------------------------

	/*******************************
	*
	*			GAME PIECES
	* 
	* ******************************/

	/*******************************
	*
	*			Thimble
	*
	* ******************************/


	/******THIMBLE COMMON PROPERTIES*******/

	glUseProgram(gProgramId);
	glUniform1i(glGetUniformLocation(gProgramId, "ubHasTexture"), GL_TRUE); // Enable texturing
	
	// Diffuse lighting
	glUniform3f(light1ColLoc, 0.3f, 0.3f, 0.3f); // Soft white light from overhead
	glUniform3f(light1PosLoc, 5.0f, 2.0f, 10.0f); // Overhead light position

	// Specular lighting
	glUniform3f(light2ColLoc, 0.6f, 0.6f, 0.6f); // Moderate white side light for depth
	glUniform3f(light2PosLoc, 5.0f, 2.0f, 10.0f); // Side light position

	// High specular intensity and concentrated highlight size make the object shiny
	glUniform1f(specInt1Loc, 0.9f); 
	glUniform1f(highlghtSz1Loc, 10.f); 
	glUniform1f(specInt2Loc, 0.9f); 
	glUniform1f(highlghtSz2Loc, 10.f); 

	/******THIMBLE BASE*******/

	glBindVertexArray(meshes.gTorusMesh.vao);
	glBindTexture(GL_TEXTURE_2D, gDottedMetal); // Bind dotted metal texture for thimble bottom

	// Tiny scale samples color from the dotted metal texture
	uvScale = glm::vec2(.00001f, .00001f);
	glUniform2fv(glGetUniformLocation(gProgramId, "uvScale"), 1, glm::value_ptr(uvScale));

	// Transformations for the base
	scale = glm::scale(glm::vec3(.1f, .1f, .1f));
	rotation = glm::rotate(glm::radians(90.f), glm::vec3(1.f, 0.f, 0.0f));
	translation = glm::translate(glm::vec3(-1.7f, 0.f, 4.1f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	
	// Draw the thimble bottom
	glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);

	/******THIMBLE MIDDLE*******/

	glBindVertexArray(meshes.gTaperedCylinderMesh.vao);

	// UV scale adjustments for the middle 
	uvScale = glm::vec2(5.f, 3.f);
	glUniform2fv(glGetUniformLocation(gProgramId, "uvScale"), 1, glm::value_ptr(uvScale));

	// Transformations for the middle 
	rotation = glm::rotate(glm::radians(0.f), glm::vec3(1.f, 0.f, 0.0f)); //Rotation is reset 
	scale = glm::scale(glm::vec3(.105f, .2f, .105f));

	
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	// Draw the middle part
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146); // Only drawing the sides as top/bottom are likely covered

	/******THIMBLE TOP*******/

	glBindVertexArray(meshes.gSphereMesh.vao);

	// Tiny scale samples color from the dotted metal texture
	uvScale = glm::vec2(.00001f, .00001f);
	glUniform2fv(glGetUniformLocation(gProgramId, "uvScale"), 1, glm::value_ptr(uvScale));

	// Transformations for the top 
	scale = glm::scale(glm::vec3(0.0555f, 0.032f, 0.0555f));
	translation = glm::translate(glm::vec3(-1.7f, .19f, 4.1f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	
	// Draw the thimble top
	glDrawElements(GL_TRIANGLES, meshes.gSphereMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Cleanup after drawing the thimble
	glBindVertexArray(0);


	/*******************************
	*
	*			TOP HAT
	*
	* ******************************/

	/******TOP HAT COMMON PROPERTIES*******/

	// Activate the shader program
	glUseProgram(gProgramId);

	// Enable texturing
	glUniform1i(glGetUniformLocation(gProgramId, "ubHasTexture"), GL_TRUE);

	// Setup common lighting properties for the Top Hat
	// Diffuse lighting
	glUniform3f(light1ColLoc, 0.3f, 0.3f, 0.3f); // Soft white light from overhead
	glUniform3f(light1PosLoc, 5.0f, 2.0f, 10.0f); // Overhead light position

	// Specular lighting
	glUniform3f(light2ColLoc, 0.6f, 0.6f, 0.6f); // Moderate white side light for depth
	glUniform3f(light2PosLoc, 5.0f, 2.0f, 10.0f); // Side light position

	// High specular intensity and concentrated highlight size make the object shiny
	glUniform1f(specInt1Loc, 0.9f);
	glUniform1f(highlghtSz1Loc, 10.f);
	glUniform1f(specInt2Loc, 0.9f);
	glUniform1f(highlghtSz2Loc, 10.f);

	//Total object translation
	objectTranslation = glm::translate(glm::vec3(-2.7f, 0.f, 2.39f));

	/******TOP HAT BASE*******/

	// Bind the cylinder mesh for the base
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// Transformations for the base
	scale = glm::scale(glm::vec3(.1f, .011f, .1f)); // Scale the base to appropriate dimensions
	rotation = glm::rotate(glm::radians(0.f), glm::vec3(1.f, 0.f, 0.0f)); // No rotation needed

	// Apply transformations
	model = objectTranslation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the base cylinder
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36); // Bottom face
	glDrawArrays(GL_TRIANGLE_FAN, 36, 72); // Top face
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146); // Side faces

	/******TOP HAT BRIM (TORUS)*******/

	// Bind the torus mesh for the brim
	glBindVertexArray(meshes.gTorusMesh.vao);

	// Transformations for the brim
	scale = glm::scale(glm::vec3(.1f, .109f, .13f)); // Scale the brim larger than the base
	rotation = glm::rotate(glm::radians(90.f), glm::vec3(1.f, 0.f, 0.0f)); // Rotate to lay flat
	translation = glm::translate(glm::vec3(0.f, 0.003f, 0.f)); // Adjust height slightly above the base

	// Apply transformations
	model = objectTranslation * translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the brim
	glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);

	/******TOP HAT TOP*******/

	// Bind the cylinder mesh for the top 
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// Transformations for the top
	scale = glm::scale(glm::vec3(.065f, .075f, .065f)); // Scale for top part dimensions
	rotation = glm::rotate(glm::radians(0.f), glm::vec3(1.f, 0.f, 0.0f)); // No rotation needed
	translation = glm::translate(glm::vec3(-0.f, .011f, 0.f)); // Position on top of the base

	// Apply transformations
	model = objectTranslation * translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the top part
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36); // Bottom face
	glDrawArrays(GL_TRIANGLE_FAN, 36, 72); // Top face
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146); // Side faces

	// Cleanup
	glBindVertexArray(0);

	/*******************************
	*
	*			IRON
	*
	* ******************************/

	/******IRON COMMON PROPERTIES*******/

	// Activate the shader program
	glUseProgram(gProgramId);

	// Enable texturing
	glUniform1i(glGetUniformLocation(gProgramId, "ubHasTexture"), GL_TRUE);

	// Setup common lighting properties for the Top Hat
	// Diffuse lighting
	glUniform3f(light1ColLoc, 0.3f, 0.3f, 0.3f); // Soft white light from overhead
	glUniform3f(light1PosLoc, 5.0f, 2.0f, 10.0f); // Overhead light position

	// Specular lighting
	glUniform3f(light2ColLoc, 0.6f, 0.6f, 0.6f); // Moderate white side light for depth
	glUniform3f(light2PosLoc, 5.0f, 2.0f, 10.0f); // Side light position

	// High specular intensity and concentrated highlight size make the object shiny
	glUniform1f(specInt1Loc, 0.9f);
	glUniform1f(highlghtSz1Loc, 10.f);
	glUniform1f(specInt2Loc, 0.9f);
	glUniform1f(highlghtSz2Loc, 10.f);

	//Total object translation
	objectTranslation = glm::translate(glm::vec3(-.32, 0.01f, 4.4f)); 

	/******IRON BASE SQUARE*******/

	// Bind the box mesh for the base square
	glBindVertexArray(meshes.gBoxMesh.vao);

	// Transformations for the base square
	scale = glm::scale(glm::vec3(.2f, .02f, .2f)); // Scale to appropriate size for the iron base
	rotation = glm::rotate(glm::radians(0.f), glm::vec3(0.f, 0.f, 1.f)); // No rotation needed
	translation = glm::translate(glm::vec3(-.32, 0.01f, 4.4f)); // Position the base

	// Apply transformations
	model = objectTranslation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the base square
	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	/******IRON BASE TRIANGLE*******/

	// Bind the prism mesh for the base triangle
	glBindVertexArray(meshes.gPrismMesh.vao);

	// Transformations for the base triangle
	scale = glm::scale(glm::vec3(.2f, .02f, .10f)); // Adjust size for the iron's pointed front
	rotation = glm::rotate(glm::radians(-90.f), glm::vec3(0.f, 1.f, 0.f)); // Rotate to align with the base square
	translation = glm::translate(glm::vec3(-.15f, 0.0f, 0.f)); // Position relative to the base square

	// Apply transformations
	model = objectTranslation * translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the base triangle
	glDrawArrays(GL_TRIANGLE_STRIP, 0, meshes.gPrismMesh.nVertices);

	/******IRON HANDLE*******/

	// Bind the cylinder mesh for the handle
	glBindVertexArray(meshes.gCylinderMesh.vao);

	// Transformations for the handle
	scale = glm::scale(glm::vec3(.01f, .075f, .01f)); // Scale to handle dimensions
	rotation = glm::rotate(glm::radians(30.f), glm::vec3(0.f, 0.f, 1.0f)); // Rotate for ergonomic angle
	translation = glm::translate(glm::vec3(-.07f, 0.f, 0.f)); // Position on top of the base

	// Apply transformations for one side of the handle
	model = objectTranslation * translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw one side of the handle
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146); // Draw sides of the cylinder

	// Adjust rotation for the other side of the handle
	rotation = glm::rotate(glm::radians(-30.f), glm::vec3(0.f, 0.f, 1.0f));
	translation = glm::translate(glm::vec3(.03f, 0.f, 0.f));

	// Apply transformations for the other side
	model = objectTranslation * translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the other side of the handle
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146); // Repeat drawing for symmetry

	// Transformations for the top part of the handle
	scale = glm::scale(glm::vec3(.01f, .1925f, .01f));
	rotation = glm::rotate(glm::radians(90.f), glm::vec3(0.f, 0.f, 1.0f));
	translation = glm::translate(glm::vec3(.0767f, .065f, 0.f));

	// Apply transformations for the top handle
	model = objectTranslation * translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the top of the handle
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 72);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Cleanup
	glBindVertexArray(0);
	
	/*******************************
	*
	*			DICE
	*
	* ******************************/

	/******DICE COMMON PROPERTIES*******/

	// Activate shader program and enable texturing
	glUseProgram(gProgramId);
	glUniform1i(glGetUniformLocation(gProgramId, "ubHasTexture"), GL_TRUE);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gDots); // Bind dots texture representing the dice faces

	// Setup lighting properties for the dice
	// Diffuse Lighting
	glUniform3f(light1ColLoc, 0.3f, 0.3f, 0.3f); // Soft white light 
	glUniform3f(light1PosLoc, 0.0f, 0.0f, 10.0f); // Overhead light source

	// Specular Lighting
	glUniform3f(light2ColLoc, 0.6f, 0.6f, 0.6f); // Moderate white light 
	glUniform3f(light2PosLoc, 10.0f, 1.0f, 3.0f); // Side light source for added depth
	glUniform1f(specInt1Loc, 0.5f); // Specular intensity for a moderate shine
	glUniform1f(highlghtSz1Loc, 12.f); // Highlight size for a broad specular reflection
	glUniform1f(specInt2Loc, 0.5f); // Specular intensity for a moderate shine
	glUniform1f(highlghtSz2Loc, 12.f); // // Highlight size for a broad specular reflection

	/******FIRST DIE*******/

	// Bind the dice mesh VAO
	glBindVertexArray(meshes.gDiceMesh.vao);

	// UV scaling and transformations for the first dice
	uvScale = glm::vec2(1.f, 1.f);
	glUniform2fv(glGetUniformLocation(gProgramId, "uvScale"), 1, glm::value_ptr(uvScale));
	scale = glm::scale(glm::vec3(.2f, .2f, .2f)); // Scale to size
	rotation1 = glm::rotate(glm::radians(270.f), glm::vec3(0.f, 0.f, 1.f)); // Rotate vertically
	rotation2 = glm::rotate(glm::radians(45.f), glm::vec3(1.f, 0.f, 0.f)); // Rotate horizontally
	glm::mat4 combinedRotation = rotation1 * rotation2; // Combine rotations
	translation = glm::translate(glm::vec3(-1.9, .1f, 1.6)); // Position on the object

	// Apply transformations and draw the first die
	model = translation * combinedRotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glDrawElements(GL_TRIANGLES, meshes.gDiceMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	/******SECOND DIE*******/

	// Transformations for the second dice
	rotation1 = glm::rotate(glm::radians(137.f), glm::vec3(0.f, 1.f, 0.f)); // Imperfect rotation for dynamic look
	rotation2 = glm::rotate(glm::radians(0.f), glm::vec3(1.f, 0.f, 0.f)); // Reset rotation
	combinedRotation = rotation1 * rotation2; // Combine rotations
	translation = glm::translate(glm::vec3(-1.5, .1f, 1.8)); // Position the object

	// Apply transformations and draw the second die
	model = translation * combinedRotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glDrawElements(GL_TRIANGLES, meshes.gDiceMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Cleanup 
	glBindVertexArray(0);


	/*******************************
	*
	*			Cards
	*
	* ******************************/

	/****** CARD COMMON PROPERTIES *******/

	// Activate the shader program and enable texturing
	glUseProgram(gProgramId);
	glUniform1i(glGetUniformLocation(gProgramId, "ubHasTexture"), GL_TRUE);

	// Setup lighting properties for the cards
	// Diffuse Lighting
	glUniform3f(light1ColLoc, 0.2f, 0.2f, 0.2f); // Dim white light for a soft appearance
	glUniform3f(light1PosLoc, 0.0f, 0.0f, 10.0f); // Overhead position for uniform lighting

	// Specular Lighting
	glUniform3f(light2ColLoc, 0.2f, 0.2f, 0.2f); // Additional light source for minimal specular highlights
	glUniform3f(light2PosLoc, 10.0f, 1.0f, 3.0f); // Side light position to add depth


	glUniform1f(specInt1Loc, 0.0f); // No specular intensity for a matte finish
	glUniform1f(highlghtSz1Loc, 1.0f); // Broad highlight size for a subtle effect
	glUniform1f(specInt2Loc, 0.0f); // Repeat specular intensity 
	glUniform1f(highlghtSz2Loc, 1.0f); // Repeat highlight 

	/****** CHANCE CARDS *******/

	// Bind the box mesh for Chance cards
	glBindVertexArray(meshes.gBoxMesh.vao);

	/******TOP ANGLED CARD*******/
	// Transformations for top laying card
	scale = glm::scale(glm::vec3(1.25f, .002f, .7f)); // Card dimensions
	rotation = glm::rotate(glm::radians(-3.f), glm::vec3(0.f, 1.f, 0.f)); // Slight rotation 
	translation = glm::translate(glm::vec3(-.6f, .135f, 2.5f)); // Position on the board
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Texture application for Chance card
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gChanceCard);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gPaper);

	// Texture blending for appearance
	uvScale = glm::vec2(1.f, 1.f);
	glUniform2fv(glGetUniformLocation(gProgramId, "uvScale"), 1, glm::value_ptr(uvScale));
	float blendFactor = 0.15f; // Blend with paper texture for a worn look
	glUniform1f(glGetUniformLocation(gProgramId, "blendFactor"), blendFactor);

	// Draw top and bottom parts of the card
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4); // Top part of the card
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4); // Bottom part of the card

	/******CARD STACK*******/
	// Transformations for card stack
	scale = glm::scale(glm::vec3(1.25f, .125f, .7f));
	rotation = glm::rotate(glm::radians(-13.f), glm::vec3(0.f, 1.f, 0.f));
	translation = glm::translate(glm::vec3(-.6f, .07f, 2.5));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw top and bottom 
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4);

	// Texture application for stack
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gCardStack);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gChanceCard);

	// Texture blending 
	uvScale = glm::vec2(1.f, .35f);
	uvScale2 = glm::vec2(1.f, .1f);
	glUniform2fv(glGetUniformLocation(gProgramId, "uvScale"), 1, glm::value_ptr(uvScale));
	glUniform2fv(glGetUniformLocation(gProgramId, "UvScale2"), 1, glm::value_ptr(uvScale2));
	blendFactor = .5f;
	glUniform1f(glGetUniformLocation(gProgramId, "blendFactor"), blendFactor);

	//Draw the sides of the card stack
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4); // First side
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4); // Second side
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4); // Third side
	glDrawArrays(GL_TRIANGLE_FAN, 20, 4); // Fourth side

	//Transformations for the sides of single card
	scale = glm::scale(glm::vec3(1.25f, .002f, .7f));
	rotation = glm::rotate(glm::radians(-3.f), glm::vec3(0.f, 1.f, 0.f));
	translation = glm::translate(glm::vec3(-.6f, .135f, 2.5));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	uvScale = glm::vec2(1.f, .01f); //removes any horizontal lines from the card stack texture for the single card
	glUniform2fv(glGetUniformLocation(gProgramId, "uvScale"), 1, glm::value_ptr(uvScale));

	// Draw the sides of the single card
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 20, 4);

	//*****Community Chest*****//

	/******CARD FACES*******/

	// Texture application for community chest faces
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gCommunityChestCard);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gPaper);

	/******TOP ANGLED CARD*******/
	
	// Transformations for top laying card
	scale = glm::scale(glm::vec3(1.25f, .002f, .7f)); // Card dimensions
	rotation = glm::rotate(glm::radians(-183.f), glm::vec3(0.f, 1.f, 0.f)); // Slight rotation and opposite facing
	translation = glm::translate(glm::vec3(.6f, .135f, -2.5f)); // Position on the board
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Texture blending
	uvScale = glm::vec2(1.f, 1.f);
	glUniform2fv(glGetUniformLocation(gProgramId, "uvScale"), 1, glm::value_ptr(uvScale));
	blendFactor = 0.15f; // Blend with paper texture for a worn look
	glUniform1f(glGetUniformLocation(gProgramId, "blendFactor"), blendFactor);

	// Draw top and bottom parts of the card
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4); // Top part of the card
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4); // Bottom part of the card

	/******CARD STACK*******/

	// Transformations for stack below the angled card
	scale = glm::scale(glm::vec3(1.25f, .125f, .7f));
	rotation = glm::rotate(glm::radians(-13.f), glm::vec3(0.f, 1.f, 0.f));
	translation = glm::translate(glm::vec3(.6f, .07f, -2.5));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw top and bottom 
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4);

	/******SINGLE CARD ON TOP OF THE MONEY*******/

	// Transformations for single card on top of the money
	scale = glm::scale(glm::vec3(1.25f, .002f, .7f));
	rotation = glm::rotate(glm::radians(-60.f), glm::vec3(0.f, 1.f, 0.f));
	translation = glm::translate(glm::vec3(-.3f, -.990f, 6.8f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw top and bottom 
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4);

	/******SIDES*******/

	// Texture application for community chest card sides
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gCardStack);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gCommunityChestCard);

	/******CARD STACK*******/

	// Transformations for stack below the angled card
	scale = glm::scale(glm::vec3(1.25f, .125f, .7f));
	rotation = glm::rotate(glm::radians(-13.f), glm::vec3(0.f, 1.f, 0.f));
	translation = glm::translate(glm::vec3(.6f, .07f, -2.5));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Texture blending 
	uvScale = glm::vec2(1.f, .35f);
	uvScale2 = glm::vec2(1.f, .1f);
	glUniform2fv(glGetUniformLocation(gProgramId, "uvScale"), 1, glm::value_ptr(uvScale));
	glUniform2fv(glGetUniformLocation(gProgramId, "UvScale2"), 1, glm::value_ptr(uvScale2));
	blendFactor = .5f;
	glUniform1f(glGetUniformLocation(gProgramId, "blendFactor"), blendFactor);

	// Sides drawing setup 
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4); // First side
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4); // Second side
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4); // Third side
	glDrawArrays(GL_TRIANGLE_FAN, 20, 4); // Fourth side

	/******TOP LAYING CARD*******/

	// Transformations for sides
	scale = glm::scale(glm::vec3(1.25f, .002f, .7f));
	rotation = glm::rotate(glm::radians(-3.f), glm::vec3(0.f, 1.f, 0.f));
	translation = glm::translate(glm::vec3(.6f, .135f, -2.5));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	uvScale = glm::vec2(1.f, .01f); //removes any horizontal lines from the card stack texture for the single card
	glUniform2fv(glGetUniformLocation(gProgramId, "uvScale"), 1, glm::value_ptr(uvScale));

	// Draw the sides 
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 20, 4);

	/******SINGLE CARD ON TOP OF THE MONEY*******/

	// Transformations for single card on top of the money
	rotation = glm::rotate(glm::radians(-60.f), glm::vec3(0.f, 1.f, 0.f));
	translation = glm::translate(glm::vec3(-.3f, -.990f, 6.8f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the sides 
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 20, 4);

	// Cleanup 
	uvScale = glm::vec2(1.f, 1.f);
	uvScale2 = glm::vec2(1.f, 1.f);
	glUniform2fv(glGetUniformLocation(gProgramId, "uvScale"), 1, glm::value_ptr(uvScale));
	glUniform2fv(glGetUniformLocation(gProgramId, "UvScale2"), 1, glm::value_ptr(uvScale2));
	glBindVertexArray(0);

	/*******************************
	*
	*			PROPERTY CARDS
	*
	* ******************************/

	/****** PROPERTY CARD COMMON PROPERTIES *******/

	// Activate the shader program and enable texturing
	glBindVertexArray(meshes.gBoxMesh.vao);
	glUniform1i(glGetUniformLocation(gProgramId, "ubHasTexture"), GL_TRUE);

	// Bind the box mesh for property cards
	glBindVertexArray(meshes.gBoxMesh.vao);

	// Setup lighting properties for the cards
	// Diffuse Lighting
	glUniform3f(light1ColLoc, 0.2f, 0.2f, 0.2f); // Dim white light for a soft appearance
	glUniform3f(light1PosLoc, 0.0f, 0.0f, 10.0f); // Overhead position for uniform lighting

	// Specular Lighting
	glUniform3f(light2ColLoc, 0.2f, 0.2f, 0.2f); // Additional light source for minimal specular highlights
	glUniform3f(light2PosLoc, 10.0f, 1.0f, 3.0f); // Side light position to add depth


	glUniform1f(specInt1Loc, 0.0f); // No specular intensity for a matte finish
	glUniform1f(highlghtSz1Loc, 1.0f); 
	glUniform1f(specInt2Loc, 0.0f); //
	glUniform1f(highlghtSz2Loc, 1.0f); 

	/****** PARK PLACE *******/

	// Transformations for Park Place
	scale = glm::scale(glm::vec3(1.25f, .002f, 1.25f));
	rotation = glm::rotate(glm::radians(45.f), glm::vec3(0.f, 1.f, 0.f));
	translation = glm::translate(glm::vec3(-3.f, -1.f, 5.3f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Texture application for Park Place
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gParkPlace);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gSmudge); 

	// Texture blending for appearance
	blendFactor = 0.08f; // Blend with smudge texture 
	glUniform1f(glGetUniformLocation(gProgramId, "blendFactor"), blendFactor);

	// Draw Park Place
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4); //Only need one face

	/****** BOARDWALK *******/

	// Transformations for Boardwalk

	rotation = glm::rotate(glm::radians(33.f), glm::vec3(0.f, 1.f, 0.f));
	translation = glm::translate(glm::vec3(-2.7f, -.999f, 5.6f));
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Texture application for Boardwalk
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBoardwalk);
	// No second texture or blend factor needed for Boardwalk as per previous example

	// Draw Boardwalk
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4); // Bottom part of the card

	// Cleanup
	glBindVertexArray(0);

	/*******************************
	 *
	 *          Money
	 *
	 ******************************/

	 /****** MONEY COMMON PROPERTIES *******/

	 // Activate the shader program and enable texturing
	glUseProgram(gProgramId);
	glUniform1i(glGetUniformLocation(gProgramId, "ubHasTexture"), GL_TRUE);

	// Setup lighting properties for the money
	// Diffuse Lighting
	glUniform3f(light1ColLoc, 0.2f, 0.2f, 0.2f); // Soft white light for a gentle appearance
	glUniform3f(light1PosLoc, 0.0f, 0.0f, 10.0f); // Overhead position for even lighting

	// Specular Lighting
	glUniform3f(light2ColLoc, 0.2f, 0.2f, 0.2f); // Secondary light source for soft highlights
	glUniform3f(light2PosLoc, 10.0f, 1.0f, 3.0f); // Side light position

	glUniform1f(specInt1Loc, 0.0f); // No specular intensity for a flat finish
	glUniform1f(highlghtSz1Loc, 1.0f); // Wide highlight size for a soft effect
	glUniform1f(specInt2Loc, 0.0f); // No specular intensity for the secondary light
	glUniform1f(highlghtSz2Loc, 1.0f); // Wide highlight size for the secondary light

	// Bind the box mesh for all money denominations
	glBindVertexArray(meshes.gBoxMesh.vao);

	/****** RENDER MONEY DENOMINATIONS *******/

	// Render each denomination
	renderMoneyDenomination(g500, glm::vec3(.23f, -.993f, 6.53f), -45.f, modelLoc);
	renderMoneyDenomination(g100, glm::vec3(.42f, -.994f, 6.45f), -35.f, modelLoc);
	renderMoneyDenomination(g50, glm::vec3(.6f, -.995f, 6.34f), -25.f, modelLoc);
	renderMoneyDenomination(g10, glm::vec3(.75f, -.996f, 6.2f), -15.f, modelLoc);
	renderMoneyDenomination(g5, glm::vec3(.9f, -.997f, 6.05f), -5.f, modelLoc);
	renderMoneyDenomination(g1, glm::vec3(1.f, -.998f, 5.87f), 5.f, modelLoc);

	// Cleanup 
	glBindVertexArray(0);

	/*******************************
	 *
	 *          Table
	 *
	 ******************************/

	 /****** TABLE PLANE *******/

	 // Activate the shader program and enable texturing for the table plane
	glUseProgram(gProgramId);
	glUniform1i(glGetUniformLocation(gProgramId, "ubHasTexture"), GL_TRUE);

	// Lighting setup for the table plane
	// Diffuse lighting
	glUniform3f(light1ColLoc, 0.4f, 0.4f, 0.4f); // Soft white light
	glUniform3f(light1PosLoc, 0.0f, 0.0f, 10.0f); // Overhead position

	// Specular lighting
	glUniform3f(light2ColLoc, 0.4f, 0.4f, 0.4f); // Secondary light source for soft highlights
	glUniform3f(light2PosLoc, 10.0f, 1.0f, 3.0f); // Side light position

	glUniform1f(specInt1Loc, 1.0f); // High specular intensity
	glUniform1f(highlghtSz1Loc, 50.f); // Smaller highlight size for sharp reflections
	glUniform1f(specInt2Loc, 1.0f); // High specular intensity for the secondary light
	glUniform1f(highlghtSz2Loc, 50.f); // Smaller highlight size for secondary light

	// Bind the table texture and set texture parameters
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTable);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Bind the VAO
	glBindVertexArray(meshes.gPlaneMesh.vao);

	// Apply transformations to the table plane
	translation = glm::translate(glm::vec3(0.0f, -1.01f, 0.f));
	rotation = glm::rotate(glm::radians(-14.f), glm::vec3(.0f, 1.0f, .0f));
	scale = glm::scale(glm::vec3(10.0f, 5.0f, 8.0f));

	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the table plane
	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	/*******************************
	 *
	 *          BOARD SURFACE
	 *
	 ******************************/

	// Activate the shader program and enable texturing for the playing surface
	glUseProgram(gProgramId);
	glUniform1i(glGetUniformLocation(gProgramId, "ubHasTexture"), GL_TRUE);

	// Lighting setup for the playing surface

	// Diffuse lighting
	glUniform3f(light1ColLoc, 0.4f, 0.4f, 0.4f); // Soft white light
	glUniform3f(light1PosLoc, 0.0f, 0.0f, 10.0f); // Overhead position
	// Specular lighting
	glUniform3f(light2ColLoc, 0.4f, 0.4f, 0.4f); // Secondary light source for soft highlights
	glUniform3f(light2PosLoc, 10.0f, 1.0f, 3.0f); // Side light position

	glUniform1f(specInt1Loc, 0.0f); // No specular intensity for a matte finish
	glUniform1f(highlghtSz1Loc, 100.f); // Broad highlight size for a soft effect
	glUniform1f(specInt2Loc, 0.0f); // No specular intensity for the secondary light
	glUniform1f(highlghtSz2Loc, 100.f); // Broad highlight size for the secondary light

	// Bind the board and stitching textures
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBoard);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gStitch);

	// Apply transformations to the board plane
	scale = glm::scale(glm::vec3(4.0f, 1.0f, 4.0f));
	rotation = glm::rotate(-45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	translation = glm::translate(glm::vec3(0.0f, 0.f, 0.f));

	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//Apply uv scaling and blending 
	uvScale = glm::vec2(10.f, 10.f);
	blendFactor = .15f;

	// Apply custom scaling to stitching texture and set blend factor
	glUniform2fv(glGetUniformLocation(gProgramId, "UvScale2"), 1, glm::value_ptr(uvScale));
	glUniform1f(glGetUniformLocation(gProgramId, "blendFactor"), blendFactor);

	// Draws the playing surface
	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Cleanup after rendering
	glBindVertexArray(0);

	/*******************************
	 *
	 *          BOARD FRAME
	 *
	 ******************************/
	
	 // Activate the shader program and enable texturing 
	glUseProgram(gProgramId);
	glUniform1i(glGetUniformLocation(gProgramId, "ubHasTexture"), GL_TRUE);

	// Lighting setup for the playing surface

	// Diffuse lighting
	glUniform3f(light1ColLoc, 0.4f, 0.4f, 0.4f); // Soft white light
	glUniform3f(light1PosLoc, 0.0f, 0.0f, 10.0f); // Overhead position
	// Specular lighting
	glUniform3f(light2ColLoc, 0.4f, 0.4f, 0.4f); // Secondary light source for soft highlights
	glUniform3f(light2PosLoc, 10.0f, 1.0f, 3.0f); // Side light position

	glUniform1f(specInt1Loc, 0.9f); // high sheen
	glUniform1f(highlghtSz1Loc, 10.f); // small highlight size 
	glUniform1f(specInt2Loc, 0.9f); 
	glUniform1f(highlghtSz2Loc, 10.f); 

	// Base wood grain texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gWoodGrain);
	glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);

	// Noise texture 
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNoise);
	glUniform1i(glGetUniformLocation(gProgramId, "uSecondTexture"), 1);

	// Bind the VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// Apply transformations to the board plane
	scale = glm::scale(glm::vec3(8.5f, 1.0f, 8.5f));
	rotation = glm::rotate(-45.f, glm::vec3(0.0, 1.0f, 0.0f));
	translation = glm::translate(glm::vec3(0.0f, -.501, 0.0f));

	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//Apply uv scaling and blending 
	//Wood texture uv scaling
	glm::vec2 uvScaleSides(1.f, .3f);
	glm::vec2 uvScaleTopBottom(1.0f, 1.0f);

	//Noise texture uv scaling
	glm::vec2 noiseUvScaleTopBottom = glm::vec2(1.0f, 1.0f);
	glm::vec2 noiseUvScaleSides = glm::vec2(10.f, .5f);

	//Apply side scaling
	glUniform2fv(glGetUniformLocation(gProgramId, "uvScale"), 1, glm::value_ptr(uvScaleSides));
	glUniform2fv(glGetUniformLocation(gProgramId, "UvScale2"), 1, glm::value_ptr(noiseUvScaleSides));
	
	//Apply blending
	blendFactor = .15f;
	glUniform1f(blendFactorLocation, blendFactor);

	// Apply custom scaling to stitching texture and set blend factor
	glUniform2fv(glGetUniformLocation(gProgramId, "UvScale2"), 1, glm::value_ptr(uvScale));
	glUniform1f(glGetUniformLocation(gProgramId, "blendFactor"), blendFactor);

	// Draw the sides
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 20, 4);

	//Set uv scaling for top and bottom
	glUniform2fv(glGetUniformLocation(gProgramId, "uvScale"), 1, glm::value_ptr(uvScaleTopBottom));
	glUniform2fv(glGetUniformLocation(gProgramId, "UvScale2"), 1, glm::value_ptr(noiseUvScaleTopBottom));

	//Draw top and bottom
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4);

	// Cleanup 
	glUniform1f(blendFactorLocation, defaultBlendFactor); //resets blending
	glBindVertexArray(0);


	/*******************************
	 *
	 *          Hotels
	 *
	 ******************************/

	/****** HOTEL COMMON PROPERTIES *******/

	// Activate shader program and enable texturing
	glUseProgram(gProgramId);
	glUniform1i(glGetUniformLocation(gProgramId, "ubHasTexture"), GL_TRUE);

	// Diffuse lighting for soft illumination
	glUniform3f(light1ColLoc, .3f, .6f, .6f); // moderate lighting with some red removed to prevent intense color
	glUniform3f(light1PosLoc, 0.0f, 0.0f, 10.0f); // Overhead position
	
	// Specular lighting for a slight glossy appearance
	glUniform3f(light2ColLoc, 0.3f, 0.6f, 0.6f); // moderate lighting with some red removed to prevent intense color
	glUniform3f(light2PosLoc, 10.0f, 0.0f, 20.f); // Side light position
	glUniform1f(specInt1Loc, 0.8f); // Specular intensity for light source 1
	glUniform1f(highlghtSz1Loc, 10.f); // Highlight size for a focused effect
	glUniform1f(specInt2Loc, .8f); // Specular intensity for light source 2
	glUniform1f(highlghtSz2Loc, 10.f); // Highlight size for a focused effect
	
	// Set texture and bind VBO
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gRedWoodGrain);
	glBindVertexArray(meshes.gBoxMesh.vao);

	std::vector<glm::mat4> modelMatrices; //list of hotel transformations
	
	/****** HOTEL BASE BOX *******/

	// Transformations for the base
	scale = glm::scale(glm::vec3(.3f, .25f, .25f));
	rotation = glm::rotate(10.f, glm::vec3(0.0, 5.0f, 0.0f));
	translation = glm::translate(glm::vec3(-.6f, 0.11f, 3.97f));

	//middle hotel
	model = translation * rotation * scale;
	modelMatrices.push_back(model);

	//right hotel
	translation = glm::translate(glm::vec3(.5f, 0.11f, 3.3f));

	model = translation * rotation * scale;
	modelMatrices.push_back(model);

	//left hotel
	scale = glm::scale(glm::vec3(.25f, .25f, .3f));
	rotation = glm::rotate(glm::radians(33.f), glm::vec3(0.0, 1.f, 0.0f));
	translation = glm::translate(glm::vec3(-2.96f, 0.11f, 1.05f));

	model = translation * rotation * scale;
	modelMatrices.push_back(model);

	for (const auto& modelMatrix : modelMatrices) {
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
		// Draw base
		glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
	}

	modelMatrices.clear(); //clear models list

	/****** HOTEL OVERHANG BOX *******/

	//middle hotel
	scale = glm::scale(glm::vec3(.3f, .05f, .3f));
	rotation = glm::rotate(10.f, glm::vec3(0.0, 5.0f, 0.0f));
	translation = glm::translate(glm::vec3(.5f, .23f, 3.3f));

	model = translation * rotation * scale;
	modelMatrices.push_back(model);

	//right hotel
	translation = glm::translate(glm::vec3(-.6f, .23f, 3.97f));

	model = translation * rotation * scale;
	modelMatrices.push_back(model);

	//left hotel
	rotation = glm::rotate(glm::radians(33.f), glm::vec3(0.0, 1.f, 0.0f));
	translation = glm::translate(glm::vec3(-2.96f, .23f, 1.05f));

	model = translation * rotation * scale;
	modelMatrices.push_back(model);

	for (const auto& modelMatrix : modelMatrices) {
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
		// Draw overhangs
		glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
	}
	
	modelMatrices.clear();//clear models list
	glBindVertexArray(0);

	/****** HOTEL ROOF PRISM *******/
	// Activate the VBO
	glBindVertexArray(meshes.gPrismMesh.vao);

	//middle hotel
	scale = glm::scale(glm::vec3(.3f, .3f, .1f));
	rotation1 = glm::rotate(glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.0f));
	rotation2 = glm::rotate(glm::radians((303.f)), glm::vec3(0.f, 0.f, 1.0f));
	combinedRotation = rotation1 * rotation2;
	translation = glm::translate(glm::vec3(.5f, .305f, 3.3f));

	model = translation * combinedRotation * scale;

	modelMatrices.push_back(model);

	//right hotel
	translation = glm::translate(glm::vec3(-.6f, .305f, 3.97f));
	model = translation * combinedRotation * scale;

	modelMatrices.push_back(model);

	//left hotel

	rotation1 = glm::rotate(glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.0f));
	rotation2 = glm::rotate(glm::radians(-147.f), glm::vec3(0.f, 0.f, 1.0f));
	translation = glm::translate(glm::vec3(-2.96, .305f, 1.05f));
	combinedRotation = rotation1 * rotation2;

	model = translation * combinedRotation * scale;
	modelMatrices.push_back(model);

	for (const auto& modelMatrix : modelMatrices) {
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
		// Draw roofs
		glDrawArrays(GL_TRIANGLE_STRIP, 0, meshes.gPrismMesh.nVertices);
	}

	modelMatrices.clear();//clear models list

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);
	
	/*******************************
	 *
	 *          Houses
	 *
	 ******************************/

	 /****** HOUSE COMMON PROPERTIES *******/

	// Activate shader program and enable texturing
	glUseProgram(gProgramId);
	glUniform1i(glGetUniformLocation(gProgramId, "ubHasTexture"), GL_TRUE);

	// Set texture for the houses
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gGreenWoodGrain);

	// Diffuse and specular lighting setup for houses
	glUniform3f(light1ColLoc, .3f, .3f, .3f); // soft white lighting
	glUniform3f(light1PosLoc, 0.0f, 0.0f, 10.0f); // Overhead position

	// Specular lighting for a slight glossy appearance
	glUniform3f(light2ColLoc, 0.6f, 0.6f, 0.6f); // moderate lighting 
	glUniform3f(light2PosLoc, 10.0f, 0.0f, 20.f); // Side light position
	glUniform1f(specInt1Loc, 0.8f); // Specular intensity for light source 1
	glUniform1f(highlghtSz1Loc, 10.f); // Highlight size for a focused effect
	glUniform1f(specInt2Loc, .8f); // Specular intensity for light source 2
	glUniform1f(highlghtSz2Loc, 10.f); // Highlight size for a focused effect

	glBindVertexArray(meshes.gBoxMesh.vao);

	/****** HOUSE BASE BOX *******/

	//right house
	scale = glm::scale(glm::vec3(.25f, .05f, .2f));
	rotation = glm::rotate(glm::radians(33.f), glm::vec3(0.0, 1.f, 0.0f));
	translation = glm::translate(glm::vec3(-1.2f, 0.13f, 3.9f));

	model = translation * rotation * scale;
	modelMatrices.push_back(model);

	//left house
	translation = glm::translate(glm::vec3(-1.91f, 0.13f, 2.75f));

	model = translation * rotation * scale;
	modelMatrices.push_back(model);


	for (const auto& modelMatrix : modelMatrices) {
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
		// Draw base
		glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
	}

	modelMatrices.clear(); //clear models list

	/****** HOUSE OVERHANGS *******/

	//right house
	scale = glm::scale(glm::vec3(.2f, .15f, .2f));
	rotation = glm::rotate(glm::radians(33.f), glm::vec3(0.0, 1.f, 0.0f));
	translation = glm::translate(glm::vec3(-1.2f, 0.08f, 3.9f));

	model = translation * rotation * scale;
	modelMatrices.push_back(model);

	//left house
	translation = glm::translate(glm::vec3(-1.91f, 0.08f, 2.75f));

	model = translation * rotation * scale;
	modelMatrices.push_back(model);


	for (const auto& modelMatrix : modelMatrices) {
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
		// Draw base
		glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);
	}

	modelMatrices.clear(); //clear models list

	glBindVertexArray(0);

	/****** HOTEL ROOF PRISM *******/
	
	// Activate the VBO
	glBindVertexArray(meshes.gPrismMesh.vao);

	//right house
	scale = glm::scale(glm::vec3(.25f, .2f, -.1f));
	rotation1 = glm::rotate(glm::radians(90.f), glm::vec3(1.f, 0.f, 0.0f));
	rotation2 = glm::rotate(glm::radians(-33.f), glm::vec3(0.f, 0.f, 1.0f));
	combinedRotation = rotation1 * rotation2;
	translation = glm::translate(glm::vec3(-1.2f, .2045f, 3.9f));

	model = translation * combinedRotation * scale;
	modelMatrices.push_back(model);

	//left house
	translation = glm::translate(glm::vec3(-1.91f, .2045f, 2.75f));
	model = translation * combinedRotation * scale;

	modelMatrices.push_back(model);


	for (const auto& modelMatrix : modelMatrices) {
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
		// Draw roofs
		glDrawArrays(GL_TRIANGLE_STRIP, 0, meshes.gPrismMesh.nVertices);
	}

	modelMatrices.clear();//clear models list

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	glfwSwapBuffers(gWindow);
}

// Function to render a single money denomination
void renderMoneyDenomination(GLuint texture, glm::vec3 translation, float rotationAngle, GLint modelLoc) {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gPaper); // Use paper texture for blending

	// Set blend factor for texture blending
	float blendFactor = 0.15f; // Blend with paper texture for a used look
	glUniform1f(glGetUniformLocation(gProgramId, "blendFactor"), blendFactor);

	// Apply transformations
	glm::mat4 model = glm::translate(translation) * glm::rotate(glm::radians(rotationAngle), glm::vec3(0.f, 1.f, 0.f)) * glm::scale(glm::vec3(2.1f, .002f, 1.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draw the denomination
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4); // Top part of the money
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4); // Bottom part of the money
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
	// Compilation and linkage error reporting
	int success = 0;
	char infoLog[512];

	// Create a Shader program object.
	programId = glCreateProgram();

	// Create the vertex and fragment shader objects
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	// Retrive the shader source
	glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

	// Compile the vertex shader, and print compilation errors (if any)
	glCompileShader(vertexShaderId); // compile the vertex shader

	// check for shader compile errors
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glCompileShader(fragmentShaderId); // compile the fragment shader
	// check for shader compile errors
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	// Attached compiled shaders to the shader program
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	glLinkProgram(programId);   // links the shader program
	// check for linking errors
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glUseProgram(programId);    // Uses the shader program

	return true;
}

bool UCreateTexture(const char* filename, GLuint& textureId) {
	int width, height, channels;
	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
	if (image) {
		// Successfully loaded the image
		cout << "Texture loaded successfully: " << filename << endl;
		cout << "Image width: " << width << ", height: " << height << ", channels: " << channels << endl;
		flipImageVertically(image, width, height, channels);
		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else {
			cout << "Not implemented to handle image with " << channels << " channels" << endl;
			return false;
		}
		glGenerateMipmap(GL_TEXTURE_2D);
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0);
		return true;
	}
	else {
		// Failed to load the image
		cout << "Failed to load texture: " << filename << endl;
		return false;
	}

}

void UDestroyTexture(GLuint& textureId) {
	glDeleteTextures(1, &textureId);
	textureId = 0;
}


void UDestroyShaderProgram(GLuint programId)
{
	glDeleteProgram(programId);
}