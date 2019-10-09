#include "ResourceMesh.h"
#include "OpenGL.h"

#include "Assimp/include/cimport.h"
#include "Assimp/include/scene.h"
#include "Assimp/include/postprocess.h"
#include "Assimp/include/cfileio.h"

ResourceMesh::ResourceMesh() : Resource(Resource::ResourceType::mesh)
{
}

ResourceMesh::~ResourceMesh()
{
}

void ResourceMesh::ImportMesh(aiMesh* mesh)
{
	// --- Vertices ---
	this->verticesSize = mesh->mNumVertices;
	this->Vertices = new float3[mesh->mNumVertices];

	for (unsigned j = 0; j < mesh->mNumVertices; ++j)
	{
		this->Vertices[j] = *((float3*)&mesh->mVertices[j]);
	}

	glGenBuffers(1, (GLuint*)&this->VerticesID); // create buffer
	glBindBuffer(GL_ARRAY_BUFFER, this->VerticesID); // start using created buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(float3) * this->verticesSize, this->Vertices, GL_STATIC_DRAW); // send vertices to VRAM
	glBindBuffer(GL_ARRAY_BUFFER, 0); // Stop using buffer

	// --- Normals ---
	if (mesh->HasNormals())
	{
		Normals = new float3[mesh->mNumVertices];
		memcpy(Normals, mesh->mNormals, sizeof(float3)*mesh->mNumVertices);

		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float3)*verticesSize, Normals);
	}

	// --- Texture Coordinates ---

	if (mesh->HasTextureCoords(0))
	{
		TexCoords = new float2[mesh->mNumVertices];

		for (unsigned i = 0; i < mesh->mNumVertices; ++i)
		{
			TexCoords[i] = float2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
		}

		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float2)*verticesSize, TexCoords);
	}


	// --- Indices ---
	this->IndicesSize = mesh->mNumFaces * 3;
	this->Indices = new uint[this->IndicesSize];

	for (unsigned j = 0; j < mesh->mNumFaces; ++j)
	{
		const aiFace& face = mesh->mFaces[j];

		assert(face.mNumIndices == 3); // Only triangles

		this->Indices[j * 3] = face.mIndices[0];
		this->Indices[j * 3 + 1] = face.mIndices[1];
		this->Indices[j * 3 + 2] = face.mIndices[2];
	}

	glGenBuffers(1, (GLuint*)&this->IndicesID); // create buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->IndicesID); // start using created buffer
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * this->IndicesSize, this->Indices, GL_STATIC_DRAW); // send vertices to VRAM
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Stop using buffer
}