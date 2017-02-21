#include "stdfx.h"
#include "util.h"
#include "model.h"
using namespace std;

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 800;

glm::vec3 eye = glm::vec3(0, 0, 3);
glm::vec3 up = glm::vec3(0, 1, 0);
glm::vec3 front = glm::vec3(0, 0, -1);
glm::vec3 vec_right = glm::vec3(1, 0, 0);
int lightMode = 0;

#define PI 3.141692653
#define KEY_W 0 
#define KEY_S 1
#define KEY_A 2
#define KEY_D 3
bool keys[4];

void do_movement();
// The MAIN function, from here we start the application and run the game loop
int main()
{
	/********************************************************************/
	//Init
	/********************************************************************/
	std::cout << "Starting GLFW context, OpenGL 3.3" << std::endl;
	// Init GLFW
	glfwInit();
	// Set all the required options for GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Set the required callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
	glewExperimental = GL_TRUE;
	// Initialize GLEW to setup the OpenGL Function pointers
	glewInit();

	glEnable(GL_DEPTH_TEST);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	/********************************************************************/
	//shader
	/********************************************************************/
	string currentProject = "deferredShading";

	Shader shader((currentProject + "\\shader\\vs.vs").c_str(), (currentProject + "\\shader\\fs.fs").c_str());
	Shader lightShader((currentProject + "\\shader\\lightVs.vs").c_str(), (currentProject + "\\shader\\lightFs.fs").c_str());
	Shader lightingShader((currentProject + "\\shader\\lighting.vs").c_str(), (currentProject + "\\shader\\lighting.fs").c_str());
	Shader triangleShader((currentProject + "\\shader\\triangleVs.vs").c_str(), (currentProject + "\\shader\\triangleFs.fs").c_str());

	lightingShader.Use();

	//light uniform
	// - Colors
	const GLuint NR_LIGHTS = 32;
	std::vector<glm::vec3> lightOffset;
	std::vector<glm::mat4> lightModels;
	std::vector<glm::vec3> lightColors;
	srand(13);
	for (GLuint i = 0; i < NR_LIGHTS; i++)
	{
		// Calculate slightly random offsets
		GLfloat xPos = ((rand() % 100) / 100.0) * 6.0 - 3.0;
		GLfloat yPos = ((rand() % 100) / 100.0) * 6.0 - 4.0;
		GLfloat zPos = ((rand() % 100) / 100.0) * 6.0 - 3.0;
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(xPos, yPos, zPos));
		model = glm::scale(model, glm::vec3(0.125, 0.125, 0.125));
		lightModels.push_back(model);
		lightOffset.push_back(glm::vec3(xPos, yPos, zPos));
		// Also calculate random color
		GLfloat rColor = ((rand() % 100) / 200.0f) + 0.5; // Between 0.5 and 1.0
		GLfloat gColor = ((rand() % 100) / 200.0f) + 0.5; // Between 0.5 and 1.0
		GLfloat bColor = ((rand() % 100) / 200.0f) + 0.5; // Between 0.5 and 1.0
		lightColors.push_back(glm::vec3(rColor, gColor, bColor));
	}

	for (int i = 0; i < NR_LIGHTS; ++i)
	{
		GLint lightDirectionLocation = glGetUniformLocation(lightingShader.Program, ("light["+to_string(i)+"].direction").c_str());
		glUniform3f(lightDirectionLocation, -1.0f, -1.0f, 1.0f);
		GLint lightAmbientLocation = glGetUniformLocation(lightingShader.Program, ("light[" + to_string(i) + "].ambient").c_str());
		glUniform3f(lightAmbientLocation, 0.2f, 0.2f, 0.2f);
		GLint lightColorLocation = glGetUniformLocation(lightingShader.Program, ("light[" + to_string(i) + "].color").c_str());
		glUniform3f(lightColorLocation, lightColors[i][0], lightColors[i][1], lightColors[i][2]);
		GLint lightPositionLocation = glGetUniformLocation(lightingShader.Program, ("light[" + to_string(i) + "].position").c_str());
		glUniform4f(lightPositionLocation, lightOffset[i][0], lightOffset[i][1], lightOffset[i][2], 1.0f);
		GLint lightConstantLocation = glGetUniformLocation(lightingShader.Program, ("light[" + to_string(i) + "].constant").c_str());
		glUniform1f(lightConstantLocation, 1.0f);
		GLint lightLinearLocation = glGetUniformLocation(lightingShader.Program, ("light[" + to_string(i) + "].linear").c_str());
		glUniform1f(lightLinearLocation, 0.7f);
		GLint lightQuadraticLocation = glGetUniformLocation(lightingShader.Program, ("light[" + to_string(i) + "].quadratic").c_str());
		glUniform1f(lightQuadraticLocation, 1.8f);
	}
	/********************************************************************/
	//gBuffer
	/********************************************************************/
	GLuint gBuffer;
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	GLuint gPosition, gNormal, gAlbedoSpec;

	//position
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	//normal
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, WIDTH, HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

	//normal
	glGenTextures(1, &gAlbedoSpec);
	glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);

	GLuint attachments[3] = { GL_COLOR_ATTACHMENT0 , GL_COLOR_ATTACHMENT1 , GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	//depth buffer
	GLuint depthBuffer;
	glGenRenderbuffers(1, &depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WIDTH, HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

	//complete check 
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	/********************************************************************/
	//model
	/********************************************************************/
	Model model("model//nanosuit.obj");

	// Define the viewport dimensions
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	//triengle
	GLfloat triangleVert[] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.0f, 0.5f, 0.0f
	};

	//lights
	GLfloat vertices[] = {
	// Back face
	-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // Bottom-left
	0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, // top-right
	0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
	0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,  // top-right
	-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,  // bottom-left
	-0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,// top-left
	// Front face
	-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
	0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,  // bottom-right
	0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,  // top-right
	0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, // top-right
	-0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // top-left
	-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,  // bottom-left
	// Left face
	-0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-right
	-0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-left
	-0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom-left
	-0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
	-0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // bottom-right
	-0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-right
	// Right face
	0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // top-left
	0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
	0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-right         
	0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  // bottom-right
	0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // top-left
	0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, // bottom-left     
	// Bottom face
	-0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
	0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, // top-left
	0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,// bottom-left
	0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom-left
	-0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom-right
	-0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
	// Top face
	-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,// top-left
	0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
	0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // top-right     
	0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // bottom-right
	-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,// top-left
	-0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f // bottom-left        
	};
	
	std::vector<glm::vec3> objectPositions;
	objectPositions.push_back(glm::vec3(-3.0, -3.0, -3.0));
	objectPositions.push_back(glm::vec3(0.0, -3.0, -3.0));
	objectPositions.push_back(glm::vec3(3.0, -3.0, -3.0));
	objectPositions.push_back(glm::vec3(-3.0, -3.0, 0.0));
	objectPositions.push_back(glm::vec3(0.0, -3.0, 0.0));
	objectPositions.push_back(glm::vec3(3.0, -3.0, 0.0));
	objectPositions.push_back(glm::vec3(-3.0, -3.0, 3.0));
	objectPositions.push_back(glm::vec3(0.0, -3.0, 3.0));
	objectPositions.push_back(glm::vec3(3.0, -3.0, 3.0));
	

	//quad
	glm::vec3 quadVert[] =
	{
		glm::vec3(-1, -1, 0),
		glm::vec3(-1, 1, 0),
		glm::vec3(1, 1, 0),
		glm::vec3(-1, -1, 0),
		glm::vec3(1, 1, 0),
		glm::vec3(1, -1, 0)
	}; 

	glm::vec2 quadUVs[] =
	{
		glm::vec2(0, 0),
		glm::vec2(0, 1),
		glm::vec2(1, 1),
		glm::vec2(0, 0),
		glm::vec2(1, 1),
		glm::vec2(1, 0)
	};

	GLuint quadVertexBuffer, quadUVBuffer, quadVertexBufferArray;
	glGenVertexArrays(1, &quadVertexBufferArray);
	glBindVertexArray(quadVertexBufferArray);

	glGenBuffers(1, &quadVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(glm::vec3), &quadVert[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(0));
	glEnableVertexAttribArray(0);
	
	glGenBuffers(1, &quadUVBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quadUVBuffer);
	glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(glm::vec2), &quadUVs[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(0));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	
	//light
	GLuint lightVertBuffer, lightColorBuffer, lightModelBuffer, lightVBA;
	glGenVertexArrays(1, &lightVBA);
	glBindVertexArray(lightVBA);

	glGenBuffers(1, &lightVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, lightVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, 36 * 8 * sizeof(GL_FLOAT), &vertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GL_FLOAT), (void*)0);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &lightColorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, lightColorBuffer);
	glBufferData(GL_ARRAY_BUFFER, lightColors.size() * sizeof(glm::vec3), &lightColors[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &lightModelBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, lightModelBuffer);
	glBufferData(GL_ARRAY_BUFFER, lightModels.size() * sizeof(glm::mat4), &lightModels[0][0], GL_STATIC_DRAW);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)0);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(1 * sizeof(glm::vec4)));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(2 * sizeof(glm::vec4)));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(glm::vec4), (void*)(3 * sizeof(glm::vec4)));
	glEnableVertexAttribArray(5);

	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);

	glBindVertexArray(0);

	//triangle
	GLuint triVertBuffer, triVBA;
	glGenVertexArrays(1, &triVBA);
	glBindVertexArray(triVBA);

	glGenBuffers(1, &triVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, triVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(GLfloat), &triangleVert[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	/********************************************************************/
	//camera
	/********************************************************************/

	glm::mat4 proj = glm::perspective(45.0f, 1.0f, 0.1f, 100.0f);

	glm::vec3 lightPos = glm::vec3(1.2f, 1.0f, 2.0f);
	
	/********************************************************************/
	//Main loop
	/********************************************************************/
	while (!glfwWindowShouldClose(window))
	{
		printError();
		// Check if any events have been activated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();
		do_movement();

		//geometry pass
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		
		shader.Use();
		// Render
		// Clear the color buffer
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::vec3 center = eye + front;
		glm::mat4 view = glm::lookAt(eye, center, up);

		for (int i = 0; i < 9; ++i)
		{
			glm::mat4 modelMat = glm::mat4(1.0f);
			modelMat = glm::translate(modelMat, objectPositions[i]);
			modelMat = glm::scale(modelMat, glm::vec3(0.25f));
			glm::mat4 MVP = proj * view * modelMat;
			glUniformMatrix4fv(glGetUniformLocation(shader.Program, "MVP"), 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(glGetUniformLocation(shader.Program, "Model"), 1, GL_FALSE, &modelMat[0][0]);

			model.Draw(shader);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		
		//lighting pass
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		lightingShader.Use();

		glUniform3f(glGetUniformLocation(lightingShader.Program, "eye"), eye[0], eye[1], eye[2]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glUniform1i(glGetUniformLocation(lightingShader.Program, "gPosition"), 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glUniform1i(glGetUniformLocation(lightingShader.Program, "gNormal"), 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
		glUniform1i(glGetUniformLocation(lightingShader.Program, "gAlbedoSpec"), 2);

		glClear(GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(quadVertexBufferArray);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		//copy depth buffer
		glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		
		//draw lights
		glBindVertexArray(lightVBA);
		lightShader.Use();
		glm::mat4 VP = proj * view;
		glUniformMatrix4fv(glGetUniformLocation(lightShader.Program, "VP"), 1, GL_FALSE, &VP[0][0]);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 36, NR_LIGHTS);
		glBindVertexArray(0);

		//draw triangle
		/*triangleShader.Use();
		glBindVertexArray(triVBA);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindVertexArray(0);*/

		// Swap the screen buffers
		glfwSwapBuffers(window);
	}
	// Properly de-allocate all resources once they've outlived their purpose
	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
	return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_W && action == GLFW_PRESS)
		keys[KEY_W] = true;
	else if (key == GLFW_KEY_W && action == GLFW_RELEASE)
		keys[KEY_W] = false;

	if (key == GLFW_KEY_S && action == GLFW_PRESS)
		keys[KEY_S] = true;
	else if (key == GLFW_KEY_S && action == GLFW_RELEASE)
		keys[KEY_S] = false;

	if (key == GLFW_KEY_D && action == GLFW_PRESS)
		keys[KEY_D] = true;
	else if (key == GLFW_KEY_D && action == GLFW_RELEASE)
		keys[KEY_D] = false;

	if (key == GLFW_KEY_A && action == GLFW_PRESS)
		keys[KEY_A] = true;
	else if (key == GLFW_KEY_A && action == GLFW_RELEASE)
		keys[KEY_A] = false;

	if (key == GLFW_KEY_L && action == GLFW_RELEASE)
		lightMode = (lightMode + 1) % 4;
}

float yaw = 0, pitch = 0;
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	//printf("x : %.2f, y : %.2f\n", xpos - 400, ypos - 400);
	//printf("pitch yaw:%.2f, %.2f\n", pitch, yaw);
	float x_offset = xpos - 400;
	float y_offset = ypos - 400;

	float sensity = 0.05f;
	yaw += -sensity * x_offset;
	pitch += -sensity * y_offset;
	if (pitch >= 89)
		pitch = 89;
	if (pitch <= -89)
		pitch = -89;

	glm::mat4 yawRot = glm::mat4(1.0f);
	yawRot = glm::rotate(yawRot, yaw, up);
	vec_right = glm::vec3(yawRot * glm::vec4(1, 0, 0, 0));
	glm::mat4 pitchRot = glm::mat4(1.0f);
	pitchRot = glm::rotate(pitchRot, pitch, vec_right);

	front = glm::vec3(pitchRot * yawRot * glm::vec4(0, 0, -1, 0));

	glfwSetCursorPos(window, 400, 400);
}

GLfloat cur = 0.0f, last = cur;
void do_movement()
{
	glm::vec3 right = glm::cross(front, up);
	float speed;// = 0.1f;

	cur = glfwGetTime();
	speed = 5.0f * (cur - last);
	last = cur;

	if (keys[KEY_W])
		eye += front * speed;

	if (keys[KEY_S])
		eye -= front * speed;

	if (keys[KEY_D])
		eye += right * speed;

	if (keys[KEY_A])
		eye -= right * speed;
}
