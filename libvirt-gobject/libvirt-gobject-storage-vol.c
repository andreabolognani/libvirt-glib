/*
 * libvirt-gobject-storage_vol.c: libvirt glib integration
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

#define GVIR_STORAGE_VOL_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_TYPE_STORAGE_VOL, GVirStorageVolPrivate))

struct _GVirStorageVolPrivate
{
    virStorageVolPtr handle;
};

G_DEFINE_TYPE(GVirStorageVol, gvir_storage_vol, G_TYPE_OBJECT);


enum {
    PROP_0,
    PROP_HANDLE,
};


#define GVIR_STORAGE_VOL_ERROR gvir_storage_vol_error_quark()


static GQuark
gvir_storage_vol_error_quark(void)
{
    return g_quark_from_static_string("gvir-storage-vol");
}

static void gvir_storage_vol_get_property(GObject *object,
                                          guint prop_id,
                                          GValue *value,
                                          GParamSpec *pspec)
{
    GVirStorageVol *conn = GVIR_STORAGE_VOL(object);
    GVirStorageVolPrivate *priv = conn->priv;

    switch (prop_id) {
    case PROP_HANDLE:
        g_value_set_boxed(value, priv->handle);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_storage_vol_set_property(GObject *object,
                                          guint prop_id,
                                          const GValue *value,
                                          GParamSpec *pspec)
{
    GVirStorageVol *conn = GVIR_STORAGE_VOL(object);
    GVirStorageVolPrivate *priv = conn->priv;

    switch (prop_id) {
    case PROP_HANDLE:
        if (priv->handle)
            virStorageVolFree(priv->handle);
        priv->handle = g_value_dup_boxed(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}


static void gvir_storage_vol_finalize(GObject *object)
{
    GVirStorageVol *conn = GVIR_STORAGE_VOL(object);
    GVirStorageVolPrivate *priv = conn->priv;

    DEBUG("Finalize GVirStorageVol=%p", conn);

    virStorageVolFree(priv->handle);

    G_OBJECT_CLASS(gvir_storage_vol_parent_class)->finalize(object);
}


static void gvir_storage_vol_class_init(GVirStorageVolClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = gvir_storage_vol_finalize;
    object_class->get_property = gvir_storage_vol_get_property;
    object_class->set_property = gvir_storage_vol_set_property;

    g_object_class_install_property(object_class,
                                    PROP_HANDLE,
                                    g_param_spec_boxed("handle",
                                                       "Handle",
                                                       "The storage_vol handle",
                                                       GVIR_TYPE_STORAGE_VOL_HANDLE,
                                                       G_PARAM_READABLE |
                                                       G_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_NAME |
                                                       G_PARAM_STATIC_NICK |
                                                       G_PARAM_STATIC_BLURB));

    g_type_class_add_private(klass, sizeof(GVirStorageVolPrivate));
}


static void gvir_storage_vol_init(GVirStorageVol *conn)
{
    DEBUG("Init GVirStorageVol=%p", conn);

    conn->priv = GVIR_STORAGE_VOL_GET_PRIVATE(conn);
}

typedef struct virStorageVol GVirStorageVolHandle;

static GVirStorageVolHandle*
gvir_storage_vol_handle_copy(GVirStorageVolHandle *src)
{
    virStorageVolRef((virStorageVolPtr)src);
    return src;
}

static void
gvir_storage_vol_handle_free(GVirStorageVolHandle *src)
{
    virStorageVolFree((virStorageVolPtr)src);
}

G_DEFINE_BOXED_TYPE(GVirStorageVolHandle, gvir_storage_vol_handle,
                    gvir_storage_vol_handle_copy, gvir_storage_vol_handle_free)

const gchar *gvir_storage_vol_get_name(GVirStorageVol *vol)
{
    GVirStorageVolPrivate *priv = vol->priv;
    const char *name;

    if (!(name = virStorageVolGetName(priv->handle))) {
        g_error("Failed to get storage_vol name on %p", priv->handle);
    }

    return name;
}

const gchar *gvir_storage_vol_get_path(GVirStorageVol *vol)
{
    GVirStorageVolPrivate *priv = vol->priv;
    const char *path;

    if (!(path = virStorageVolGetPath(priv->handle))) {
        g_error("Failed to get storage_vol path on %p", priv->handle);
    }

    return path;
}

/**
 * gvir_storage_vol_get_config:
 * @vol: the storage_vol
 * @flags: the flags
 * Returns: (transfer full): the config
 */
GVirConfigStorageVol *gvir_storage_vol_get_config(GVirStorageVol *vol,
                                                  guint flags,
                                                  GError **err)
{
    GVirStorageVolPrivate *priv = vol->priv;
    gchar *xml;

    if (!(xml = virStorageVolGetXMLDesc(priv->handle, flags))) {
        if (err)
            *err = gvir_error_new_literal(GVIR_STORAGE_VOL_ERROR,
                                          0,
                                          "Unable to get storage_vol XML config");
        return NULL;
    }

    GVirConfigStorageVol *conf = gvir_config_storage_vol_new_from_xml(xml, err);

    free(xml);
    return conf;
}
