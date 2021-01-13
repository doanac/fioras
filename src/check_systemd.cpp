#include <gio/gio.h>
#include <string>

#include "check.h"

static GVariant *dbus_call(GDBusConnection *con, const char *object_path, const char *interface, const char *method,
                           GVariant *params) {
  g_autoptr(GDBusProxy) proxy = NULL;
  g_autoptr(GError) error = NULL;

  proxy = g_dbus_proxy_new_sync(con, G_DBUS_PROXY_FLAGS_NONE, NULL, /* GDBusInterfaceInfo */
                                "org.freedesktop.systemd1",         /* name */
                                object_path,                        /* object path */
                                interface,                          /* interface */
                                NULL,                               /* GCancellable */
                                &error);

  if (proxy == NULL or error != NULL) {
    std::string msg("Unable to get dbus proxy.");
    if (error != NULL) {
      msg += " ";
      msg += error->message;
    }
    throw std::runtime_error(msg);
  }

  GVariant *variant = g_dbus_proxy_call_sync(proxy, method, params, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

  if (variant == NULL || error != NULL) {
    std::string msg("Method(");
    msg += method;
    msg += ") failed.";
    if (error != NULL) {
      msg += " ";
      msg += error->message;
    }
    throw std::runtime_error(msg);
  }
  return variant;
}

static std::string unit_state(GDBusConnection *con, const char *obj_path, const char *property) {
  GVariant *params = g_variant_new("(ss)", "org.freedesktop.systemd1.Unit", property);
  g_autoptr(GVariant) active_state = dbus_call(con, obj_path, "org.freedesktop.DBus.Properties", "Get", params);

  g_autoptr(GVariant) state = NULL;
  g_variant_get_child(active_state, 0, "v", &state);
  g_autofree gchar *state_str;
  g_variant_get(state, "s", &state_str);
  return std::string(state_str);
}

SystemDCheck::SystemDCheck(const std::string &service_name) : svc_(service_name) {}

Status SystemDCheck::run() const {
  try {
    g_autoptr(GDBusConnection) con = NULL;
    g_autoptr(GError) error = NULL;

    con = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (con == NULL || error != NULL) {
      std::string msg("Unable to get dbus connection.");
      if (error != NULL) {
        msg += " ";
        msg += error->message;
      }
      throw std::runtime_error(msg);
    }

    GVariant *get_unit_params = g_variant_new("(s)", svc_.c_str());
    g_autoptr(GVariant) unit =
        dbus_call(con, "/org/freedesktop/systemd1", "org.freedesktop.systemd1.Manager", "GetUnit", get_unit_params);

    g_autofree gchar *obj_path = NULL;
    g_variant_get(unit, "(o)", &obj_path);

    std::string active_state = unit_state(con, obj_path, "ActiveState");
    std::string load_state = unit_state(con, obj_path, "LoadState");
    std::string sub_state = unit_state(con, obj_path, "SubState");
    StatusVal val = StatusVal::ERROR;
    if (active_state == "active" && sub_state == "running") {
      val = StatusVal::OK;
    }
    std::string msg = load_state + " / " + active_state + "(" + sub_state + ")";
    return Status{msg, val};
  } catch (const std::exception &ex) {
    return Status{ex.what(), StatusVal::ERROR};
  }
}

std::string SystemDCheck::getLog() const { return "TODO"; }
