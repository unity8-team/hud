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
 */

#define G_LOG_DOMAIN "test-hudappindicatorsource"

#include "hudsettings.h"
#include "hudsource.h"
#include "hudtoken.h"
#include "hudwindowsource.h"
#include "hudmenumodelcollector.h"
#include "hudtestutils.h"

#include <glib-object.h>
#include <gio/gio.h>
#include <libdbustest/dbus-test.h>

/* hardcode some parameters for reasons of determinism.
 */
HudSettings hud_settings = {
  .indicator_penalty = 50,
  .add_penalty = 10,
  .drop_penalty = 10,
  .end_drop_penalty = 1,
  .swap_penalty = 15,
  .max_distance = 30
};

static const gchar *BAMF_BUS_NAME = "org.ayatana.bamf";
static const gchar *MATCHER_OBJECT_PATH = "/org/ayatana/bamf/matcher";
static const gchar *APPLICATION_INTERFACE_NAME = "org.ayatana.bamf.application";
static const gchar *MATCHER_INTERFACE_NAME = "org.ayatana.bamf.matcher";
static const gchar *VIEW_INTERFACE_NAME = "org.ayatana.bamf.view";
static const gchar *WINDOW_INTERFACE_NAME = "org.ayatana.bamf.window";

static void
test_window_source_add_view_methods (GDBusConnection* connection,
    const gchar *object_path)
{
  dbus_mock_add_method (connection, BAMF_BUS_NAME, object_path,
      VIEW_INTERFACE_NAME, "Name", "", "s", "ret = 'name'");
  dbus_mock_add_method (connection, BAMF_BUS_NAME, object_path,
      VIEW_INTERFACE_NAME, "Children", "", "as", "ret = []");
  dbus_mock_add_method (connection, BAMF_BUS_NAME, object_path,
      VIEW_INTERFACE_NAME, "Icon", "", "s", "ret = 'icon.png'");
}

static void
test_window_source_menu_model ()
{
  const gchar *app_dbus_name = "app.dbus.name";
  const gchar *app_dbus_menu_path = "/app/dbus/menu/path";

  DbusTestService *service = dbus_test_service_new (NULL);
  hud_test_utils_dbus_mock_start (service, BAMF_BUS_NAME,
      MATCHER_OBJECT_PATH, MATCHER_INTERFACE_NAME);
  hud_test_utils_start_menu_model_full (service,
      "./test-menu-input-model-simple", app_dbus_name, app_dbus_menu_path,
      TRUE);
  GDBusConnection *connection = hud_test_utils_mock_dbus_connection_new (service,
      BAMF_BUS_NAME, app_dbus_name, NULL);
  hud_test_utils_process_mainloop (300);

  /* Define the mock window */
  {
    DBusMockProperties* properties = dbus_mock_new_properties ();
    DBusMockMethods* methods = dbus_mock_new_methods ();
    dbus_mock_methods_append (methods, "GetXid", "", "u", "ret = 1");
    dbus_mock_methods_append (methods, "Monitor", "", "i", "ret = 0");
    dbus_mock_methods_append (methods, "Maximized", "", "i", "ret = 1");
    dbus_mock_methods_append (methods, "Xprop", "s", "s", ""
        "dict = {'_GTK_UNIQUE_BUS_NAME': 'app.dbus.name',\n"
        "       '_GTK_APP_MENU_OBJECT_PATH': '/app/dbus/menu/path',\n"
        "       '_GTK_MENUBAR_OBJECT_PATH': '',\n"
        "       '_GTK_APPLICATION_OBJECT_PATH': '/app/dbus/menu/path',\n"
        "       '_GTK_WINDOW_OBJECT_PATH': ''\n"
        "       }\n"
        "ret = dict[args[0]]");
    dbus_mock_add_object (connection, BAMF_BUS_NAME, MATCHER_OBJECT_PATH,
        "/org/ayatana/bamf/window00000001", WINDOW_INTERFACE_NAME, properties,
        methods);
    test_window_source_add_view_methods (connection, "/org/ayatana/bamf/window00000001");
  }

  /* Define the mock application */
  {
      DBusMockProperties* properties = dbus_mock_new_properties ();
      DBusMockMethods* methods = dbus_mock_new_methods ();
      dbus_mock_methods_append (methods, "DesktopFile", "", "s",
          "ret = '/usr/share/applications/name.desktop'");
      dbus_mock_add_object (connection, BAMF_BUS_NAME, MATCHER_OBJECT_PATH,
          "/org/ayatana/bamf/application00000001", APPLICATION_INTERFACE_NAME,
          properties, methods);
      test_window_source_add_view_methods (connection, "/org/ayatana/bamf/application00000001");
    }

  dbus_mock_add_method (connection,
      BAMF_BUS_NAME, MATCHER_OBJECT_PATH,
      MATCHER_INTERFACE_NAME, "ActiveWindow", "", "s",
        "ret = '/org/ayatana/bamf/window00000001'");
  dbus_mock_add_method (connection,
        BAMF_BUS_NAME, MATCHER_OBJECT_PATH,
        MATCHER_INTERFACE_NAME, "ApplicationForXid", "u", "s",
          "if args[0] == 1:\n"
          "    ret = '/org/ayatana/bamf/application00000001'\n"
          "else:\n"
          "    ret = None");

  hud_test_utils_process_mainloop (100);

  HudWindowSource* source = hud_window_source_new ();
  g_assert(source != NULL);
  g_assert(HUD_IS_WINDOW_SOURCE(source));

  hud_test_utils_process_mainloop (100);

  HudTokenList *search = hud_token_list_new_from_string ("simple");
  {
    GPtrArray *results = g_ptr_array_new_with_free_func(g_object_unref);
    hud_source_search(HUD_SOURCE(source), search, hud_test_utils_results_append_func, results);
    g_assert_cmpuint(results->len, ==, 1);
    hud_test_utils_source_assert_result (results, 0, "Simple");
    g_ptr_array_free(results, TRUE);
  }

  hud_token_list_free(search);
  g_object_unref (source);

  hud_test_utils_process_mainloop (100);

  dbus_test_service_stop(service);
  g_object_unref (service);
  g_object_unref (connection);
}

int
main (int argc, char **argv)
{
  g_type_init ();

  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/hud/windowsource/menu_model", test_window_source_menu_model);

  return g_test_run ();
}
