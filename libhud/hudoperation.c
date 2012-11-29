/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of either or both of the following licences:
 *
 * 1) the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation; and/or
 * 2) the GNU Lesser General Public License version 2.1, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 and version 2.1 along with this program.  If not,
 * see <http://www.gnu.org/licenses/>
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "hudoperation.h"

#include "hudoperation-private.h"

G_DEFINE_TYPE (HudOperation, hud_operation, G_TYPE_SIMPLE_ACTION_GROUP)

enum
{
  SIGNAL_PREPARE,
  SIGNAL_STARTED,
  SIGNAL_CHANGED,
  SIGNAL_ENDED,
  N_SIGNALS
};

static guint hud_operation_signals[N_SIGNALS];

static void
hud_operation_finalize (GObject *object)
{
  HudOperation *operation = HUD_OPERATION (object);

  if (operation->priv->group)
    g_object_unref (operation->priv->group);

  G_OBJECT_CLASS (hud_operation_parent_class)
    ->finalize (object);
}

static void
hud_operation_init (HudOperation *operation)
{
  operation->priv = G_TYPE_INSTANCE_GET_PRIVATE (operation, HUD_TYPE_OPERATION, HudOperationPrivate);
  operation->priv->group = g_simple_action_group_new ();
}

static void
hud_operation_class_init (HudOperationClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = hud_operation_finalize;

  g_type_class_add_private (class, sizeof (HudOperationPrivate));

  hud_operation_signals[SIGNAL_PREPARE] = g_signal_new ("prepare", HUD_TYPE_OPERATION, G_SIGNAL_RUN_FIRST,
                                                        G_STRUCT_OFFSET (HudOperationClass, start),
                                                        NULL, NULL, g_cclosure_marshal_VOID__VOID,
                                                        G_TYPE_NONE, 0);
  hud_operation_signals[SIGNAL_STARTED] = g_signal_new ("started", HUD_TYPE_OPERATION, G_SIGNAL_RUN_FIRST,
                                                        G_STRUCT_OFFSET (HudOperationClass, start),
                                                        NULL, NULL, g_cclosure_marshal_VOID__VOID,
                                                        G_TYPE_NONE, 0);
  hud_operation_signals[SIGNAL_CHANGED] = g_signal_new ("changed", HUD_TYPE_OPERATION,
                                                        G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
                                                        G_STRUCT_OFFSET (HudOperationClass, update),
                                                        NULL, NULL, g_cclosure_marshal_VOID__STRING,
                                                        G_TYPE_NONE, 1, G_TYPE_STRING);
  hud_operation_signals[SIGNAL_ENDED] = g_signal_new ("ended", HUD_TYPE_OPERATION, G_SIGNAL_RUN_FIRST,
                                                      G_STRUCT_OFFSET (HudOperationClass, end),
                                                      NULL, NULL, g_cclosure_marshal_VOID__VOID,
                                                      G_TYPE_NONE, 0);
}

HudOperation *
hud_operation_new (gpointer user_data)
{
  HudOperation *operation;

  operation = g_object_new (HUD_TYPE_OPERATION, NULL);
  operation->priv->user_data = user_data;

  return operation;
}

void
hud_operation_setup (HudOperation *operation,
                     GVariant     *parameters)
{
  GVariantIter iter;
  const gchar *name;
  GVariant *value;

  g_variant_iter_init (&iter, parameters);
  while (g_variant_iter_loop (&iter, "{&sv}", &name, &value))
    g_action_group_change_action_state (G_ACTION_GROUP (operation->priv->group), name, value);
}

gpointer
hud_operation_get_user_data (HudOperation *operation)
{
  return operation->priv->user_data;
}

void
hud_operation_started (HudOperation *operation)
{
  g_signal_emit (operation, hud_operation_signals[SIGNAL_STARTED], 0);
}

void
hud_operation_ended (HudOperation *operation)
{
  g_signal_emit (operation, hud_operation_signals[SIGNAL_ENDED], 0);
}

gint
hud_operation_get_int (HudOperation *operation,
                       const gchar  *action_name)
{
  GVariant *variant;
  gint value;

  variant = g_action_group_get_action_state (G_ACTION_GROUP (operation->priv->group), action_name);
  value = g_variant_get_int32 (variant);
  g_variant_unref (variant);

  return value;
}

guint
hud_operation_get_uint (HudOperation *operation,
                        const gchar  *action_name)
{
  GVariant *variant;
  guint value;

  variant = g_action_group_get_action_state (G_ACTION_GROUP (operation->priv->group), action_name);
  value = g_variant_get_uint32 (variant);
  g_variant_unref (variant);

  return value;
}

gboolean
hud_operation_get_boolean (HudOperation *operation,
                           const gchar  *action_name)
{
  GVariant *variant;
  gboolean value;

  variant = g_action_group_get_action_state (G_ACTION_GROUP (operation->priv->group), action_name);
  value = g_variant_get_boolean (variant);
  g_variant_unref (variant);

  return value;
}

gdouble
hud_operation_get_double (HudOperation *operation,
                          const gchar  *action_name)
{
  GVariant *variant;
  gdouble value;

  variant = g_action_group_get_action_state (G_ACTION_GROUP (operation->priv->group), action_name);
  value = g_variant_get_double (variant);
  g_variant_unref (variant);

  return value;
}
