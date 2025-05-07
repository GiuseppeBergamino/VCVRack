#include "plugin.hpp"


Plugin* pluginInstance;


void init(Plugin* p) {
	pluginInstance = p;

	// Add modules here
    p->addModel(modelPomodrone);
    p->addModel(modelMacro);
    p->addModel(modelRikeda);
    p->addModel(modelTaig);
    p->addModel(modelRizl);
    p->addModel(modelMixa);
	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
