#define _BSD_SOURCE 1
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <npapi.h>
#include <npruntime.h>

#include "npobject.h"


static bool getProperty(NPP instance, NPObject *obj, const char *name, NPVariant *result) {
    NPIdentifier ident = NPN_GetStringIdentifier(name);
    return NPN_GetProperty(instance, obj, ident, result);
}

static char *getDocumentURL(NPP instance) {
    NPObject *obj;
    
    NPN_GetValue(instance, NPNVWindowNPObject, &obj);
    
    static const char const *identifiers[] = {
        "document", "location", "href", NULL
    };
    
    const char **identifier = &identifiers[0];
    while (1) {
        NPVariant value;
        
        getProperty(instance, obj, *identifier, &value);
        NPN_ReleaseObject(obj);
        
        identifier++;
        if (*identifier) {
            // Expecting an object
            if (!NPVARIANT_IS_OBJECT(value)) {
                NPN_ReleaseVariantValue(&value);
                return NULL;
            }
            obj = NPVARIANT_TO_OBJECT(value);
        } else {
            // Expecting a string
            if (!NPVARIANT_IS_STRING(value)) {
                NPN_ReleaseVariantValue(&value);
                return NULL;
            }
            char *url = strdup(NPVARIANT_TO_STRING(value).utf8characters);
            NPN_ReleaseVariantValue(&value);
            return url;
        }
    }
}

/* Object methods */
static NPObject *objAllocate(NPP npp, NPClass *aClass) {
    return malloc(sizeof(PluginObject));
}

static void objDeallocate(NPObject *npobj) {
    PluginObject *this = (PluginObject*)npobj;
    plugin_free(this->plugin);
    free(this);
}

static bool copyIdentifierName(NPIdentifier ident, char *name, int maxLength) {
    char *heapStr = NPN_UTF8FromIdentifier(ident);
    if (!heapStr) return false;
    int len = strlen(heapStr);
    if (len+1 >= maxLength) return false;
    memcpy(name, heapStr, len+1);
    NPN_MemFree(heapStr);
    return true;
}

static bool objHasMethod(NPObject *npobj, NPIdentifier ident) {
    PluginObject *this = (PluginObject*)npobj;
    char name[64];
    if (!copyIdentifierName(ident, name, sizeof(name)))
        return false;
    
    switch (this->plugin->type) {
        case PT_Version:
            return !strcmp(name, "GetVersion");
        case PT_Authentication:
            return !strcmp(name, "GetParam") || !strcmp(name, "SetParam") ||
                   !strcmp(name, "PerformAction") || !strcmp(name, "GetLastError");
        default:
            return false;
    }
}

static bool objInvoke(NPObject *npobj, NPIdentifier ident,
                      const NPVariant *args, uint32_t argCount,
                      NPVariant *result) {
    PluginObject *this = (PluginObject*)npobj;
    char name[64];
    if (!copyIdentifierName(ident, name, sizeof(name)))
        return false;
    
    switch (this->plugin->type) {
        case PT_Version:
            if (!strcmp(name, "GetVersion") && (argCount == 0)) {
                char *s = version_getVersion(this->plugin);
                STRINGZ_TO_NPVARIANT(s, *result);
                return true;
            }
            return false;
        case PT_Authentication:
            if (!strcmp(name, "GetParam") && (argCount == 1) &&
                NPVARIANT_IS_STRING(args[0])) {
                // Get parameter
                char *s = auth_getParam(this->plugin,
                                        NPVARIANT_TO_STRING(args[0]).utf8characters);
                STRINGZ_TO_NPVARIANT(s, *result);
                return true;
            } else if (!strcmp(name, "SetParam") && (argCount == 2) &&
                       NPVARIANT_IS_STRING(args[0]) && NPVARIANT_IS_STRING(args[1])) {
                // Set parameter
                auth_setParam(this->plugin,
                              NPVARIANT_TO_STRING(args[0]).utf8characters,
                              NPVARIANT_TO_STRING(args[1]).utf8characters);
                VOID_TO_NPVARIANT(*result);
                return true;
            } else if (!strcmp(name, "PerformAction") && (argCount == 1) &&
                       NPVARIANT_IS_STRING(args[0])) {
                // Perform action
                int ret = auth_performAction(this->plugin,
                                             NPVARIANT_TO_STRING(args[0]).utf8characters);
                INT32_TO_NPVARIANT((int32_t)ret, *result);
                return true;
            } else if (!strcmp(name, "GetLastError") && (argCount == 0)) {
                // Get last error
                int ret = auth_getLastError(this->plugin);
                INT32_TO_NPVARIANT((int32_t)ret, *result);
                return true;
            }
            return false;
        default:
            return false;
    }
}

static bool objInvokeDefault(NPObject *npobj, const NPVariant *args,
                             uint32_t argCount, NPVariant *result) {
    return false;
}

static bool objHasProperty(NPObject *npobj, NPIdentifier name) {
    return false;
}

static bool objGetProperty(NPObject *npobj, NPIdentifier name,
                               NPVariant *result) {
    return false;
}

static bool objSetProperty(NPObject *npobj, NPIdentifier name,
                           const NPVariant *value) {
    return false;
}

static bool objRemoveProperty(NPObject *npobj, NPIdentifier name) {
    return false;
}

static bool objEnumerate(NPObject *npobj, NPIdentifier **value,
                         uint32_t *count) {
    return false;
}

/*static bool objConstruct(NPObject *npobj, const NPVariant *args,
                         uint32_t argCount, NPVariant *result) {
    return false;
}*/

static NPClass baseClass = {
    NP_CLASS_STRUCT_VERSION,
    objAllocate,
    objDeallocate,
    NULL,
    objHasMethod,
    objInvoke,
    objInvokeDefault,
    objHasProperty,
    objGetProperty,
    objSetProperty,
    objRemoveProperty,
    objEnumerate,
    //objConstruct,
    NULL,
};


/* Object construction */
static NPObject *npobject_new(NPP instance, PluginType pluginType) {
    PluginObject *obj = (PluginObject*)NPN_CreateObject(instance, &baseClass);
    assert(obj->base._class != NULL);
    
    char *url = getDocumentURL(instance);
    obj->plugin = plugin_new(pluginType,
                             (url != NULL ? url : ""));
    free(url);
    return (NPObject*)obj;
}

NPObject *npobject_fromMIME(NPP instance, NPMIMEType mimeType) {
    if (!strcmp(mimeType, MIME_VERSION)) {
        return npobject_new(instance, PT_Version);
    } else if (!strcmp(mimeType, MIME_AUTHENTICATION)) {
        return npobject_new(instance, PT_Authentication);
    } else {
        return NULL;
    }
}

