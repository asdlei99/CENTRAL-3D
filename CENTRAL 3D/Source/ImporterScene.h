#ifndef __IMPORTER_SCENE_H__
#define __IMPORTER_SCENE_H__

#include "Importer.h"
#include <vector>
#include <map>
#include <string>

struct aiNode;
struct aiScene;
class ComponentMaterial;
class GameObject;
class ResourceMesh;
class ResourceMaterial;
class ResourceModel;
class Resource;


class ImporterScene : public Importer
{
	
public:
	ImporterScene();
	virtual ~ImporterScene();

	Resource* Import(ImportData& IData) const override;
	Resource* Load(const char* path) const override;
	std::string SaveSceneToFile(std::vector<GameObject*>& scene_gos, std::string& scene_name) const;

	static inline Importer::ImporterType GetType() { return Importer::ImporterType::Scene; };

	void LoadSceneMeshes(const aiScene* scene, std::map<uint, ResourceMesh*>& scene_meshes, const char* source_file) const;
	void FreeSceneMeshes(std::map<uint, ResourceMesh*>* scene_meshes) const;
	void LoadSceneMaterials(const aiScene* scene, std::map<uint, ResourceMaterial*>& scene_mats, const char* source_file, bool library_deleted) const;
	void FreeSceneMaterials(std::map<uint, ResourceMaterial*>* scene_mats) const;


	void LoadNodes(const aiNode* node, GameObject* parent ,const aiScene* scene,  std::vector<GameObject*>& scene_gos, const char* path, std::map<uint, ResourceMesh*>& scene_meshes, std::map<uint, ResourceMaterial*>& scene_mats) const;
};

#endif
