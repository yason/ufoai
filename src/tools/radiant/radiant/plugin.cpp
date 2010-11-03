/**
 * @file plugin.cpp
 */

/*
 Copyright (C) 2001-2006, William Joseph.
 All Rights Reserved.

 This file is part of GtkRadiant.

 GtkRadiant is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 GtkRadiant is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with GtkRadiant; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <iostream>
#include "plugin.h"

#include "debugging/debugging.h"

#include "iradiant.h"
#include "ifilesystem.h"
#include "ishadersystem.h"
#include "ientity.h"
#include "ieclass.h"
#include "irender.h"
#include "iscenegraph.h"
#include "iselection.h"
#include "ifilter.h"
#include "iscriplib.h"
#include "igl.h"
#include "iundo.h"
#include "itextures.h"
#include "ireference.h"
#include "ifiletypes.h"
#include "preferencesystem.h"
#include "brush/TexDef.h"
#include "ibrush.h"
#include "iimage.h"
#include "itoolbar.h"
#include "iregistry.h"
#include "iplugin.h"
#include "imaterial.h"
#include "iump.h"
#include "iufoscript.h"
#include "imap.h"
#include "iclipper.h"
#include "namespace.h"
#include "isound.h"
#include "ifilter.h"
#include "ieventmanager.h"
#include "igrid.h"
#include "iufoscript.h"
#include "ioverlay.h"

#include "gtkutil/image.h"
#include "gtkutil/messagebox.h"
#include "gtkutil/filechooser.h"

#include "maplib.h"
#include "map/map.h"
#include "qe3.h"
#include "sidebar/sidebar.h"
#include "gtkmisc.h"
#include "mainframe.h"
#include "ui/mru/MRU.h"
#include "entity.h"
#include "select.h"
#include "settings/preferences.h"
#include "map/AutoSaver.h"
#include "dialogs/findtextures.h"
#include "referencecache/nullmodel.h"
#include "xyview/GlobalXYWnd.h"
#include "camera/GlobalCamera.h"
#include "material.h"
#include "ump.h"
#include "particles.h"
#include "pathfinding.h"
#include "model.h"

#include "modulesystem/modulesmap.h"
#include "modulesystem/singletonmodule.h"

#include "generic/callback.h"

inline const std::string& GameDescription_getRequiredKeyValue (const std::string& key)
{
	return g_pGameDescription->getRequiredKeyValue(key);
}

inline const std::string getMapName ()
{
	return Map_Name(g_map);
}

inline scene::Node& getMapWorldEntity ()
{
	return Map_FindOrInsertWorldspawn(g_map);
}

class RadiantCoreAPI
{
		IRadiant m_radiantcore;
		public:
		typedef IRadiant Type;
		STRING_CONSTANT(Name, "*");

		RadiantCoreAPI ()
		{
			m_radiantcore.getMainWindow = MainFrame_getWindow;
			m_radiantcore.getEnginePath = &EnginePath_get;
			m_radiantcore.getAppPath = &AppPath_get;
			m_radiantcore.getSettingsPath = &SettingsPath_get;
			m_radiantcore.getMapsPath = &getMapsPath;

			m_radiantcore.getMapName = &getMapName;
			m_radiantcore.getMapWorldEntity = getMapWorldEntity;

			m_radiantcore.setStatusText = Sys_Status;

			m_radiantcore.getRequiredGameDescriptionKeyValue = &GameDescription_getRequiredKeyValue;

			m_radiantcore.attachGameToolsPathObserver = Radiant_attachGameToolsPathObserver;
			m_radiantcore.detachGameToolsPathObserver = Radiant_detachGameToolsPathObserver;
			m_radiantcore.attachEnginePathObserver = Radiant_attachEnginePathObserver;
			m_radiantcore.detachEnginePathObserver = Radiant_detachEnginePathObserver;
			m_radiantcore.attachGameModeObserver = Radiant_attachGameModeObserver;
			m_radiantcore.detachGameModeObserver = Radiant_detachGameModeObserver;
		}
		IRadiant* getTable ()
		{
			return &m_radiantcore;
		}
};

typedef SingletonModule<RadiantCoreAPI> RadiantCoreModule;
typedef Static<RadiantCoreModule> StaticRadiantCoreModule;
StaticRegisterModule staticRegisterRadiantCore(StaticRadiantCoreModule::instance());

class RadiantDependencies: public GlobalRadiantModuleRef,
		public GlobalEventManagerModuleRef,
		public GlobalFileSystemModuleRef,
		public GlobalSoundManagerModuleRef,
		public GlobalUMPSystemModuleRef,
		public GlobalUFOScriptSystemModuleRef,
		public GlobalMaterialSystemModuleRef,
		public GlobalEntityModuleRef,
		public GlobalShadersModuleRef,
		public GlobalBrushModuleRef,
		public GlobalRegistryModuleRef,
		public GlobalSceneGraphModuleRef,
		public GlobalShaderCacheModuleRef,
		public GlobalFiletypesModuleRef,
		public GlobalSelectionModuleRef,
		public GlobalReferenceModuleRef,
		public GlobalOpenGLModuleRef,
		public GlobalEntityClassManagerModuleRef,
		public GlobalUndoModuleRef,
		public GlobalScripLibModuleRef,
		public GlobalNamespaceModuleRef,
		public GlobalFilterModuleRef,
		public GlobalClipperModuleRef,
		public GlobalGridModuleRef,
		public GlobalOverlayModuleRef
{
		ImageModulesRef m_image_modules;
		MapModulesRef m_map_modules;
		ToolbarModulesRef m_toolbar_modules;
		PluginModulesRef m_plugin_modules;

	public:
		RadiantDependencies () :
			GlobalSoundManagerModuleRef("*"), GlobalUMPSystemModuleRef("*"), GlobalUFOScriptSystemModuleRef("*"),
					GlobalMaterialSystemModuleRef("*"), GlobalEntityModuleRef("ufo"), GlobalShadersModuleRef("ufo"),
					GlobalBrushModuleRef("ufo"), GlobalEntityClassManagerModuleRef("ufo"), m_image_modules(
							GlobalRadiant().getRequiredGameDescriptionKeyValue("texturetypes")),
					m_map_modules("mapufo"), m_toolbar_modules("*"), m_plugin_modules("*")
		{
		}

		ImageModules& getImageModules ()
		{
			return m_image_modules.get();
		}
		MapModules& getMapModules ()
		{
			return m_map_modules.get();
		}
		ToolbarModules& getToolbarModules ()
		{
			return m_toolbar_modules.get();
		}
		PluginModules& getPluginModules ()
		{
			return m_plugin_modules.get();
		}
};

class Radiant: public TypeSystemRef
{
	public:
		Radiant ()
		{
			Preferences_Init();

			/** @todo Add soundtypes support into ufoai.game */
			GlobalFiletypes().addType("sound", "wav", filetype_t("PCM sound files", "*.wav"));
			GlobalFiletypes().addType("sound", "ogg", filetype_t("OGG sound files", "*.ogg"));

			Selection_Construct();
			HomePaths_Construct();
			VFS_Construct();
			GLWindow_Construct();
			Map_Construct();
			EntityList_Construct();
			sidebar::MapInfo_Construct();
			MainFrame_Construct();
			SurfaceInspector_Construct();
			GlobalCamera().construct();
			GlobalXYWnd().construct();
			Material_Construct();
			TextureBrowser_Construct();
			Entity_Construct();
			map::AutoSaver().init();
			EntityInspector_Construct();
			FindTextureDialog_Construct();
			NullModel_Construct();
			MapRoot_Construct();

			EnginePath_verify();
			EnginePath_Realise();

			Particles_Construct();
			Pathfinding_Construct();
			UMP_Construct();
			GlobalUFOScriptSystem()->init();

			// Load the shortcuts from the registry
			GlobalEventManager().loadAccelerators();
		}
		~Radiant ()
		{
			UMP_Destroy();
			Pathfinding_Destroy();
			Particles_Destroy();
			Material_Destroy();
			TextureBrowser_Destroy();

			EnginePath_Unrealise();

			MapRoot_Destroy();
			NullModel_Destroy();
			FindTextureDialog_Destroy();
			EntityInspector_Destroy();
			Entity_Destroy();
			CamWnd_Destroy();
			GlobalXYWnd().destroy();
			SurfaceInspector_Destroy();
			MainFrame_Destroy();
			EntityList_Destroy();
			sidebar::MapInfo_Destroy();
			Map_Destroy();
			GLWindow_Destroy();
			VFS_Destroy();
			HomePaths_Destroy();
			Selection_Destroy();
		}
};

namespace
{
	bool g_RadiantInitialised = false;
	RadiantDependencies* g_RadiantDependencies;
	Radiant* g_Radiant;
}

bool Radiant_Construct (ModuleServer& server)
{
	GlobalModuleServer::instance().set(server);
	/** @todo find a way to do this with in a static way */
	ModelModules_Init();
	StaticModuleRegistryList().instance().registerModules();

	g_RadiantDependencies = new RadiantDependencies();

	g_RadiantInitialised = !server.getError();

	if (g_RadiantInitialised) {
		g_Radiant = new Radiant;
	}

	return g_RadiantInitialised;
}

void Radiant_Destroy ()
{
	if (g_RadiantInitialised) {
		delete g_Radiant;
	}

	delete g_RadiantDependencies;
}

ui::ColourSchemeManager& ColourSchemes() {
	static ui::ColourSchemeManager _manager;
	return _manager;
}

ImageModules& Radiant_getImageModules ()
{
	return g_RadiantDependencies->getImageModules();
}
MapModules& Radiant_getMapModules ()
{
	return g_RadiantDependencies->getMapModules();
}
ToolbarModules& Radiant_getToolbarModules ()
{
	return g_RadiantDependencies->getToolbarModules();
}
PluginModules& Radiant_getPluginModules ()
{
	return g_RadiantDependencies->getPluginModules();
}
