#ifndef __PANEL_SETTINGS_H__
#define __PANEL_SETTINGS_H__

#define FPS_TRACKER_SIZE 100

#include "Panel.h"
#include <vector>

class Module;

class PanelSettings : public Panel
{
public:

	PanelSettings(char* name);
	~PanelSettings();

	void AddFPS(float fps, float ms);
	bool Draw();

private:

	void ApplicationNode() const;
	void WindowNode() const;
	void InputNode() const;
	void HardwareNode() const;

private:
	std::vector<float> FPS_Tracker;
	std::vector<float> MS_Tracker;
};

#endif