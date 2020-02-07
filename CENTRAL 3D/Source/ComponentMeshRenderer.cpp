#include "ComponentMeshRenderer.h"
#include "ComponentMesh.h"
#include "ComponentTransform.h"
#include "ComponentCamera.h"
#include "GameObject.h"
#include "OpenGL.h"
#include "Color.h"
#include "Application.h"
#include "ModuleTextures.h"
#include "ModuleSceneManager.h"
#include "ModuleRenderer3D.h"
#include "ModuleTimeManager.h"
#include "ModuleWindow.h"
#include "ModuleResources.h"

#include "ResourceMesh.h"
#include "ResourceShader.h"
#include "ResourceTexture.h"
#include "ResourceMaterial.h"

#include "mmgr/mmgr.h"

ComponentMeshRenderer::ComponentMeshRenderer(GameObject* ContainerGO): Component(ContainerGO, Component::ComponentType::MeshRenderer)
{
	material = (ResourceMaterial*)App->resources->GetResource(App->resources->GetDefaultMaterialUID());
}

ComponentMeshRenderer::~ComponentMeshRenderer()
{

}

void ComponentMeshRenderer::Draw(bool outline) const
{
	ComponentMesh * mesh = this->GO->GetComponent<ComponentMesh>();
	ComponentTransform* transform = GO->GetComponent<ComponentTransform>();
	ComponentCamera* camera = GO->GetComponent<ComponentCamera>();

	uint shader = App->renderer3D->defaultShader->ID;

	shader = material->shader->ID;

	float4x4 model = transform->GetGlobalTransform();

	if (outline)
	{
		shader = App->renderer3D->OutlineShader->ID;
		// --- Draw selected, pass scaled-up matrix to shader ---
		float3 scale = float3(1.05f, 1.05f, 1.05f);
		
		model = float4x4::FromTRS(model.TranslatePart(), model.RotatePart(), scale);
	}

	//mat->resource_material->UpdateUniforms();

	// --- Display Z buffer ---
	if (App->renderer3D->zdrawer)
	{
		shader = App->renderer3D->ZDrawerShader->ID;
	}

	glUseProgram(shader);

	// --- Set uniforms ---
	GLint modelLoc = glGetUniformLocation(shader, "model_matrix");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model.Transposed().ptr());
	
	GLint viewLoc = glGetUniformLocation(shader, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, App->renderer3D->active_camera->GetOpenGLViewMatrix().ptr());

	GLint timeLoc = glGetUniformLocation(shader, "time");
	glUniform1f(timeLoc, App->time->time);

	float farp = App->renderer3D->active_camera->GetFarPlane();
	float nearp = App->renderer3D->active_camera->GetNearPlane();
	// --- Give ZDrawer near and far camera frustum planes pos ---
	if (App->renderer3D->zdrawer)
	{
		int nearfarLoc = glGetUniformLocation(shader, "nearfar");
		glUniform2f(nearfarLoc, nearp, farp);
	}

	// right handed projection matrix
	float f = 1.0f / tan(App->renderer3D->active_camera->GetFOV()*DEGTORAD / 2.0f);
	 float4x4 proj_RH(
		f / App->renderer3D->active_camera->GetAspectRatio(), 0.0f, 0.0f, 0.0f,
		0.0f, f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, nearp, 0.0f);

	GLint projectLoc = glGetUniformLocation(shader, "projection");
	glUniformMatrix4fv(projectLoc, 1, GL_FALSE, proj_RH.ptr());


	if (mesh && mesh->resource_mesh && mesh->IsEnabled())
	{
		DrawMesh(*mesh->resource_mesh);
		DrawNormals(*mesh->resource_mesh,*transform);
	}

	glUseProgram(App->renderer3D->defaultShader->ID);

	// --- Draw Frustum ---
	if (camera)
		ModuleSceneManager::DrawWire(camera->frustum, White, App->scene_manager->GetPointLineVAO());

	if(App->scene_manager->display_boundingboxes)
	ModuleSceneManager::DrawWire(GO->GetAABB(), Green, App->scene_manager->GetPointLineVAO());
}

void ComponentMeshRenderer::DrawMesh(ResourceMesh& mesh) const
{
	glBindVertexArray(mesh.VAO);

		if (this->checkers)
			glBindTexture(GL_TEXTURE_2D, App->textures->GetCheckerTextureID()); // start using texture
		else
		{
			if(material->resource_diffuse)
			glBindTexture(GL_TEXTURE_2D, material->resource_diffuse->GetTexID());
		}
	


	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
	glDrawElements(GL_TRIANGLES, mesh.IndicesSize, GL_UNSIGNED_INT, NULL); // render primitives from array data

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0); // Stop using buffer (texture)
}

void ComponentMeshRenderer::DrawNormals(const ResourceMesh& mesh, const ComponentTransform& transform) const
{
	// --- Draw Mesh Normals ---

	// --- Set Uniforms ---
	glUseProgram(App->renderer3D->linepointShader->ID);

	GLint modelLoc = glGetUniformLocation(App->renderer3D->linepointShader->ID, "model_matrix");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, transform.GetGlobalTransform().Transposed().ptr());

	GLint viewLoc = glGetUniformLocation(App->renderer3D->linepointShader->ID, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, App->renderer3D->active_camera->GetOpenGLViewMatrix().ptr());

	float nearp = App->renderer3D->active_camera->GetNearPlane();

	// right handed projection matrix
	float f = 1.0f / tan(App->renderer3D->active_camera->GetFOV()*DEGTORAD / 2.0f);
	float4x4 proj_RH(
		f / App->renderer3D->active_camera->GetAspectRatio(), 0.0f, 0.0f, 0.0f,
		0.0f, f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, nearp, 0.0f);

	GLint projectLoc = glGetUniformLocation(App->renderer3D->linepointShader->ID, "projection");
	glUniformMatrix4fv(projectLoc, 1, GL_FALSE, proj_RH.ptr());

	int vertexColorLocation = glGetAttribLocation(App->renderer3D->linepointShader->ID, "color");
	glVertexAttrib3f(vertexColorLocation, 1.0, 1.0, 0);

	if (draw_vertexnormals && mesh.vertices->normal)
	{
		// --- Draw Vertex Normals ---

		float3* vertices = new float3[mesh.IndicesSize * 2];

		for (uint i = 0; i < mesh.IndicesSize; ++i)
		{
			// --- Normals ---
			vertices[i * 2] = float3(mesh.vertices[mesh.Indices[i]].position[0], mesh.vertices[mesh.Indices[i]].position[1], mesh.vertices[mesh.Indices[i]].position[2]);
			vertices[(i * 2) + 1] = float3(mesh.vertices[mesh.Indices[i]].position[0] + mesh.vertices[mesh.Indices[i]].normal[0] * NORMAL_LENGTH, mesh.vertices[mesh.Indices[i]].position[1] + mesh.vertices[mesh.Indices[i]].normal[1] * NORMAL_LENGTH, mesh.vertices[mesh.Indices[i]].position[2] + mesh.vertices[mesh.Indices[i]].normal[2] * NORMAL_LENGTH);
		}

		// --- Create VAO, VBO ---
		unsigned int VBO;
		glGenBuffers(1, &VBO);
		// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
		glBindVertexArray(App->scene_manager->GetPointLineVAO());

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3*mesh.IndicesSize*2, vertices, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);


		// --- Draw lines ---
		glLineWidth(3.0f);
		glBindVertexArray(App->scene_manager->GetPointLineVAO());
		glDrawArrays(GL_LINES, 0, mesh.IndicesSize * 2);
		glBindVertexArray(0);
		glLineWidth(1.0f);


		// --- Delete VBO and vertices ---
		glDeleteBuffers(1, &VBO);
		delete[] vertices;
	}
	
	// --- Draw Face Normals 

	if (draw_facenormals)
	{
		Triangle face;
		float3* vertices = new float3[mesh.IndicesSize/3*2];
		
		// --- Compute face normals ---
		for (uint j = 0; j < mesh.IndicesSize / 3; ++j)
		{
			face.a = float3(mesh.vertices[mesh.Indices[j * 3]].position);
			face.b = float3(mesh.vertices[mesh.Indices[(j * 3) + 1]].position);
			face.c = float3(mesh.vertices[mesh.Indices[(j * 3) + 2]].position);

			float3 face_center = face.Centroid();

			float3 face_normal = Cross(face.b - face.a, face.c - face.b);

			face_normal.Normalize();

			vertices[j*2] = float3(face_center.x, face_center.y, face_center.z);
			vertices[(j*2) + 1] = float3(face_center.x + face_normal.x*NORMAL_LENGTH, face_center.y + face_normal.y*NORMAL_LENGTH, face_center.z + face_normal.z*NORMAL_LENGTH);
		}

		// --- Create VAO, VBO ---
		unsigned int VBO;
		glGenBuffers(1, &VBO);
		// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
		glBindVertexArray(App->scene_manager->GetPointLineVAO());

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)*3 * mesh.IndicesSize / 3 * 2, vertices, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		// --- Draw lines ---

		glLineWidth(3.0f);
		glBindVertexArray(App->scene_manager->GetPointLineVAO());
		glDrawArrays(GL_LINES, 0, mesh.IndicesSize / 3 * 2);
		glBindVertexArray(0);
		glLineWidth(1.0f);

		// --- Delete VBO and vertices ---
		glDeleteBuffers(1, &VBO);
		delete[] vertices;
	}

	glUseProgram(App->renderer3D->defaultShader->ID);
}

json ComponentMeshRenderer::Save() const
{
	json node;

	//if (scene_gos[i]->GetComponent<ComponentMaterial>(Component::ComponentType::Material)->resource_material->resource_diffuse)
//{
//	component_path = TEXTURES_FOLDER;
//	component_path.append(std::to_string(scene_gos[i]->GetComponent<ComponentMaterial>(Component::ComponentType::Material)->resource_material->resource_diffuse->GetUID()));
//	component_path.append(".dds");

//	// --- Store path to component file ---
//	file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())];
//	file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["diffuse"] = component_path;
//	component_path = ((scene_gos[i]->GetComponent<ComponentMaterial>(Component::ComponentType::Material)->resource_material->shader->name));
//	file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["shader"] = component_path;
//	file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"];

//	std::vector<Uniform*>* uniforms = &((scene_gos[i]->GetComponent<ComponentMaterial>(Component::ComponentType::Material)->resource_material->uniforms));
//	uint shader = ((scene_gos[i]->GetComponent<ComponentMaterial>(Component::ComponentType::Material)->resource_material->shader->ID));

//	float* tmpf = new float[4];
//	int* tmpi = new int[4];


//	for (std::vector<Uniform*>::const_iterator iterator = uniforms->begin(); iterator != uniforms->end(); ++iterator)
//	{
//		file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name];
//		file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["type"] = std::to_string((*iterator)->type);

//		switch ((*iterator)->type)
//		{
//		case GL_INT:				
//			glGetUniformiv(shader, (*iterator)->location, tmpi);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["x"] = std::to_string(tmpi[0]);
//			break;

//		case GL_FLOAT:
//			glGetUniformfv(shader, (*iterator)->location, tmpf);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["x"] = std::to_string(tmpf[0]);
//			break;

//		case GL_FLOAT_VEC2:
//			glGetUniformfv(shader, (*iterator)->location, tmpf);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["x"] = std::to_string(tmpf[0]);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["y"] = std::to_string(tmpf[1]);
//			break;

//		case GL_FLOAT_VEC3:
//			glGetUniformfv(shader, (*iterator)->location, tmpf);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["x"] = std::to_string(tmpf[0]);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["y"] = std::to_string(tmpf[1]);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["z"] = std::to_string(tmpf[2]);
//			break;

//		case GL_FLOAT_VEC4:
//			glGetUniformfv(shader, (*iterator)->location, tmpf);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["x"] = std::to_string(tmpf[0]);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["y"] = std::to_string(tmpf[1]);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["z"] = std::to_string(tmpf[2]);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["w"] = std::to_string(tmpf[3]);
//			break;

//		case GL_INT_VEC2:
//			glGetUniformiv(shader, (*iterator)->location, tmpi);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["x"] = std::to_string(tmpi[0]);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["y"] = std::to_string(tmpi[1]);
//			break;

//		case GL_INT_VEC3:
//			glGetUniformiv(shader, (*iterator)->location, tmpi);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["x"] = std::to_string(tmpi[0]);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["y"] = std::to_string(tmpi[1]);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["z"] = std::to_string(tmpi[2]);
//			break;

//		case GL_INT_VEC4:
//			glGetUniformiv(shader, (*iterator)->location, tmpi);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["x"] = std::to_string(tmpi[0]);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["y"] = std::to_string(tmpi[1]);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["z"] = std::to_string(tmpi[2]);
//			file[scene_gos[i]->GetName()]["Components"][std::to_string((uint)scene_gos[i]->GetComponents()[j]->GetType())]["uniforms"][(*iterator)->name]["w"] = std::to_string(tmpi[3]);
//			break;

//		default:
//			continue;
//			break;

//		}

//	}

//	delete[] tmpf;
//	delete[] tmpi;

//}

	return node;
}

void ComponentMeshRenderer::Load(json& node)
{
}
