///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


 /***********************************************************
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/
		
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3; //Less dull highlights
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);


	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.4f); // Slightly darker base color
	plasticMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f); // Minimal reflection
	plasticMaterial.shininess = 0.02f; // Very dull highlights
	plasticMaterial.tag = "plastic";

	m_objectMaterials.push_back(plasticMaterial);



}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

	 // Enable lighting
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Ambient light for global illumination (soft, subtle light for realism)
	m_pShaderManager->setVec3Value("ambientLight", glm::vec3(0.2f, 0.2f, 0.2f)); // Dim global light

	// Directional light simulating sunlight coming from an angle
	m_pShaderManager->setVec3Value("directionalLight.direction", glm::vec3(-0.3f, -1.0f, -0.5f));
	m_pShaderManager->setVec3Value("directionalLight.ambient", glm::vec3(0.2f, 0.2f, 0.2f)); // Slight ambient
	m_pShaderManager->setVec3Value("directionalLight.diffuse", glm::vec3(0.8f, 0.8f, 0.8f)); // Brighter diffuse light
	m_pShaderManager->setVec3Value("directionalLight.specular", glm::vec3(1.0f, 1.0f, 1.0f)); // Specular highlights
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Spotlight focused on the center of the table
	m_pShaderManager->setVec3Value("spotLight.position", glm::vec3(0.0f, 10.0f, 5.0f)); // Above the table
	m_pShaderManager->setVec3Value("spotLight.direction", glm::vec3(0.0f, -1.0f, 0.0f)); // Downward
	m_pShaderManager->setVec3Value("spotLight.ambient", glm::vec3(0.1f, 0.1f, 0.1f)); // Soft ambient
	m_pShaderManager->setVec3Value("spotLight.diffuse", glm::vec3(1.0f, 0.9f, 0.8f)); // Warm light
	m_pShaderManager->setVec3Value("spotLight.specular", glm::vec3(1.0f, 1.0f, 1.0f)); // Highlights
	m_pShaderManager->setFloatValue("spotLight.cutOff", glm::cos(glm::radians(10.0f))); // Tight spotlight
	m_pShaderManager->setFloatValue("spotLight.outerCutOff", glm::cos(glm::radians(30.0f))); // Soft edges
	m_pShaderManager->setFloatValue("spotLight.constant", 1.0f);
	m_pShaderManager->setFloatValue("spotLight.linear", 0.09f);
	m_pShaderManager->setFloatValue("spotLight.quadratic", 0.032f);
	m_pShaderManager->setBoolValue("spotLight.bActive", true);

	// Point Light 1: Above Coffee Mug, angled toward it
	m_pShaderManager->setVec3Value("pointLights[0].position", glm::vec3(-11.0f, 8.0f, 6.0f)); // Offset to point toward the mug
	m_pShaderManager->setVec3Value("pointLights[0].ambient", glm::vec3(0.05f, 0.05f, 0.05f));
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
	m_pShaderManager->setVec3Value("pointLights[0].specular", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.14f);
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.07f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Point Light 2: Above Box Item, angled toward it
	m_pShaderManager->setVec3Value("pointLights[1].position", glm::vec3(-6.0f, 8.0f, 6.5f)); // Offset to point toward the box
	m_pShaderManager->setVec3Value("pointLights[1].ambient", glm::vec3(0.05f, 0.05f, 0.05f));
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
	m_pShaderManager->setVec3Value("pointLights[1].specular", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.14f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.07f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// Point Light 3: Above Trashcan, angled toward it
	m_pShaderManager->setVec3Value("pointLights[2].position", glm::vec3(3.0f, 8.0f, 4.0f)); // Offset to point toward the trashcan
	m_pShaderManager->setVec3Value("pointLights[2].ambient", glm::vec3(0.05f, 0.05f, 0.05f));
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
	m_pShaderManager->setVec3Value("pointLights[2].specular", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setFloatValue("pointLights[2].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[2].linear", 0.14f);
	m_pShaderManager->setFloatValue("pointLights[2].quadratic", 0.07f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);

	// Point Light 4: Above Wooden Plank, angled toward it
	m_pShaderManager->setVec3Value("pointLights[3].position", glm::vec3(9.0f, 8.0f, 6.0f)); // Offset to point toward the plank
	m_pShaderManager->setVec3Value("pointLights[3].ambient", glm::vec3(0.05f, 0.05f, 0.05f));
	m_pShaderManager->setVec3Value("pointLights[3].diffuse", glm::vec3(0.8f, 0.8f, 0.8f));
	m_pShaderManager->setVec3Value("pointLights[3].specular", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setFloatValue("pointLights[3].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[3].linear", 0.14f);
	m_pShaderManager->setFloatValue("pointLights[3].quadratic", 0.07f);
	m_pShaderManager->setBoolValue("pointLights[3].bActive", true);

}

void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	bool bReturn = false;
	bReturn = CreateGLTexture("textures/black.jpg", "mugbase"); //Flat black color texture
	bReturn = CreateGLTexture("textures/teachflag.jpg", "flag"); //Black background with imposed image for mugh
	bReturn = CreateGLTexture("textures/Wood066_2K-JPG_Color.jpg", "table"); //Wood plank to replicate table top 
	bReturn = CreateGLTexture("textures/Wood048_1K-JPG_Color.jpg", "plank"); //Base wood texture
	bReturn = CreateGLTexture("textures/Plastic010_1K-JPG_Color.jpg", "plastic"); //Light blue plastic appearing texture
	bReturn = CreateGLTexture("textures/office.jpg", "office"); //Black background with imposed image to replicate desk decour

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();
	// load the textures for the 3D scene
	LoadSceneTextures();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();
}

//Table Top Creates Surface for objects and called in RenderScence
void SceneManager::RenderTableTop() {

	//Table top
	/*******************************************************/
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(30.0f, 1.0f, 15.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -0.5f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("table"); //provide wood like texture to replicate image
	SetShaderMaterial("wood"); //texture reflects wood in a similar way it would in real life

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

}
//Funciton combines Cylinder and Toros for coffee mug called in RenderScene
void SceneManager::RenderMug() {

	//Coffee Mug Body

	// set the XYZ scale for the mesh
	glm::vec3 scaleXYZ = glm::vec3(1.0f, 2.8f, 1.0f);

	// set the XYZ rotation for the mesh
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 120.0f;
	float ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	glm::vec3 positionXYZ = glm::vec3(-10.2f, 0.1f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color for the next draw command
	// Set the texture for the coffee mug
	SetShaderTexture("flag");
	// Set UV scale for tiling
	SetTextureUVScale(0.5f, 0.5f);
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh(false, false, true); //place texture on side of cylinder

	SetShaderTexture("mugbase");
	// Set UV scale for tiling
	SetTextureUVScale(0.5f, 0.5f);
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh(false, true, false); //place texture on the bottom of cylinder leaving the top open

	//Mug Handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.6f, 0.9f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-9.0f, 1.5f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color for the next draw command
	SetShaderTexture("mugbase");
	SetShaderMaterial("wood");
	// draw the filled cylinder shape
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/

}
//Function creates office desk item and called in RenderScene
void SceneManager::RenderOffieDeskItem() {

	//Office Desk Box Item
	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	glm::vec3 scaleXYZ = glm::vec3(6.0f, 5.0f, 0.5f);

	// set the XYZ rotation for the mesh
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	glm::vec3 positionXYZ = glm::vec3(-4.5f, 2.5f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color for the next draw command
	SetShaderTexture("office");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::front); //Place office image on front of item leaving the sides blank

	SetShaderTexture("mugbase");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");
	m_basicMeshes->DrawBoxMesh(); //Wrap the rest of the object in a flat black texture
	/****************************************************************/

}
//Funciton Creates trashcan combining half sphere and cylinder called in RenderScene
void SceneManager::RenderTrashCan() {
	//Trashcan base
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh

	glm::vec3 scaleXYZ = glm::vec3(2.53f, 1.0f, 2.53f);

	// set the XYZ rotation for the mesh
	float XrotationDegrees = 180.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	glm::vec3 positionXYZ = glm::vec3(4.5f, 1.0f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color for the next draw command
	// draw the filled sphere shape
	//Wrap half sphere base to give rounded bottom appearance
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawHalfSphereMesh();
	/****************************************************************/


	/******************************************************************/
	//Trashcan Outer Cylinder
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.5f, 5.0f, 2.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.5f, 0.97f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color for the next draw command
	//Wrap the side of the cylinder in a plastic like texture appearance and color
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("plastic");
	// draw the filled cylinder shape
	m_basicMeshes->DrawCylinderMesh(false, false, true); //Only creates side of the cylinder to show sides of trashcan and leave an open top
	/******************************************************************/

}
//Creates Scrap wood and called in RenderScene
void SceneManager::RenderScrapWood() {

	//Piece of wood
	/****************************************************************/
	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	glm::vec3 scaleXYZ = glm::vec3(1.0f, 7.0f, 1.0f);

	// set the XYZ rotation for the mesh
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	glm::vec3 positionXYZ = glm::vec3(10.0f, 3.5f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);


	// set the color for the next draw command
	//SetShaderColor(0.8f, 0.5f, 0.3f, 1.0f);
	//Wraps all sides of the plank of wood like in real life
	SetShaderTexture("plank");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");
	// draw the filled cylinder shape
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

}
/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	RenderTableTop();
	RenderMug();
	RenderOffieDeskItem();
	RenderTrashCan();
	RenderScrapWood();
}




