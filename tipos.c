#include <stddef.h>
#include <string.h>
#include "tipos.h"

int tipo_es_numerico(const char *tipo)
{
    static const char *const numericos[] = {
        "char", "signed char", "unsigned char", "short", "unsigned short",
        "int", "unsigned int", "long", "unsigned long", "float", "double",
        "long double"
    };
    size_t i;
    if (tipo == NULL)
        return 0;
    for (i = 0; i < sizeof(numericos) / sizeof(numericos[0]); i++)
        if (strcmp(tipo, numericos[i]) == 0)
            return 1;
    return 0;
}

int tipos_compatibles_asignacion(const char *destino, const char *origen)
{
    return tipo_es_numerico(destino) && tipo_es_numerico(origen);
}
