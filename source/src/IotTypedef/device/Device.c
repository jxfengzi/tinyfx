/**
 * Copyright (C) 2013-2015
 *
 * @author jxfengzi@gmail.com
 * @date   2016-7-25
 *
 * @file   Device.c
 *
 * @remark
 *
 */

#include <tiny_malloc.h>
#include <tiny_log.h>
#include "Property.h"
#include "Device.h"
#include "Service.h"
#include "Action.h"

#define TAG     "Device"

TINY_LOR
static TinyRet Device_Construct(Device *thiz);

TINY_LOR
static void Device_Dispose(Device *thiz);

TINY_LOR
static void service_release_handler(void *data, void *ctx)
{
    Service_Delete((Service *)data);
}

TINY_LOR
Device* Device_New(void)
{
    Device *thiz = NULL;

    do
    {
        thiz = (Device *)tiny_malloc(sizeof(Device));
        if (thiz == NULL)
        {
            LOG_D(TAG, "tiny_malloc FAILED");
            break;
        }

        if (RET_FAILED(Device_Construct(thiz)))
        {
            Device_Delete(thiz);
            thiz = NULL;
            break;
        }
    } while (false);

    return thiz;
}

TINY_LOR
static TinyRet Device_Construct(Device *thiz)
{
    TinyRet ret = TINY_RET_OK;

    RETURN_VAL_IF_FAIL(thiz, TINY_RET_E_ARG_NULL);

    do
    {
        memset(thiz, 0, sizeof(Device));

        ret = TinyList_Construct(&thiz->services);
        if (RET_FAILED(ret))
        {
            LOG_D(TAG, "TinyList_Construct FAILED: %s", tiny_ret_to_str( ret));
            break;
        }

        thiz->services.additionalData = thiz;
        TinyList_SetDeleteListener(&thiz->services, service_release_handler, thiz);
    } while (false);

    return ret;
}

TINY_LOR
static void Device_Dispose(Device *thiz)
{
    RETURN_IF_FAIL(thiz);

    TinyList_Dispose(&thiz->services);
}

TINY_LOR
void Device_Delete(Device *thiz)
{
    RETURN_IF_FAIL(thiz);

    Device_Dispose(thiz);
    tiny_free(thiz);
}

TINY_LOR
void Device_InitializeInstanceID(Device *thiz, uint16_t diid)
{
    uint16_t siid = 1;
    thiz->iid = diid;

    for (uint32_t j = 0; j < thiz->services.size; ++j)
    {
        uint16_t piid = 1;
        Service *s = (Service *) TinyList_GetAt(&thiz->services, j);
        s->diid = thiz->iid;
        s->iid = siid++;

        for (uint32_t k = 0; k < s->properties.size; ++k)
        {
            Property *p = (Property * )TinyList_GetAt(&s->properties, k);
            p->diid = thiz->iid;
            p->siid = s->iid;
            p->iid = piid++;
        }
    }
}

TINY_LOR
void Device_SetHandler(Device *thiz, PropertyOnGet onGet, PropertyOnSet onSet, ActionOnInvoke onInvoke)
{
    RETURN_IF_FAIL(thiz);

    LOG_D(TAG, "Device_SetHandler");

    for (uint32_t i = 0; i < thiz->services.size; ++i)
    {
        Service *s = (Service *) TinyList_GetAt(&thiz->services, i);

        for (uint32_t j = 0; j < s->properties.size; ++j)
        {
            Property *p = (Property * )TinyList_GetAt(&s->properties, j);
            p->onSet = onSet;
            p->onGet = onGet;
        }

        for (uint32_t j = 0; j < s->actions.size; ++j)
        {
            Action *a = (Action * )TinyList_GetAt(&s->actions, j);
            a->onInvoke = onInvoke;
        }
    }
}

TINY_LOR
bool Device_CheckHandler(Device *thiz)
{
    RETURN_VAL_IF_FAIL(thiz, false);

    LOG_D(TAG, "DeviceManager_CheckHandler");

    for (uint32_t i = 0; i < thiz->services.size; ++i)
    {
        Service *s = (Service *) TinyList_GetAt(&thiz->services, i);

        for (uint32_t j = 0; j < s->properties.size; ++j)
        {
            Property *p = (Property * )TinyList_GetAt(&s->properties, j);
            if (Property_IsReadable(p) && (p->onGet == NULL))
            {
                LOG_E(TAG, "Property.onGet not handle: %s", p->type.name);
                return false;
            }

            if (Property_IsWritable(p) && p->onSet == NULL)
            {
                LOG_E(TAG, "Property.onSet not handle: %s", p->type.name);
                return false;
            }
        }

        for (uint32_t j = 0; j < s->actions.size; ++j)
        {
            Action *a = (Action * )TinyList_GetAt(&s->actions, j);
            if (a->onInvoke == NULL)
            {
                LOG_E(TAG, "Action.onInvoke not handle: %s", a->type.name);
                return false;
            }
        }
    }

    return true;
}