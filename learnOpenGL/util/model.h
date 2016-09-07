#pragma once
#include "mesh.h"

class Model
{
public :

	Model(GLchar* path)
	{
		this->loadModel(path);
	}

	void Draw(Shader shader);

private :
	vector<Mesh> meshes;
	vector<Texture> textures_loaded;
	string directory;

	void loadModel(string path);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName);
};