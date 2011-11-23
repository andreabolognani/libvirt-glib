/*
 * libvirt-gobject-domain.c: libvirt glib integration
 *
 * Copyright (C) 2008 Daniel P. Berrange
 * Copyright (C) 2010 Red Hat
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

#include <config.h>

#include <libvirt/virterror.h>
#include <string.h>

#include "libvirt-glib/libvirt-glib.h"
#include "libvirt-gobject/libvirt-gobject.h"
#include "libvirt-gobject-compat.h"

extern gboolean debugFlag;

#define DEBUG(fmt, ...) do { if (G_UNLIKELY(debugFlag)) g_debug(fmt, ## __VA_ARGS__); } while (0)

#define GVIR_DOMAIN_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_TYPE_DOMAIN, GVirDomainPrivate))

struct _GVirDomainPrivate
{
    virDomainPtr handle;
    gchar uuid[VIR_UUID_STRING_BUFLEN];
};

G_DEFINE_TYPE(GVirDomain, gvir_domain, G_TYPE_OBJECT);


enum {
    PROP_0,
    PROP_HANDLE,
};

enum {
    VIR_STARTED,
    VIR_SUSPENDED,
    VIR_RESUMED,
    VIR_STOPPED,
    VIR_UPDATED,
    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL];

#define GVIR_DOMAIN_ERROR gvir_domain_error_quark()


static GQuark
gvir_domain_error_quark(void)
{
    return g_quark_from_static_string("gvir-domain");
}

static void gvir_domain_get_property(GObject *object,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec)
{
    GVirDomain *conn = GVIR_DOMAIN(object);
    GVirDomainPrivate *priv = conn->priv;

    switch (prop_id) {
    case PROP_HANDLE:
        g_value_set_boxed(value, priv->handle);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_domain_set_property(GObject *object,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec)
{
    GVirDomain *conn = GVIR_DOMAIN(object);
    GVirDomainPrivate *priv = conn->priv;

    switch (prop_id) {
    case PROP_HANDLE:
        if (priv->handle)
            virDomainFree(priv->handle);
        priv->handle = g_value_dup_boxed(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_domain_finalize(GObject *object)
{
    GVirDomain *conn = GVIR_DOMAIN(object);
    GVirDomainPrivate *priv = conn->priv;

    DEBUG("Finalize GVirDomain=%p", conn);

    virDomainFree(priv->handle);

    G_OBJECT_CLASS(gvir_domain_parent_class)->finalize(object);
}


static void gvir_domain_constructed(GObject *object)
{
    GVirDomain *conn = GVIR_DOMAIN(object);
    GVirDomainPrivate *priv = conn->priv;

    G_OBJECT_CLASS(gvir_domain_parent_class)->constructed(object);

    /* xxx we may want to turn this into an initable */
    if (virDomainGetUUIDString(priv->handle, priv->uuid) < 0) {
        g_error("Failed to get domain UUID on %p", priv->handle);
    }
}


static void gvir_domain_class_init(GVirDomainClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = gvir_domain_finalize;
    object_class->get_property = gvir_domain_get_property;
    object_class->set_property = gvir_domain_set_property;
    object_class->constructed = gvir_domain_constructed;

    g_object_class_install_property(object_class,
                                    PROP_HANDLE,
                                    g_param_spec_boxed("handle",
                                                       "Handle",
                                                       "The domain handle",
                                                       GVIR_TYPE_DOMAIN_HANDLE,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_NICK |
                                                       G_PARAM_STATIC_BLURB));

    signals[VIR_STARTED] = g_signal_new("started",
                                        G_OBJECT_CLASS_TYPE(object_class),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE |
                                        G_SIGNAL_NO_HOOKS | G_SIGNAL_DETAILED,
                                        G_STRUCT_OFFSET(GVirDomainClass, started),
                                        NULL, NULL,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE,
                                        0);

    signals[VIR_SUSPENDED] = g_signal_new("suspended",
                                        G_OBJECT_CLASS_TYPE(object_class),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE |
                                        G_SIGNAL_NO_HOOKS | G_SIGNAL_DETAILED,
                                        G_STRUCT_OFFSET(GVirDomainClass, suspended),
                                        NULL, NULL,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE,
                                        0);

    signals[VIR_RESUMED] = g_signal_new("resumed",
                                        G_OBJECT_CLASS_TYPE(object_class),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE |
                                        G_SIGNAL_NO_HOOKS | G_SIGNAL_DETAILED,
                                        G_STRUCT_OFFSET(GVirDomainClass, resumed),
                                        NULL, NULL,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE,
                                        0);

    signals[VIR_STOPPED] = g_signal_new("stopped",
                                        G_OBJECT_CLASS_TYPE(object_class),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE |
                                        G_SIGNAL_NO_HOOKS | G_SIGNAL_DETAILED,
                                        G_STRUCT_OFFSET(GVirDomainClass, stopped),
                                        NULL, NULL,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE,
                                        0);

    signals[VIR_UPDATED] = g_signal_new("updated",
                                        G_OBJECT_CLASS_TYPE(object_class),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        G_STRUCT_OFFSET(GVirDomainClass, updated),
                                        NULL, NULL,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE,
                                        0);

    g_type_class_add_private(klass, sizeof(GVirDomainPrivate));
}


static void gvir_domain_init(GVirDomain *conn)
{
    DEBUG("Init GVirDomain=%p", conn);

    conn->priv = GVIR_DOMAIN_GET_PRIVATE(conn);
}

typedef struct virDomain GVirDomainHandle;

static GVirDomainHandle*
gvir_domain_handle_copy(GVirDomainHandle *src)
{
    virDomainRef((virDomainPtr)src);
    return src;
}

static void
gvir_domain_handle_free(GVirDomainHandle *src)
{
    virDomainFree((virDomainPtr)src);
}

G_DEFINE_BOXED_TYPE(GVirDomainHandle, gvir_domain_handle,
                    gvir_domain_handle_copy, gvir_domain_handle_free)

static GVirDomainInfo *
gvir_domain_info_copy(GVirDomainInfo *info)
{
    return g_slice_dup(GVirDomainInfo, info);
}


static void
gvir_domain_info_free(GVirDomainInfo *info)
{
    g_slice_free(GVirDomainInfo, info);
}


G_DEFINE_BOXED_TYPE(GVirDomainInfo, gvir_domain_info,
                    gvir_domain_info_copy, gvir_domain_info_free)


const gchar *gvir_domain_get_name(GVirDomain *dom)
{
    GVirDomainPrivate *priv = dom->priv;
    const char *name;

    if (!(name = virDomainGetName(priv->handle))) {
        g_error("Failed to get domain name on %p", priv->handle);
    }

    return name;
}


const gchar *gvir_domain_get_uuid(GVirDomain *dom)
{
    g_return_val_if_fail(GVIR_IS_DOMAIN(dom), NULL);

    return dom->priv->uuid;
}

gint gvir_domain_get_id(GVirDomain *dom,
                        GError **err)
{
    GVirDomainPrivate *priv = dom->priv;
    gint ret;

    if ((ret = virDomainGetID(priv->handle)) < 0) {
        if (err)
            *err = gvir_error_new_literal(GVIR_DOMAIN_ERROR,
                                          0,
                                          "Unable to get ID for domain");
    }
    return ret;
}


/**
 * gvir_domain_start:
 * @dom: the domain
 * @flags:  the flags
 */
gboolean gvir_domain_start(GVirDomain *dom,
                           guint flags,
                           GError **err)
{
    GVirDomainPrivate *priv = dom->priv;
    int ret;

    if (flags)
        ret = virDomainCreateWithFlags(priv->handle, flags);
    else
        ret = virDomainCreate(priv->handle);
    if (ret < 0) {
        if (err)
            *err = gvir_error_new_literal(GVIR_DOMAIN_ERROR,
                                          0,
                                          "Unable to start domain");
        return FALSE;
    }

    return TRUE;
}

/**
 * gvir_domain_resume:
 * @dom: the domain
 *
 * Returns: TRUE on success
 */
gboolean gvir_domain_resume(GVirDomain *dom,
                            GError **err)
{
    GVirDomainPrivate *priv = dom->priv;

    if (virDomainResume(priv->handle) < 0) {
        if (err)
            *err = gvir_error_new_literal(GVIR_DOMAIN_ERROR,
                                          0,
                                          "Unable to resume domain");
        return FALSE;
    }

    return TRUE;
}

/**
 * gvir_domain_stop:
 * @dom: the domain
 * @flags:  the flags
 */
gboolean gvir_domain_stop(GVirDomain *dom,
                          guint flags,
                          GError **err)
{
    GVirDomainPrivate *priv = dom->priv;
    int ret;

    if (flags)
        ret = virDomainDestroyFlags(priv->handle, flags);
    else
        ret = virDomainDestroy(priv->handle);
    if (ret < 0) {
        if (err)
            *err = gvir_error_new_literal(GVIR_DOMAIN_ERROR,
                                          0,
                                          "Unable to stop domain");
        return FALSE;
    }

    return TRUE;
}

/**
 * gvir_domain_delete:
 * @dom: the domain
 * @flags:  the flags
 */
gboolean gvir_domain_delete(GVirDomain *dom,
                            guint flags,
                            GError **err)
{
    GVirDomainPrivate *priv = dom->priv;
    int ret;

    if (flags)
        ret = virDomainUndefineFlags(priv->handle, flags);
    else
        ret = virDomainUndefine(priv->handle);
    if (ret < 0) {
        if (err)
            *err = gvir_error_new_literal(GVIR_DOMAIN_ERROR,
                                          0,
                                          "Unable to delete domain");
        return FALSE;
    }

    return TRUE;
}

/**
 * gvir_domain_shutdown:
 * @dom: the domain
 * @flags:  the flags
 */
gboolean gvir_domain_shutdown(GVirDomain *dom,
                              guint flags G_GNUC_UNUSED,
                              GError **err)
{
    GVirDomainPrivate *priv = dom->priv;

    if (virDomainShutdown(priv->handle) < 0) {
        if (err)
            *err = gvir_error_new_literal(GVIR_DOMAIN_ERROR,
                                          0,
                                          "Unable to shutdown domain");
        return FALSE;
    }

    return TRUE;
}

/**
 * gvir_domain_reboot:
 * @dom: the domain
 * @flags:  the flags
 */
gboolean gvir_domain_reboot(GVirDomain *dom,
                            guint flags,
                            GError **err)
{
    GVirDomainPrivate *priv = dom->priv;

    if (virDomainReboot(priv->handle, flags) < 0) {
        if (err)
            *err = gvir_error_new_literal(GVIR_DOMAIN_ERROR,
                                          0,
                                          "Unable to reboot domain");
        return FALSE;
    }

    return TRUE;
}

/**
 * gvir_domain_get_config:
 * @dom: the domain
 * @flags:  the flags
 * Returns: (transfer full): the config
 */
GVirConfigDomain *gvir_domain_get_config(GVirDomain *dom,
                                         guint flags,
                                         GError **err)
{
    GVirDomainPrivate *priv = dom->priv;
    gchar *xml;

    if (!(xml = virDomainGetXMLDesc(priv->handle, flags))) {
        if (err)
            *err = gvir_error_new_literal(GVIR_DOMAIN_ERROR,
                                          0,
                                          "Unable to get domain XML config");
        return NULL;
    }

    GVirConfigDomain *conf = gvir_config_domain_new_from_xml(xml, err);
    g_free(xml);
    if ((err != NULL) && (*err != NULL))
        return NULL;

    return conf;
}

/**
 * gvir_domain_set_config:
 * @domain: the domain
 * @conf: the new configuration for the domain
 * @err: (allow-none): Place-holder for error or NULL
 *
 * Resets configuration of an existing domain.
 *
 * Note: If domain is already running, the new configuration will not take
 * affect until domain reboots.
 *
 * Returns: TRUE on success, FALSE if an error occurred.
 */
gboolean gvir_domain_set_config(GVirDomain *domain,
                                GVirConfigDomain *conf,
                                GError **err)
{
    gchar *xml;
    virConnectPtr conn;
    virDomainPtr handle;
    gchar uuid[VIR_UUID_STRING_BUFLEN];
    GVirDomainPrivate *priv = domain->priv;

    g_return_val_if_fail(GVIR_IS_DOMAIN (domain), FALSE);
    g_return_val_if_fail(GVIR_IS_CONFIG_DOMAIN (conf), FALSE);
    g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

    xml = gvir_config_object_to_xml(GVIR_CONFIG_OBJECT(conf));

    g_return_val_if_fail(xml != NULL, FALSE);

    if ((conn = virDomainGetConnect(priv->handle)) == NULL) {
        if (err != NULL)
            *err = gvir_error_new_literal(GVIR_DOMAIN_ERROR,
                                          0,
                                          "Failed to get domain connection");
        g_free (xml);

        return FALSE;
    }

    handle = virDomainDefineXML(conn, xml);
    g_free (xml);

    if (handle == NULL) {
        if (err != NULL)
            *err = gvir_error_new_literal(GVIR_DOMAIN_ERROR,
                                          0,
                                          "Failed to set "
                                          "domain configuration");
        return FALSE;
    }

    virDomainGetUUIDString(handle, uuid);
    virDomainFree(handle);

    if (g_strcmp0 (uuid, priv->uuid) != 0) {
        if (err != NULL)
            *err = gvir_error_new_literal(GVIR_DOMAIN_ERROR,
                                          0,
                                          "Failed to set "
                                          "domain configuration");

        return FALSE;
    }

    return TRUE;
}

/**
 * gvir_domain_get_info:
 * @dom: the domain
 * Returns: (transfer full): the info
 */
GVirDomainInfo *gvir_domain_get_info(GVirDomain *dom,
                                     GError **err)
{
    GVirDomainPrivate *priv = dom->priv;
    virDomainInfo info;
    GVirDomainInfo *ret;

    if (virDomainGetInfo(priv->handle, &info) < 0) {
        if (err)
            *err = gvir_error_new_literal(GVIR_DOMAIN_ERROR,
                                          0,
                                          "Unable to get domain info");
        return NULL;
    }

    ret = g_slice_new(GVirDomainInfo);
    ret->state = info.state;
    ret->maxMem = info.maxMem;
    ret->memory = info.memory;
    ret->nrVirtCpu = info.nrVirtCpu;
    ret->cpuTime = info.cpuTime;

    return ret;
}

/**
 * gvir_domain_screenshot:
 * @stream: stream to use as output
 * @monitor_id: monitor ID to take screenshot from
 * @flags: extra flags, currently unused
 *
 * Returns: (transfer full): mime-type of the image format, or NULL upon error.
 */
gchar *gvir_domain_screenshot(GVirDomain *dom,
                              GVirStream *stream,
                              guint monitor_id,
                              guint flags,
                              GError **err)
{
    GVirDomainPrivate *priv;
    virStreamPtr st = NULL;
    gchar *mime = NULL;

    g_return_val_if_fail(GVIR_IS_DOMAIN(dom), NULL);
    g_return_val_if_fail(GVIR_IS_STREAM(stream), NULL);

    priv = dom->priv;
    g_object_get(stream, "handle", &st, NULL);

    if (!(mime = virDomainScreenshot(priv->handle,
                                     st,
                                     monitor_id,
                                     flags))) {
        if (err)
            *err = gvir_error_new_literal(GVIR_DOMAIN_ERROR,
                                          0,
                                          "Unable to take a screenshot");
        goto end;
    }
end:
    if (st != NULL)
        virStreamFree(st);
    return mime;
}
