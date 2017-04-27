/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2013 Blender Foundation.
 * All rights reserved.
 *
 * Original Author: Joshua Leung
 * Contributor(s): Based on original depsgraph.c code - Blender Foundation (2005-2013)
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/depsgraph/intern/builder/deg_builder_nodes_scene.cc
 *  \ingroup depsgraph
 *
 * Methods for constructing depsgraph's nodes
 */

#include "intern/builder/deg_builder_nodes.h"

#include <stdio.h>
#include <stdlib.h>

#include "MEM_guardedalloc.h"

extern "C" {
#include "BLI_blenlib.h"
#include "BLI_string.h"
#include "BLI_utildefines.h"

#include "DNA_node_types.h"
#include "DNA_layer_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"

#include "BKE_layer.h"
#include "BKE_main.h"
#include "BKE_node.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"
} /* extern "C" */

#include "intern/builder/deg_builder.h"
#include "intern/nodes/deg_node.h"
#include "intern/nodes/deg_node_component.h"
#include "intern/nodes/deg_node_operation.h"
#include "intern/depsgraph_types.h"
#include "intern/depsgraph_intern.h"
#include "util/deg_util_foreach.h"

namespace DEG {

void DepsgraphNodeBuilder::build_scene(Main *bmain, Scene *scene)
{
	/* scene ID block */
	add_id_node(&scene->id);

	/* timesource */
	add_time_source(NULL);

	/* build subgraph for set, and link this in... */
	// XXX: depending on how this goes, that scene itself could probably store its
	//      own little partial depsgraph?
	if (scene->set) {
		build_scene(bmain, scene->set);
	}

	/* scene objects */
	int selection_color = 1;
	for (SceneLayer *sl = (SceneLayer *)scene->render_layers.first; sl; sl = sl->next) {
		for (Base *base = (Base *)sl->object_bases.first; base; base = base->next) {
			/* object itself */
			build_object(scene, base->object);
			base->selcol = selection_color++;
		}
	}

	/* rigidbody */
	if (scene->rigidbody_world) {
		build_rigidbody(scene);
	}

	/* scene's animation and drivers */
	if (scene->adt) {
		build_animdata(&scene->id);
	}

	/* world */
	if (scene->world) {
		build_world(scene->world);
	}

	/* compo nodes */
	if (scene->nodetree) {
		build_compositor(scene);
	}

	/* sequencer */
	// XXX...

	/* grease pencil */
	if (scene->gpd) {
		build_gpencil(scene->gpd);
	}

	/* Palettes. */
	LINKLIST_FOREACH(Palette *, palette, &bmain->palettes) {
		build_palette(palette);
	}

	/* Cache file. */
	LINKLIST_FOREACH (CacheFile *, cachefile, &bmain->cachefiles) {
		build_cachefile(cachefile);
	}

	/* Masks. */
	LINKLIST_FOREACH (Mask *, mask, &bmain->mask) {
		build_mask(mask);
	}

	/* Movie clips. */
	LINKLIST_FOREACH (MovieClip *, clip, &bmain->movieclip) {
		build_movieclip(clip);
	}

	/* Collections. */
	build_scene_layer_collections(scene);
}

}  // namespace DEG
