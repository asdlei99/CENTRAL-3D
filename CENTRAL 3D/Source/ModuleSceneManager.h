#ifndef __SCENE_MANAGER_H__
#define __SCENE_MANAGER_H__

#include "Module.h"
#include <vector>

class GameObject;
class ComponentMaterial;
struct aiScene;
struct ImportMaterialData;
struct par_shapes_mesh_s;

class ModuleSceneManager : public Module
{
public:

	ModuleSceneManager(bool start_enabled = true);
	~ModuleSceneManager();

	bool Init(json file);
	bool Start();
	update_status PreUpdate(float dt);
	update_status Update(float dt);
	bool CleanUp();

	// --- Creators ---
	GameObject* CreateEmptyGameObject();
	ComponentMaterial* CreateEmptyMaterial();
	GameObject* CreateCube(float sizeX, float sizeY, float sizeZ) const;
	GameObject* CreateSphere(float Radius, int slices, int slacks) const;
	void CreateGrid() const;

	void DestroyGameObject(GameObject* go);

	// --- Getters ---
	GameObject* GetSelectedGameObject() const;

	// --- Setters ---
	void SetSelectedGameObject(GameObject* go);
	void SetTextureToSelectedGO(uint id);

	// --- Utilities ---
	void Draw();
	GameObject* GetRootGO() const;

	// --- Save/Load ----
	void SaveStatus(json &file) const override;
	void LoadStatus(const json & file) override;
	void SaveScene();
	void LoadScene();

private:
	void GatherGameObjects(std::vector<GameObject*> & scene_gos, GameObject* go);
	GameObject* CreateRootGameObject();
	void DrawRecursive(GameObject* go);
	void LoadParMesh(par_shapes_mesh_s* mesh, GameObject& new_object) const;

public:
	ComponentMaterial* CheckersMaterial = nullptr;
	ComponentMaterial* DefaultMaterial = nullptr;

private:
	uint go_count = 0;
	GameObject* root = nullptr;
	GameObject* SelectedGameObject = nullptr;
	std::vector<ComponentMaterial*> Materials;
};

#endif