#include "ImporterModel.h"
#include "Application.h"
#include "ModuleFileSystem.h"
#include "ModuleSceneManager.h"
#include "ModuleResourceManager.h"

#include "Assimp/include/cimport.h"
#include "Assimp/include/scene.h"
#include "Assimp/include/postprocess.h"
#include "Assimp/include/cfileio.h"

#include "GameObject.h"
#include "ImporterScene.h"
#include "ImporterMeta.h"
#include "ImporterMesh.h"


#include "Components.h"

#include "ResourceModel.h"
#include "ResourceMeta.h"
#include "ResourceMaterial.h"

#include "mmgr/mmgr.h"


ImporterModel::ImporterModel() : Importer(Importer::ImporterType::Model)
{
}

ImporterModel::~ImporterModel()
{
}

// --- Import external file ---
Resource* ImporterModel::Import(ImportData& IData) const
{
	ImportModelData MData = (ImportModelData&)IData;
	ResourceModel* model = nullptr;
	const aiScene* scene = nullptr;

	// --- Import scene from path ---
	if (App->fs->Exists(MData.path))
		scene = aiImportFileEx(MData.path, aiProcessPreset_TargetRealtime_MaxQuality /*| aiPostProcessSteps::aiProcess_FlipUVs*/, App->fs->GetAssimpIO());

	GameObject* rootnode = nullptr;
	ImporterScene* IScene = App->resources->GetImporter<ImporterScene>();

	if (scene && IScene)
	{
		if (MData.model_overwrite)
		{
			model = MData.model_overwrite;
		}
		else
		{
			uint UID = App->GetRandom().Int();
			model = (ResourceModel*)App->resources->CreateResource(Resource::ResourceType::MODEL, MData.path);
		}

		rootnode = App->scene_manager->CreateEmptyGameObject();

		// --- Save Game objects to vector so we can save to lib later ---
		std::vector<GameObject*> model_gos;
		model_gos.push_back(rootnode);

		// --- Load all meshes ---
		std::map<uint, ResourceMesh*> model_meshes;
		IScene->LoadSceneMeshes(scene, model_meshes, MData.path);

		// --- Load all materials ---
		std::map<uint, ResourceMaterial*> model_mats;
		IScene->LoadSceneMaterials(scene, model_mats, MData.path, MData.library_deleted);

		// --- Use scene->mNumMeshes to iterate on scene->mMeshes array ---
		IScene->LoadNodes(scene->mRootNode,rootnode, scene, model_gos, MData.path, model_meshes, model_mats);

		for (uint i = 0; i < model_meshes.size(); ++i)
		{
			model->AddResource(model_meshes[i]);
			model_meshes[i]->SetParent(model);
		}
		for (uint j = 0; j < model_mats.size(); ++j)
		{
			model->AddResource(model_mats[j]);
			model_mats[j]->SetParent(model);
		}

		// --- Save to Own format file in Library ---
		Save(model, model_gos, rootnode->GetName());

		// --- Free everything ---
		IScene->FreeSceneMaterials(&model_mats);
		IScene->FreeSceneMeshes(&model_meshes);
		rootnode->RecursiveDelete();

		// --- Free scene ---
		aiReleaseImport(scene);

		// --- Create meta ---
		if (MData.model_overwrite == nullptr)
		{
			ImporterMeta* IMeta = App->resources->GetImporter<ImporterMeta>();

			ResourceMeta* meta = (ResourceMeta*)App->resources->CreateResourceGivenUID(Resource::ResourceType::META, IData.path, model->GetUID());

			if (meta)
				IMeta->Save(meta);
		}
	}
	else
		CONSOLE_LOG("|[error]: Error loading FBX %s", IData.path);

	return model;
}


// --- Load file from library ---
Resource* ImporterModel::Load(const char* path) const
{
	ResourceModel* resource = nullptr;

	ImporterMeta* IMeta = App->resources->GetImporter<ImporterMeta>();
	ResourceMeta* meta = (ResourceMeta*)IMeta->Load(path);

	resource = App->resources->models.find(meta->GetUID()) != App->resources->models.end() ? App->resources->models.find(meta->GetUID())->second : (ResourceModel*)App->resources->CreateResourceGivenUID(Resource::ResourceType::MODEL, meta->GetOriginalFile(), meta->GetUID());

	// --- A folder has been renamed ---
	if (!App->fs->Exists(resource->GetOriginalFile()))
	{
		resource->SetOriginalFile(path);
		meta->SetOriginalFile(path);
		App->resources->AddResourceToFolder(resource);
	}

	json file;

	if(App->fs->Exists(resource->GetResourceFile()))
		file = App->GetJLoader()->Load(resource->GetResourceFile());
	else
	{
		// --- Library has been deleted, rebuild ---
		ImportModelData MData(path);
		MData.model_overwrite = resource;
		MData.library_deleted = true;
		Import(MData);
	}
	

	if (!file.is_null())
	{
		// --- Iterate main nodes ---
		for (json::iterator it = file.begin(); it != file.end(); ++it)
		{
			// --- Iterate components ---
			json components = file[it.key()]["Components"];

			for (json::iterator it2 = components.begin(); it2 != components.end(); ++it2)
			{
				// --- Iterate and load resources ---
				json _resources = components[it2.key()]["Resources"];

				if (!_resources.is_null())
				{
					for (json::iterator it3 = _resources.begin(); it3 != _resources.end(); ++it3)
					{
						std::string value = _resources[it3.key()];
						Importer::ImportData IData(value.c_str());
						Resource* to_Add = App->resources->ImportAssets(IData);
						to_Add->SetParent(resource);
						resource->AddResource(to_Add);
					}
				}

			}

		}
	}

	return resource;
}

void ImporterModel::InstanceOnCurrentScene(const char* model_path, ResourceModel* model) const
{
	if (model_path)
	{
		// --- Load Scene/model file ---
		json file = App->GetJLoader()->Load(model_path);
	
		// --- Delete buffer data ---
		if (!file.is_null())
		{
			std::vector<GameObject*> objects;
	
			// --- Iterate main nodes ---
			for (json::iterator it = file.begin(); it != file.end(); ++it)
			{
				// --- Create a Game Object for each node ---
				GameObject* go = App->scene_manager->CreateEmptyGameObject();
	
				// --- Retrieve GO's UID and name ---
				go->SetName(it.key().data());
				std::string uid = file[it.key()]["UID"];
				go->GetUID() = std::stoi(uid);
	
				// --- Iterate components ---
				json components = file[it.key()]["Components"];
	
	
				for (json::iterator it2 = components.begin(); it2 != components.end(); ++it2)
				{
					// --- Determine ComponentType ---
					std::string type_string = it2.key();
					uint type_uint = std::stoi(type_string);
					Component::ComponentType type = (Component::ComponentType)type_uint;
	
					Component* component = nullptr;
	
					// --- Create/Retrieve Component ---
					component = go->AddComponent(type);
	
					// --- Load Component Data ---
					if (component)
						component->Load(components[type_string]);
	
				}
	
				objects.push_back(go);
			}
	

			// --- Parent Game Objects / Build Hierarchy ---
			for (uint i = 0; i < objects.size(); ++i)
			{
				std::string parent_uid_string = file[objects[i]->GetName()]["Parent"];
				uint parent_uid = std::stoi(parent_uid_string);
			
				for (uint j = 0; j < objects.size(); ++j)
				{
					if (parent_uid == objects[j]->GetUID())
					{
						objects[j]->AddChildGO(objects[i]);
					break;
					}
				}
			}

			// --- Add pointer to model ---
			if(objects[0])
			objects[0]->model = model;
		}
	}
}

void ImporterModel::Save(ResourceModel* model, std::vector<GameObject*>& model_gos, std::string& model_name) const
{
	// --- Save Model to file ---

	json file;

	for (int i = 0; i < model_gos.size(); ++i)
	{
		// --- Create GO Structure ---
		file[model_gos[i]->GetName()];
		file[model_gos[i]->GetName()]["UID"] = std::to_string(model_gos[i]->GetUID());
		file[model_gos[i]->GetName()]["Parent"] = std::to_string(model_gos[i]->parent->GetUID());
		file[model_gos[i]->GetName()]["Components"];

		for (int j = 0; j < model_gos[i]->GetComponents().size(); ++j)
		{
			// --- Save Components to file ---
			file[model_gos[i]->GetName()]["Components"][std::to_string((uint)model_gos[i]->GetComponents()[j]->GetType())] = model_gos[i]->GetComponents()[j]->Save();
		}

	}

	// --- Serialize JSON to string ---
	std::string data;
	data = App->GetJLoader()->Serialize(file);

	// --- Finally Save to file ---
	char* buffer = (char*)data.data();
	uint size = data.length();

	App->fs->Save(model->GetResourceFile(), buffer, size);
}

