#include "Resource.h"
#include "Application.h"
#include "ModuleResourceManager.h"
#include "ModuleFileSystem.h"
#include "GameObject.h"

#include "mmgr/mmgr.h"


// MYTODO: TO BE DELETED ONCE SHADERS REWORK IS DONE 
Resource::Resource(ResourceType type)
{
	this->type = type;
}

Resource::Resource(ResourceType type, uint UID, std::string source_file)
{
	this->type = type;
	this->UID = UID;
	original_file = source_file;
	App->fs->SplitFilePath(source_file.c_str(), nullptr, &name);
}

Resource::~Resource()
{

}

uint Resource::GetUID() const
{
	return UID;
}

void Resource::SetUID(uint UID)
{ 
	this->UID = UID;
}

Resource::ResourceType Resource::GetType() const
{
	return type;
}

const char * Resource::GetOriginalFile() const
{
	return original_file.c_str();
}

const char * Resource::GetResourceFile() const
{
	return resource_file.c_str();
}

const char* Resource::GetExtension() const
{
	return extension.c_str();
}

const char * Resource::GetName() const
{
	return name.c_str();
}

const uint Resource::GetPreviewTexID() const
{
	return previewTexID;
}

const uint Resource::GetNumInstances() const
{
	return instances;
}

void Resource::SetOriginalFile(const char* new_path)
{
	original_file = new_path;
	// Redo resource file
	Repath();
}

bool Resource::IsInMemory() const
{
	return instances >= 1;
}

bool Resource::LoadToMemory()
{
	if (instances > 0)
	{
		instances++;
	}
	else
	{
		instances = LoadInMemory() ? 1 : 0;
	}

	return instances > 0;
}

void Resource::Release()
{
	if (instances != 0)
	{
		if (--instances == 0)
		{
			FreeMemory();
		}
	}
	else
		CONSOLE_LOG("![Warning]: Trying to release an already released resource: %s", name.c_str());
}

void Resource::AddUser(GameObject* user)
{
	users.push_back(user);
}

void Resource::RemoveUser(GameObject* user)
{
	for (std::vector<GameObject*>::iterator it = users.begin(); it != users.end(); ++it)
	{
		if ((*it)->GetUID() == user->GetUID())
		{
			users.erase(it);
			break;
		}
	}
}

void Resource::NotifyUsers(ResourceNotificationType type)
{
	for (uint i = 0; i < users.size(); ++i)
	{
		users[i]->ONResourceEvent(UID, type);
	}
}

void Resource::SetName(const char * name)
{
	this->name = name;
}


