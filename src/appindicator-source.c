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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 *          Ted Gould <ted@canonical.com>
 */

#define G_LOG_DOMAIN "hudappindicatorsource"

#include "appindicator-source.h"

#include <glib/gi18n.h>
#include <gio/gio.h>

#include "settings.h"
#include "dbusmenu-collector.h"
#include "source.h"

/**
 * SECTION:hudappindicatorsource
 * @title: HudAppIndicatorSource
 * @short_description: a #HudSource to search through the application
 *   indicators
 *
 * #HudAppIndicatorSource searches through the menus of application
 * indicators.
 **/

/**
 * HudAppIndicatorSource:
 *
 * This is an opaque structure type.
 **/

struct _HudAppIndicatorSource
{
  GObject parent_instance;

  GSequence    *indicators;
  guint         subscription;
  GCancellable *cancellable;
  gint          use_count;
  gboolean      ready;
  guint         watch_id;
  GDBusConnection *connection;
};

typedef GObjectClass HudAppIndicatorSourceClass;

static void hud_app_indicator_source_iface_init (HudSourceInterface *iface);
G_DEFINE_TYPE_WITH_CODE (HudAppIndicatorSource, hud_app_indicator_source, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (HUD_TYPE_SOURCE, hud_app_indicator_source_iface_init))

static void
hud_app_indicator_source_collector_changed (HudSource *collector,
                                            gpointer   user_data)
{
  HudAppIndicatorSource *source = user_data;

  hud_source_changed (HUD_SOURCE (source));
}

static void
hud_app_indicator_source_add_indicator (HudAppIndicatorSource *source,
                                        GVariant              *description)
{
  HudDbusmenuCollector *collector;
  const gchar *dbus_name;
  const gchar *dbus_path;
  GSequenceIter *iter;
  const gchar *id;
  const gchar *icon_name;
  gint32 position;
  gchar *title;

  g_variant_get_child (description, 0, "&s", &icon_name);
  g_variant_get_child (description, 1, "i", &position);
  g_variant_get_child (description, 2, "&s", &dbus_name);
  g_variant_get_child (description, 3, "&o", &dbus_path);
  g_variant_get_child (description, 8, "&s", &id);
  g_variant_get_child (description, 9, "s", &title);

  if (title[0] == '\0')
    {
      g_free (title);
      /* TRANSLATORS:  This is used for Application indicators that
         are not providing a title string.  The '%s' represents the
         unique ID that the app indicator provides, but it is usually
         the package name and not generally human readable.  An example
         for Network Manager would be 'nm-applet'. */
      title = g_strdup_printf(_("Untitled Indicator (%s)"), id);
    }
  g_debug ("adding appindicator %s at %d ('%s', %s, %s, %s)", id, position, title, icon_name, dbus_name, dbus_path);

  collector = hud_dbusmenu_collector_new_for_endpoint (id, title, icon_name,
                                                       hud_settings.indicator_penalty,
                                                       dbus_name, dbus_path,
                                                       HUD_SOURCE_ITEM_TYPE_INDICATOR);
  g_signal_connect (collector, "changed", G_CALLBACK (hud_app_indicator_source_collector_changed), source);

  /* If query is active, mark new app indicator as used. */
  if (source->use_count)
    hud_source_use (HUD_SOURCE (collector));

  iter = g_sequence_get_iter_at_pos (source->indicators, position);
  g_sequence_insert_before (iter, collector);
  g_free (title);
}

static void
hud_app_indicator_source_remove_indicator (HudAppIndicatorSource *source,
                                           GVariant              *description)
{
  GSequenceIter *iter;
  gint32 position;

  g_variant_get_child (description, 0, "i", &position);

  g_debug ("removing appindicator at %d", position);

  iter = g_sequence_get_iter_at_pos (source->indicators, position);
  if (!g_sequence_iter_is_end (iter))
    {
      HudDbusmenuCollector *collector;

      collector = g_sequence_get (iter);
      g_signal_handlers_disconnect_by_func (collector, hud_app_indicator_source_collector_changed, source);
      /* Drop use count if we added one... */
      if (source->use_count)
        hud_source_unuse (HUD_SOURCE (collector));
      g_sequence_remove (iter);
    }
}

static void
hud_app_indicator_source_dbus_signal (GDBusConnection *connection,
                                      const gchar     *sender_name,
                                      const gchar     *object_path,
                                      const gchar     *interface_name,
                                      const gchar     *signal_name,
                                      GVariant        *parameters,
                                      gpointer         user_data)
{
  HudAppIndicatorSource *source = user_data;

  g_debug ("got signal");

  if (!source->ready)
    {
      g_debug ("not ready, so ignoring signal");
      return;
    }

  if (g_str_equal (signal_name, "ApplicationAdded"))
    {
      if (!g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(sisossssss)")))
        return;

      hud_app_indicator_source_add_indicator (source, parameters);
      hud_source_changed (HUD_SOURCE (source));
    }

  else if (g_str_equal (signal_name, "ApplicationRemoved"))
    {
      if (!g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(i)")))
        return;

      hud_app_indicator_source_remove_indicator (source, parameters);
      hud_source_changed (HUD_SOURCE (source));
    }

  else if (g_str_equal (signal_name, "ApplicationTitleChanged"))
    {
      GSequenceIter *iter;
      const gchar *title;
      gint32 position;

      if (!g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(is)")))
        return;

      g_variant_get (parameters, "(i&s)", &position, &title);

      g_debug ("changing title of appindicator at %d to '%s'", position, title);

      iter = g_sequence_get_iter_at_pos (source->indicators, position);
      if (!g_sequence_iter_is_end (iter))
        {
          HudDbusmenuCollector *collector;

          collector = g_sequence_get (iter);
          hud_dbusmenu_collector_set_prefix (collector, title);
        }
    }

  else if (g_str_equal (signal_name, "ApplicationIconChanged"))
    {
      GSequenceIter *iter;
      const gchar *icon;
      gint32 position;

      if (!g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(iss)")))
        return;

      g_variant_get (parameters, "(i&ss)", &position, &icon, NULL);

      g_debug ("changing icon of appindicator at %d to '%s'", position, icon);

      iter = g_sequence_get_iter_at_pos (source->indicators, position);
      if (!g_sequence_iter_is_end (iter))
        {
          HudDbusmenuCollector *collector;

          collector = g_sequence_get (iter);
          hud_dbusmenu_collector_set_icon (collector, icon);
        }
    }
}

static void
hud_app_indicator_source_ready (GObject      *connection,
                                GAsyncResult *result,
                                gpointer      user_data)
{
  HudAppIndicatorSource *source = user_data;
  GError *error = NULL;
  GVariant *reply;

  g_debug ("GetApplications returned");

  g_clear_object (&source->cancellable);

  if (source == NULL)
  {
    g_debug("Callback invoked with null connection");
    return;
  }

  reply = g_dbus_connection_call_finish (G_DBUS_CONNECTION (connection), result, &error);

  if (reply)
    {
      GVariant *description;
      GVariantIter *iter;

      g_assert (!source->ready);
      source->ready = TRUE;

      g_debug ("going ready");

      g_variant_get (reply, "(a(sisossssss))", &iter);
      while ((description = g_variant_iter_next_value (iter)))
        {
          hud_app_indicator_source_add_indicator (source, description);
          g_variant_unref (description);
        }
      g_variant_iter_free (iter);
      g_variant_unref (reply);

      hud_source_changed (HUD_SOURCE (source));
    }
  else
    {
      g_warning ("GetApplications returned an error: %s", error->message);
      g_error_free (error);
    }

  g_object_unref (source);

  g_debug ("done handling GetApplications reply");
}

static void
hud_app_indicator_source_name_appeared (GDBusConnection *connection,
                                        const gchar     *name,
                                        const gchar     *name_owner,
                                        gpointer         user_data)
{
  HudAppIndicatorSource *source = user_data;

  g_debug ("name appeared (owner is %s)", name_owner);

  g_assert (source->subscription == 0);
  source->subscription = g_dbus_connection_signal_subscribe (connection, name_owner,
                                                             APP_INDICATOR_SERVICE_IFACE, NULL,
                                                             APP_INDICATOR_SERVICE_OBJECT_PATH, NULL,
                                                             G_DBUS_SIGNAL_FLAGS_NONE,
                                                             hud_app_indicator_source_dbus_signal,
                                                             source, NULL);

  g_assert (source->cancellable == NULL);
  source->cancellable = g_cancellable_new ();
  g_dbus_connection_call (connection, name_owner, APP_INDICATOR_SERVICE_OBJECT_PATH, APP_INDICATOR_SERVICE_IFACE,
                          "GetApplications", NULL, G_VARIANT_TYPE ("(a(sisossssss))"),
                          G_DBUS_CALL_FLAGS_NONE, -1, source->cancellable,
                          hud_app_indicator_source_ready, g_object_ref(source));
}

static void
hud_app_indicator_source_name_vanished (GDBusConnection *connection,
                                        const gchar     *name,
                                        gpointer         user_data)
{
  HudAppIndicatorSource *source = user_data;

  g_debug ("name vanished");

  if (source->subscription > 0)
    {
      g_dbus_connection_signal_unsubscribe (connection, source->subscription);
      source->subscription = 0;
    }

  if (source->cancellable)
    {
      g_cancellable_cancel (source->cancellable);
      g_clear_object (&source->cancellable);
    }

  if (source->ready)
    {
      GSequenceIter *iter;

      source->ready = FALSE;

      iter = g_sequence_get_begin_iter (source->indicators);
      while (!g_sequence_iter_is_end (iter))
        {
          HudDbusmenuCollector *collector;
          GSequenceIter *next;

          collector = g_sequence_get (iter);
          g_signal_handlers_disconnect_by_func (collector, hud_app_indicator_source_collector_changed, source);
          next = g_sequence_iter_next (iter);
          g_sequence_remove (iter);
          iter = next;
        }

      hud_source_changed (HUD_SOURCE (source));
    }
}

static void
hud_app_indicator_source_use (HudSource *hud_source)
{
  HudAppIndicatorSource *source = HUD_APP_INDICATOR_SOURCE (hud_source);

  if (source->use_count == 0)
    g_sequence_foreach (source->indicators, (GFunc) hud_source_use, NULL);

  source->use_count++;
}

static void
hud_app_indicator_source_unuse (HudSource *hud_source)
{
  HudAppIndicatorSource *source = HUD_APP_INDICATOR_SOURCE (hud_source);

  g_return_if_fail (source->use_count > 0);

  source->use_count--;

  if (source->use_count == 0)
    g_sequence_foreach (source->indicators, (GFunc) hud_source_unuse, NULL);
}

static void
hud_app_indicator_source_search (HudSource    *hud_source,
                                 HudTokenList *search_tokens,
                                 void        (*append_func) (HudResult * result, gpointer user_data),
                                 gpointer      user_data)
{
  HudAppIndicatorSource *source = HUD_APP_INDICATOR_SOURCE (hud_source);
  GSequenceIter *iter;

  iter = g_sequence_get_begin_iter (source->indicators);

  while (!g_sequence_iter_is_end (iter))
    {
      hud_source_search (g_sequence_get (iter), search_tokens, append_func, user_data);
      iter = g_sequence_iter_next (iter);
    }
}

static void
hud_app_indicator_source_list_applications (HudSource    *hud_source,
                                            HudTokenList *search_tokens,
                                            void        (*append_func) (const gchar *application_id, const gchar *application_icon, HudSourceItemType type, gpointer user_data),
                                            gpointer      user_data)
{
  HudAppIndicatorSource *source = HUD_APP_INDICATOR_SOURCE (hud_source);
  GSequenceIter *iter;

  iter = g_sequence_get_begin_iter (source->indicators);

  while (!g_sequence_iter_is_end (iter))
    {
      hud_source_list_applications (g_sequence_get (iter), search_tokens, append_func, user_data);
      iter = g_sequence_iter_next (iter);
    }
}

static HudSource *
hud_app_indicator_source_get (HudSource     *hud_source,
                              const gchar   *application_id)
{
  HudAppIndicatorSource *source = HUD_APP_INDICATOR_SOURCE (hud_source);
  GSequenceIter *iter;

  iter = g_sequence_get_begin_iter (source->indicators);

  while (!g_sequence_iter_is_end (iter))
    {
      HudSource *result = hud_source_get (g_sequence_get (iter), application_id);
      if (result != NULL)
        return result;
      iter = g_sequence_iter_next (iter);
    }
  return NULL;
}

static void
hud_app_indicator_source_finalize (GObject *object)
{
  g_debug("hud_app_indicator_source_finalize");

  HudAppIndicatorSource *source = HUD_APP_INDICATOR_SOURCE(object);
  if (source->subscription)
  {
    g_dbus_connection_signal_unsubscribe(source->connection, source->subscription);
    source->subscription = 0;
  }
  g_bus_unwatch_name(source->watch_id);
  g_sequence_free(source->indicators);
  g_object_unref(source->connection);

  G_OBJECT_CLASS(hud_app_indicator_source_parent_class)->finalize(object);
}

static void
hud_app_indicator_source_init (HudAppIndicatorSource *source)
{
  g_debug ("online");

  source->indicators = g_sequence_new (g_object_unref);
}

static void
hud_app_indicator_source_iface_init (HudSourceInterface *iface)
{
  iface->use = hud_app_indicator_source_use;
  iface->unuse = hud_app_indicator_source_unuse;
  iface->search = hud_app_indicator_source_search;
  iface->list_applications = hud_app_indicator_source_list_applications;
  iface->get = hud_app_indicator_source_get;
}

static void
hud_app_indicator_source_class_init (HudAppIndicatorSourceClass *class)
{
  class->finalize = hud_app_indicator_source_finalize;
}

/**
 * hud_app_indicator_source_new:
 *
 * Creates a #HudAppIndicatorSource.
 *
 * Returns: a new #HudAppIndicatorSource
 **/
HudAppIndicatorSource *
hud_app_indicator_source_new (GDBusConnection *connection)
{
  HudAppIndicatorSource *source = g_object_new (HUD_TYPE_APP_INDICATOR_SOURCE, NULL);

  source->connection = g_object_ref(connection);
  source->watch_id = g_bus_watch_name_on_connection(connection, APP_INDICATOR_SERVICE_BUS_NAME, G_BUS_NAME_WATCHER_FLAGS_NONE,
                    hud_app_indicator_source_name_appeared, hud_app_indicator_source_name_vanished,
                    source, NULL);
  return source;
}