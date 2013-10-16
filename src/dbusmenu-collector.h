/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __HUD_DBUSMENU_COLLECTOR_H__
#define __HUD_DBUSMENU_COLLECTOR_H__

#include "glib-object.h"
#include "window-info.h"
#include "source.h"

#define HUD_TYPE_DBUSMENU_COLLECTOR                         (hud_dbusmenu_collector_get_type ())
#define HUD_DBUSMENU_COLLECTOR(inst)                        (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             HUD_TYPE_DBUSMENU_COLLECTOR, HudDbusmenuCollector))
#define HUD_IS_DBUSMENU_COLLECTOR(inst)                     (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             HUD_TYPE_DBUSMENU_COLLECTOR))

typedef struct _HudDbusmenuCollector                        HudDbusmenuCollector;

GType                   hud_dbusmenu_collector_get_type                 (void);

HudDbusmenuCollector *  hud_dbusmenu_collector_new_for_endpoint         (const gchar *application_id,
                                                                         const gchar *prefix,
                                                                         const gchar *icon,
                                                                         guint        penalty,
                                                                         const gchar *bus_name,
                                                                         const gchar *object_path,
                                                                         HudSourceItemType type);
HudDbusmenuCollector *  hud_dbusmenu_collector_new_for_window           (guint xid,
                                                                         const gchar *application_id,
                                                                         const gchar *icon,
                                                                         HudSourceItemType type);
void                    hud_dbusmenu_collector_set_prefix               (HudDbusmenuCollector *collector,
                                                                         const gchar          *prefix);
void                    hud_dbusmenu_collector_set_icon                 (HudDbusmenuCollector *collector,
                                                                         const gchar          *icon);
guint32                 hud_dbusmenu_collector_get_xid                  (HudDbusmenuCollector *collector);

#endif /* __HUD_DBUSMENU_COLLECTOR_H__ */
