#include "PanelInspector.h"
#include "Imgui/imgui.h"

#include "Application.h"
#include "ModuleSceneManager.h"

#include "GameObject.h"
#include "ComponentMesh.h"
#include "ComponentMaterial.h"
#include "ComponentRenderer.h"

#include "mmgr/mmgr.h"

PanelInspector::PanelInspector(char * name) : Panel(name)
{
}

PanelInspector::~PanelInspector()
{
}

bool PanelInspector::Draw()
{
	ImGuiWindowFlags settingsFlags = 0;
	settingsFlags = ImGuiWindowFlags_NoFocusOnAppearing;

	if (ImGui::Begin(name, &enabled, settingsFlags))
	{
		GameObject* Selected = App->scene_manager->GetGameObjects().at(App->scene_manager->GetSelectedGameObjects());

		if(Startup)
		ImGui::SetNextItemOpen(true);

		if (ImGui::TreeNode("Transform"))
		{

			// --- Transform Position ---
			ImGui::Text("Position  ");
			ImGui::SameLine();

			float3 position = Selected->GetPosition();
			ImGui::Text("X");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth()*0.15f);
	
			ImGui::DragFloat("PX", &position.x, 0.005f);

			ImGui::SameLine();

			ImGui::Text("Y");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth()*0.15f);

			ImGui::DragFloat("PY", &position.y, 0.005f);
				
			ImGui::SameLine();

			ImGui::Text("Z");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth()*0.15f);

			ImGui::DragFloat("PZ", &position.z, 0.005f);

			// --- Transform Rotation ---
			ImGui::Text("Rotation  ");
			ImGui::SameLine();

			float3 rotation = Selected->GetRotation();
			ImGui::Text("X");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth()*0.15f);

			ImGui::DragFloat("RX", &rotation.x, 0.005f);

			ImGui::SameLine();

			ImGui::Text("Y");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth()*0.15f);

			ImGui::DragFloat("RY", &rotation.y, 0.005f);

			ImGui::SameLine();

			ImGui::Text("Z");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth()*0.15f);

			ImGui::DragFloat("RZ", &rotation.z, 0.005f);

			// --- Transform Scale ---
			ImGui::Text("Scale     ");
			ImGui::SameLine();

			float3 scale = Selected->GetScale();
			ImGui::Text("X");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth()*0.15f);

			ImGui::DragFloat("SX", &scale.x, 0.005f);

			ImGui::SameLine();

			ImGui::Text("Y");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth()*0.15f);

			ImGui::DragFloat("SY", &scale.y, 0.005f);

			ImGui::SameLine();

			ImGui::Text("Z");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth()*0.15f);

			ImGui::DragFloat("SZ", &scale.z, 0.005f);


			// --- Transform Set ---
			Selected->SetPosition(position.x, position.y, position.z);
			Selected->Scale(scale.x, scale.y, scale.z);


			ImGui::TreePop();
		}

		ImGui::Separator();


		if (Selected->GetComponent(Component::ComponentType::Mesh))
		{
			ComponentMesh* mesh = (ComponentMesh*)Selected->GetComponent(Component::ComponentType::Mesh);

			ImGui::Checkbox("MeshActive", &mesh->GetActive(),false);
			ImGui::SameLine();

			if (Startup)
			ImGui::SetNextItemOpen(true);

			if (ImGui::TreeNode("Mesh"))
			{
				std::string Triangle_count = "Triangles   ";
				Triangle_count.append(std::to_string(mesh->IndicesSize / 3));
				ImGui::Text(Triangle_count.data());

				ImGui::TreePop();
			}
		}
		ImGui::Separator();

		if (Selected->GetComponent(Component::ComponentType::Renderer))
		{
			ComponentRenderer* renderer = (ComponentRenderer*)Selected->GetComponent(Component::ComponentType::Renderer);

			ImGui::Checkbox("RenActive", &renderer->GetActive(), false);
			ImGui::SameLine();

			if (Startup)
				ImGui::SetNextItemOpen(true);

			if (ImGui::TreeNode("Mesh Renderer"))
			{

				ImGui::Checkbox("Vertex Normals", &renderer->draw_vertexnormals);
				ImGui::SameLine();
				ImGui::Checkbox("Face Normals  ", &renderer->draw_facenormals);
				ImGui::SameLine();
				ImGui::Checkbox("Checkers", &renderer->checkers);

				ImGui::TreePop();
			}
		}
		ImGui::Separator();

		if (Selected->GetComponent(Component::ComponentType::Material))
		{
			ComponentMaterial* material = (ComponentMaterial*)Selected->GetComponent(Component::ComponentType::Material);

			ImGui::Checkbox("MatActive", &material->GetActive(), false);
			ImGui::SameLine();
			if (Startup)
				ImGui::SetNextItemOpen(true);

			if (ImGui::TreeNode("Material"))
			{

				// --- Print Texture Path ---
				std::string Path = "Path: ";
				Path.append(material->Texture_path);

				ImGui::Text(Path.data());

				// --- Print Texture Width and Height ---
				ImGui::Text(std::to_string(material->Texture_width).data());
				ImGui::SameLine();
				ImGui::Text(std::to_string(material->Texture_height).data());

				ImGui::TreePop();
			}
		}

		ImGui::Separator();

		if(Startup)
		Startup = false;
	}

	ImGui::End();


	return true;
}
