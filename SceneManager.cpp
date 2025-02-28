///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// REVISED BY: Taylor Stapus
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
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
SceneManager::SceneManager(ShaderManager *pShaderManager)
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
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	goldMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	goldMaterial.shininess = 70.0;
	goldMaterial.tag = "metal";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);
	woodMaterial.shininess = 40.0;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	glassMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glassMaterial.shininess = 95.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL clayMaterial;
	clayMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	clayMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	clayMaterial.shininess = 40.0;
	clayMaterial.tag = "vase";

	m_objectMaterials.push_back(clayMaterial); 

	OBJECT_MATERIAL backdropMaterial;
	backdropMaterial.diffuseColor = glm::vec3(0.8f, 0.8f, 0.9f);
	backdropMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	backdropMaterial.shininess = 2.0;
	backdropMaterial.tag = "wall";

	m_objectMaterials.push_back(backdropMaterial);

	OBJECT_MATERIAL leafMaterial;
	leafMaterial.diffuseColor = glm::vec3(0.4f, 0.2f, 0.4f);
	leafMaterial.specularColor = glm::vec3(0.1f, 0.05f, 0.1f);
	leafMaterial.shininess = 0.30;
	leafMaterial.tag = "leaf";

	m_objectMaterials.push_back(leafMaterial);

	OBJECT_MATERIAL paperMaterial;
	paperMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	paperMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	paperMaterial.shininess = 1.0;
	paperMaterial.tag = "paper";

	m_objectMaterials.push_back(paperMaterial);

	OBJECT_MATERIAL fabricMaterial;
	fabricMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	fabricMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	fabricMaterial.shininess = 1.0;
	fabricMaterial.tag = "fabric";

	m_objectMaterials.push_back(fabricMaterial);

}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable lighting in the shader
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Directional light setup
	m_pShaderManager->setVec3Value("directionalLight.direction", -7.0f, 10.0f, -10.0f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.7f, 0.7f, 0.7f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Point light 1 over ottoman
	m_pShaderManager->setVec3Value("pointLights[0].position", 14.0f, 35.0f, 5.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.08f, 0.08f, 0.08f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Point light 1 over bookshelf
	m_pShaderManager->setVec3Value("pointLights[1].position", 14.0f, 35.0f, -17.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.08f, 0.08f, 0.08f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.4f, 0.4f, 0.4f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// Point light in lamp
	m_pShaderManager->setVec3Value("pointLights[2].position", -2.0f, 13.0f, -17.0f);
	m_pShaderManager->setVec3Value("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[2].diffuse", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("pointLights[2].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("pointLights[2].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[2].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[2].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[2].bActive", true);

	// Point light in lamp
	m_pShaderManager->setVec3Value("pointLights[3].position", -2.0f, 13.0f, -15.0f);
	m_pShaderManager->setVec3Value("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[3].diffuse", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("pointLights[3].specular", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setFloatValue("pointLights[3].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[3].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[3].quadratic", 0.032f);
	m_pShaderManager->setBoolValue("pointLights[3].bActive", true);

	
}



 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/leaf.jpg",
		"leaf");

	bReturn = CreateGLTexture(
		"textures/vase.jpg",
		"vase");

	bReturn = CreateGLTexture(
		"textures/floor.jpg",
		"floor");

	bReturn = CreateGLTexture(
		"textures/wall.jpg",
		"wall");

	bReturn = CreateGLTexture(
		"textures/ottoman.jpg",
		"ottoman");

	bReturn = CreateGLTexture(
		"textures/pillow.jpg",
		"pillow");

	bReturn = CreateGLTexture(
		"textures/bookshelf.jpg",
		"bookshelf");

	bReturn = CreateGLTexture(
		"textures/picture.jpg",
		"picture");

	bReturn = CreateGLTexture(
		"textures/rug.jpg",
		"rug");

	bReturn = CreateGLTexture(
		"textures/lamp_bot.jpg",
		"lamp_bot");
	
	bReturn = CreateGLTexture(
		"textures/lamp_top.jpg",
		"lamp_top");

	bReturn = CreateGLTexture(
		"textures/books.jpg",
		"books");

	bReturn = CreateGLTexture(
		"textures/book2.jpg",
		"book2");

	bReturn = CreateGLTexture(
		"textures/snowglobe_bot.jpg",
		"snowglobe_bot");


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
	// load the textures for the 3D scene
	LoadSceneTextures();
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
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
/******************************************************************/
	//FLOOR
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 20.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("floor");
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


/****************************************************************/
/******************************************************************/
	//WALL 1
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 20.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(20.0f, 20.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("wall");
	SetShaderMaterial("wall");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

/****************************************************************/
/******************************************************************/
	//WALL 2
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 20.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 20.0f, -20.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("wall");
	SetShaderMaterial("wall");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

/****************************************************************/
/******************************************************************/
	//LEAF 1
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 1.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 45.0f;
	YrotationDegrees = -90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.5f, 3.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);
	SetShaderTexture("leaf");
	SetShaderMaterial("leaf");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPyramid4Mesh();

/****************************************************************/
/******************************************************************/
	//LEAF 2
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 1.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -45.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.0f, -0.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);
	SetShaderTexture("leaf");
	SetShaderMaterial("leaf");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPyramid4Mesh();

/****************************************************************/	
/******************************************************************/
	//LEAF 3
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 1.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 45.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.0f, 0.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);
	SetShaderTexture("leaf");
	SetShaderMaterial("leaf");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPyramid4Mesh();

/****************************************************************/
/******************************************************************/
	//LEAF 4
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 1.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -45.0f;
	YrotationDegrees = -90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.5f, 3.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);
	SetShaderTexture("leaf");
	SetShaderMaterial("leaf");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPyramid4Mesh();
	
/****************************************************************/
/******************************************************************/
	//LEAF BASE
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 1.5f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 3.2f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);
	SetShaderTexture("leaf");
	SetShaderMaterial("leaf");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPyramid4Mesh();

/****************************************************************/
/******************************************************************/
	//VASE BASE
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.5f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 2.4f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 0.9, 0.9, 0.9);
	SetShaderTexture("vase");
	SetShaderMaterial("vase");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

/****************************************************************/
/******************************************************************/
	//OTTOMAN
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(6.0f, 5.0f, 6.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(14.0f, 0.0f, 5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 0.9, 0.9, 0.9);
	SetShaderTexture("ottoman");
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

/******************************************************************/
/******************************************************************/
	//PILLOW 1
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 1.0f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -75.0f;
	YrotationDegrees = 120.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(16.0f, 7.5f, 3.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);
	SetShaderTexture("pillow");
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

/******************************************************************/
/******************************************************************/
	//PILLOW 2
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(5.0f, 1.0f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = -20.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(18.0f, 7.5f, 6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);
	SetShaderTexture("pillow");
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

/******************************************************************/
/******************************************************************/
	//BOOK SHELF BACK
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(15.0f, 0.5f, 20.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.8f, 10.0f, -20.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);
	SetShaderTexture("bookshelf");
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

/******************************************************************/
/******************************************************************/
	//BOOK SHELF MIDDLE
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.0f, 0.5f, 15.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.8f, 10.0f, -18.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);
	SetShaderTexture("bookshelf");
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

/******************************************************************/
/******************************************************************/
//BOOK SHELF UPPER SHELF
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.0f, 0.5f, 15.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.8f, 15.0f, -18.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);
	SetShaderTexture("bookshelf");
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

/******************************************************************/
/******************************************************************/
//BOOK SHELF LOWER SHELF
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.0f, 0.5f, 15.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.8f, 5.0f, -18.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);
	SetShaderTexture("bookshelf");
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

/******************************************************************/
/******************************************************************/
//BOOK SHELF TOP
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.0f, 0.5f, 15.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.8f, 20.0f, -18.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);
	SetShaderTexture("bookshelf");
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

/******************************************************************/
/******************************************************************/
//BOOK SHELF BOTTOM
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.0f, 0.5f, 15.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(10.8f, 0.0f, -18.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);
	SetShaderTexture("bookshelf");
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

/******************************************************************/
/******************************************************************/
//BOOK SHELF LEFT
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.0f, 0.5f, 20.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.4f, 10.0f, -18.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);
	SetShaderTexture("bookshelf");
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

/******************************************************************/
/******************************************************************/
//BOOK SHELF RIGHT
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(7.0f, 0.5f, 20.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(18.4f, 10.0f, -18.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);
	SetShaderTexture("bookshelf");
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

/******************************************************************/
/******************************************************************/
	//PICTURE
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(8.0f, 0.5f, 11.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(19.8f, 20.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);
	SetShaderTexture("picture");
	SetShaderMaterial("paper");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
/******************************************************************/
/******************************************************************/

	//RUG
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 0.3f, 15.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5, 0.5, 0.5, 0.5);
	SetShaderTexture("rug");
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

/******************************************************************/
/******************************************************************/
	//LAMP TOP
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 3.5f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 13.0f, -17.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.3, 0.3, 0.3, 0.3);
	SetShaderTexture("lamp_top");
	SetShaderMaterial("paper");


	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

/******************************************************************/
/******************************************************************/
	//LAMP BOT
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 13.0f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.5f, -17.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 0.9, 0.9, 0.9);
	SetShaderTexture("lamp_bot");
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

/******************************************************************/
/******************************************************************/
	//LAMP BOT
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 0.5f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-2.0f, 0.0f, -17.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 0.9, 0.9, 0.9);
	SetShaderTexture("lamp_bot");
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

/******************************************************************/
/******************************************************************/
//BOOK 1
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 1.0f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(17.6f, 12.0f, -18.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 0.9, 0.9, 0.9);
	SetShaderTexture("books");
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


/******************************************************************/
/******************************************************************/
	//BOOK 2
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 1.0f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(16.3f, 12.5f, -18.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 0.9, 0.9, 0.9);
	SetShaderTexture("book2");
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

/******************************************************************/
/******************************************************************/
//BOOK 3
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 1.0f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(15.0f, 12.0f, -18.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 0.9, 0.9, 0.9);
	SetShaderTexture("books");
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
/******************************************************************/
/******************************************************************/
//BOOK 4
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 1.0f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.2f, 17.0f, -18.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 0.9, 0.9, 0.9);
	SetShaderTexture("books");
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

/******************************************************************/
/******************************************************************/
	//BOOK 5
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.0f, 1.0f, 5.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.4f, 17.4f, -18.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 0.9, 0.9, 0.9);
	SetShaderTexture("book2");
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

/******************************************************************/
/******************************************************************/
//BOOK 6
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.9f, 1.0f, 3.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 20.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.0f, 17.1f, -18.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 0.9, 0.9, 0.9);
	SetShaderTexture("books");
	SetShaderMaterial("fabric");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

/******************************************************************/
/******************************************************************/
	//SNOWGLOBE BOTTOM
// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.0f, 5.0f, -17.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 0.9, 0.9, 0.9);
	SetShaderTexture("snowglobe_bot");
	SetShaderMaterial("metal");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

/******************************************************************/
/******************************************************************/
		//SNOWGLOBE Top
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.9f, 0.9, 0.9f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.0f, 6.7f, -17.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.9, 0.9, 0.9, 0.9);
	SetShaderTexture("lamp_bot");
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
}



