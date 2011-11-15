/*
 * libvirt-gconfig-storage-pool.c: libvirt glib integration
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

#include <string.h>

#include "libvirt-gconfig/libvirt-gconfig.h"

extern gboolean debugFlag;

#define DEBUG(fmt, ...) do { if (G_UNLIKELY(debugFlag)) g_debug(fmt, ## __VA_ARGS__); } while (0)

#define GVIR_CONFIG_STORAGE_POOL_GET_PRIVATE(obj)                         \
        (G_TYPE_INSTANCE_GET_PRIVATE((obj), GVIR_TYPE_CONFIG_STORAGE_POOL, GVirConfigStoragePoolPrivate))

struct _GVirConfigStoragePoolPrivate
{
    gboolean unused;
};

G_DEFINE_TYPE(GVirConfigStoragePool, gvir_config_storage_pool, GVIR_TYPE_CONFIG_OBJECT);


static void gvir_config_storage_pool_class_init(GVirConfigStoragePoolClass *klass)
{
    g_type_class_add_private(klass, sizeof(GVirConfigStoragePoolPrivate));
}


static void gvir_config_storage_pool_init(GVirConfigStoragePool *conn)
{
    GVirConfigStoragePoolPrivate *priv;

    DEBUG("Init GVirConfigStoragePool=%p", conn);

    priv = conn->priv = GVIR_CONFIG_STORAGE_POOL_GET_PRIVATE(conn);

    memset(priv, 0, sizeof(*priv));
}


GVirConfigStoragePool *gvir_config_storage_pool_new(void)
{
    GVirConfigObject *object;

    object = gvir_config_object_new(GVIR_TYPE_CONFIG_STORAGE_POOL,
                                    "pool",
                                    DATADIR "/libvirt/schemas/storagepool.rng");
    return GVIR_CONFIG_STORAGE_POOL(object);
}

GVirConfigStoragePool *gvir_config_storage_pool_new_from_xml(const gchar *xml,
                                                             GError **error)
{
    GVirConfigObject *object;

    object = gvir_config_object_new_from_xml(GVIR_TYPE_CONFIG_STORAGE_POOL,
                                             "pool",
                                             DATADIR "/libvirt/schemas/storagepool.rng",
                                             xml, error);
    return GVIR_CONFIG_STORAGE_POOL(object);
}
