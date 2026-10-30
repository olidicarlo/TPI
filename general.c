#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "general.h"

extern YYLTYPE yylloc;

static Simbolo *inicio = NULL;
static Simbolo *fin = NULL;
static int nivel_actual = 0;

void pausa(void)
{
    printf("Presione ENTER para continuar...\n");
    getchar();
}

void inicializarUbicacion(void)
{
    yylloc.first_line = yylloc.last_line = INICIO_CONTEO_LINEA;
    yylloc.first_column = yylloc.last_column = INICIO_CONTEO_COLUMNA;
}

void reinicializarUbicacion(void)
{
    yylloc.first_line = yylloc.last_line;
    yylloc.first_column = yylloc.last_column;
}

static void error_fatal(const char *mensaje)
{
    fprintf(stderr, "%s\n", mensaje);
    exit(EXIT_FAILURE);
}

static char *copiar_texto(const char *texto)
{
    char *copia;
    if (texto == NULL)
        return NULL;
    copia = (char *) malloc(strlen(texto) + 1);
    if (copia == NULL)
        error_fatal("Sin memoria");
    strcpy(copia, texto);
    return copia;
}

static int texto_igual(const char *a, const char *b)
{
    return a != NULL && b != NULL && strcmp(a, b) == 0;
}

Simbolo *crear_simbolo(const char *nombre, CategoriaSimbolo categoria,
                       const char *tipo, int linea, int columna)
{
    Simbolo *s;
    if (nombre == NULL || *nombre == '\0' || tipo == NULL || *tipo == '\0' ||
        (categoria != SIMBOLO_VARIABLE && categoria != SIMBOLO_FUNCION))
        return NULL;
    s = (Simbolo *) calloc(1, sizeof(*s));
    if (s == NULL)
        error_fatal("Sin memoria");
    s->nombre = copiar_texto(nombre);
    s->tipo = copiar_texto(tipo);
    s->categoria = categoria;
    s->linea = linea;
    s->columna = columna;
    s->alcance = nivel_actual;
    s->tiene_prototipo = 1;
    return s;
}

static void destruir_registro(Simbolo *s)
{
    Parametros *p, *siguiente;
    for (p = s->parametros; p != NULL; p = siguiente) {
        siguiente = p->siguiente;
        free(p->nombre);
        free(p->tipo);
        free(p);
    }
    free(s->nombre);
    free(s->tipo);
    free(s);
}

void destruir_simbolo(Simbolo *s)
{
    if (s != NULL && !s->en_tabla)
        destruir_registro(s);
}

EstadoTS agregar_parametro_ubicado(Simbolo *f, const char *tipo,
                                  const char *nombre, int linea, int columna)
{
    Parametros *p, **enlace;
    if (f == NULL || f->en_tabla || f->categoria != SIMBOLO_FUNCION ||
        !f->tiene_prototipo || tipo == NULL || *tipo == '\0')
        return TS_ARGUMENTO_INVALIDO;
    /* void se representa con una lista vacia, nunca como argumento real. */
    if (texto_igual(tipo, "void"))
        return TS_PARAMETRO_INVALIDO;
    if (nombre != NULL && *nombre == '\0')
        nombre = NULL;
    for (enlace = &f->parametros; *enlace != NULL;
         enlace = &(*enlace)->siguiente)
        if (nombre != NULL && texto_igual((*enlace)->nombre, nombre))
            return TS_PARAMETRO_DUPLICADO;
    if (f->cant_parametros == INT_MAX)
        error_fatal("Demasiados parametros");
    p = (Parametros *) calloc(1, sizeof(*p));
    if (p == NULL)
        error_fatal("Sin memoria");
    p->tipo = copiar_texto(tipo);
    p->nombre = copiar_texto(nombre);
    p->linea = linea;
    p->columna = columna;
    *enlace = p;
    f->cant_parametros++;
    return TS_OK;
}

void agregar_parametro(Simbolo *f, const char *tipo, const char *nombre)
{
    (void) agregar_parametro_ubicado(f, tipo, nombre,
                                    f != NULL ? f->linea : 0,
                                    f != NULL ? f->columna : 0);
}

Parametros *obtener_parametro(const Simbolo *f, int n)
{
    Parametros *p;
    if (f == NULL || f->categoria != SIMBOLO_FUNCION || n < 0)
        return NULL;
    for (p = f->parametros; p != NULL && n > 0; p = p->siguiente)
        n--;
    return p;
}

void marcar_definido(Simbolo *s)
{
    if (s != NULL && !s->en_tabla && s->categoria == SIMBOLO_FUNCION)
        s->definicion = 1;
}

Simbolo *buscar_simbolo(const char *nombre)
{
    Simbolo *s, *mejor = NULL;
    if (nombre == NULL)
        return NULL;
    for (s = inicio; s != NULL; s = s->siguiente)
        if (s->activo && texto_igual(s->nombre, nombre) &&
            (mejor == NULL || s->alcance > mejor->alcance ||
             (s->alcance == mejor->alcance &&
              !(mejor->categoria == SIMBOLO_FUNCION &&
                mejor->tiene_prototipo && !s->tiene_prototipo))))
            mejor = s;
    return mejor;
}

Simbolo *buscar_simbolo_en_alcance_actual(const char *nombre)
{
    Simbolo *s, *encontrado = NULL;
    if (nombre == NULL)
        return NULL;
    for (s = inicio; s != NULL; s = s->siguiente)
        if (s->activo && s->alcance == nivel_actual &&
            texto_igual(s->nombre, nombre) &&
            !(encontrado != NULL && encontrado->categoria == SIMBOLO_FUNCION &&
              encontrado->tiene_prototipo && !s->tiene_prototipo))
            encontrado = s;
    return encontrado;
}

int firmas_iguales(const Simbolo *a, const Simbolo *b)
{
    const Parametros *pa, *pb;
    if (a == NULL || b == NULL || a->categoria != SIMBOLO_FUNCION ||
        b->categoria != SIMBOLO_FUNCION || !texto_igual(a->tipo, b->tipo) ||
        a->tiene_prototipo != b->tiene_prototipo ||
        a->cant_parametros != b->cant_parametros)
        return 0;
    for (pa = a->parametros, pb = b->parametros;
         pa != NULL && pb != NULL; pa = pa->siguiente, pb = pb->siguiente)
        if (!texto_igual(pa->tipo, pb->tipo))
            return 0;
    return pa == NULL && pb == NULL;
}

/* Compatibilidad C90 de un prototipo con una declaracion f(): los tipos
 * deben sobrevivir sin cambios a las promociones por defecto. No se admiten
 * definiciones K&R con parametros; requieren otra representacion en el parser.
 */
static int funciones_compatibles(const Simbolo *a, const Simbolo *b)
{
    const Simbolo *prototipo, *sin_prototipo;
    const Parametros *p;
    if (!texto_igual(a->tipo, b->tipo))
        return 0;
    if (a->tiene_prototipo == b->tiene_prototipo)
        return firmas_iguales(a, b);
    prototipo = a->tiene_prototipo ? a : b;
    sin_prototipo = a->tiene_prototipo ? b : a;
    if (sin_prototipo->definicion && prototipo->cant_parametros != 0)
        return 0;
    for (p = prototipo->parametros; p != NULL; p = p->siguiente)
        if (texto_igual(p->tipo, "float") || texto_igual(p->tipo, "char") ||
            texto_igual(p->tipo, "signed char") ||
            texto_igual(p->tipo, "unsigned char") ||
            texto_igual(p->tipo, "short") || texto_igual(p->tipo, "unsigned short"))
            return 0;
    return 1;
}

static EstadoTS validar_candidato(const Simbolo *s)
{
    const Parametros *p, *q;
    int cantidad = 0;
    if (s == NULL || s->en_tabla || s->nombre == NULL || *s->nombre == '\0' ||
        s->tipo == NULL || *s->tipo == '\0' || s->alcance != nivel_actual ||
        (s->categoria != SIMBOLO_VARIABLE && s->categoria != SIMBOLO_FUNCION))
        return TS_ARGUMENTO_INVALIDO;
    if (s->categoria == SIMBOLO_VARIABLE)
        return s->parametros == NULL && s->cant_parametros == 0 &&
               !texto_igual(s->tipo, "void") ? TS_OK : TS_ARGUMENTO_INVALIDO;
    if (!s->tiene_prototipo && (s->parametros != NULL || s->cant_parametros != 0))
        return TS_PARAMETRO_INVALIDO;
    for (p = s->parametros; p != NULL; p = p->siguiente) {
        if (p->tipo == NULL || *p->tipo == '\0' || texto_igual(p->tipo, "void") ||
            (s->definicion && (p->nombre == NULL || *p->nombre == '\0')))
            return TS_PARAMETRO_INVALIDO;
        for (q = s->parametros; q != p; q = q->siguiente)
            if (p->nombre != NULL && *p->nombre != '\0' &&
                texto_igual(p->nombre, q->nombre))
                return TS_PARAMETRO_DUPLICADO;
        if (cantidad == INT_MAX)
            return TS_PARAMETRO_INVALIDO;
        cantidad++;
    }
    return cantidad == s->cant_parametros ? TS_OK : TS_PARAMETRO_INVALIDO;
}

EstadoTS registrar_simbolo(Simbolo *s, DiagnosticoTS *diagnostico)
{
    Simbolo *previo = NULL, *p, *definicion = NULL;
    EstadoTS estado = validar_candidato(s);
    if (estado == TS_OK) {
        previo = buscar_simbolo_en_alcance_actual(s->nombre);
        if (previo != NULL) {
            if (previo->categoria != s->categoria)
                estado = TS_DISTINTA_CATEGORIA;
            else if (s->categoria == SIMBOLO_VARIABLE) {
                estado = texto_igual(previo->tipo, s->tipo) &&
                         previo->es_const == s->es_const ?
                         TS_REDECLARACION_VARIABLE : TS_CONFLICTO_TIPOS;
            } else {
                /* Comparar con TODAS las declaraciones visibles de la misma
                 * entidad: f() no puede borrar restricciones de un prototipo.
                 */
                for (p = inicio; p != NULL; p = p->siguiente) {
                    if (!p->activo || p->alcance != nivel_actual ||
                        !texto_igual(p->nombre, s->nombre))
                        continue;
                    if (!funciones_compatibles(p, s)) {
                        previo = p;
                        estado = TS_CONFLICTO_TIPOS;
                        break;
                    }
                    if (p->definicion)
                        definicion = p;
                }
                if (estado == TS_OK && s->definicion && definicion != NULL) {
                    previo = definicion;
                    estado = TS_REDEFINICION_FUNCION;
                }
            }
        }
    }
    if (diagnostico != NULL) {
        diagnostico->estado = estado;
        diagnostico->previo = previo;
    }
    if (estado != TS_OK)
        return estado;
    s->reportable = previo == NULL ||
                    (s->categoria == SIMBOLO_FUNCION && s->definicion);
    s->activo = 1;
    s->en_tabla = 1;
    s->siguiente = NULL;
    if (fin == NULL)
        inicio = s;
    else
        fin->siguiente = s;
    fin = s;
    /* Todos los registros de esta entidad quedan activos. La busqueda da
     * prioridad a los prototipos, y el registro conserva las definiciones
     * incluso si una declaracion f() posterior no aporta informacion.
     */
    return TS_OK;
}

Simbolo *insertar_simbolo(Simbolo *s)
{
    return registrar_simbolo(s, NULL) == TS_OK ? s : NULL;
}

Simbolo *primer_simbolo(void)
{
    return inicio;
}

void entrar_alcance(void)
{
    if (nivel_actual == INT_MAX)
        error_fatal("Demasiados alcances anidados");
    nivel_actual++;
}

void salir_alcance(void)
{
    Simbolo *s;
    if (nivel_actual == 0)
        return;
    /* Retener los registros para el reporte y las referencias de diagnosticos.
     * Un bloque hermano no reactiva declaraciones del bloque anterior.
     */
    for (s = inicio; s != NULL; s = s->siguiente)
        if (s->alcance == nivel_actual)
            s->activo = 0;
    nivel_actual--;
}

int alcance_actual(void)
{
    return nivel_actual;
}

int tipo_es_aritmetico(const char *tipo)
{
    static const char *const tipos[] = {
        "char", "signed char", "unsigned char", "short", "unsigned short",
        "int", "unsigned int", "long", "unsigned long", "float", "double",
        "long double"
    };
    size_t i;
    for (i = 0; i < sizeof(tipos) / sizeof(tipos[0]); i++)
        if (texto_igual(tipo, tipos[i]))
            return 1;
    return 0;
}

int tipos_asignables(const char *destino, const char *origen)
{
    /* Solo tipos aritmeticos simples: void no es un valor; ni un identificador
     * de funcion ni una cadena deben pasar su retorno como tipo de expresion.
     * El parser debe distinguirlos de una llamada y de una variable.
     */
    return tipo_es_aritmetico(destino) && tipo_es_aritmetico(origen);
}

static void imprimir_tipo(FILE *salida, const Simbolo *s)
{
    const Parametros *p;
    if (s->es_const && s->categoria == SIMBOLO_VARIABLE)
        fputs("const ", salida);
    fputs(s->tipo, salida);
    if (s->categoria != SIMBOLO_FUNCION)
        return;
    fputc('(', salida);
    if (s->parametros == NULL && s->tiene_prototipo)
        fputs("void", salida);
    for (p = s->parametros; p != NULL; p = p->siguiente) {
        if (p != s->parametros)
            fputs(", ", salida);
        fputs(p->tipo, salida);
    }
    fputc(')', salida);
}

void imprimir_error_declaracion(FILE *salida, const Simbolo *s,
                                const DiagnosticoTS *d)
{
    const Simbolo *previo;
    if (salida == NULL || s == NULL || s->nombre == NULL || s->tipo == NULL ||
        d == NULL || d->estado == TS_OK)
        return;
    previo = d->previo;
    fprintf(salida, "%d:%d: ", s->linea, s->columna);
    switch (d->estado) {
    case TS_DISTINTA_CATEGORIA:
        fprintf(salida, "'%s' redeclarado como un tipo diferente de simbolo\n", s->nombre);
        break;
    case TS_CONFLICTO_TIPOS:
        fprintf(salida, "conflicto de tipos para '%s'; la ultima es de tipo '", s->nombre);
        imprimir_tipo(salida, s);
        fputs("'\n", salida);
        break;
    case TS_REDECLARACION_VARIABLE:
        fprintf(salida, "Redeclaracion de '%s'\n", s->nombre);
        break;
    case TS_REDEFINICION_FUNCION:
        fprintf(salida, "Redefinicion de '%s'\n", s->nombre);
        break;
    case TS_PARAMETRO_INVALIDO:
        fprintf(salida, "Parametro invalido en '%s'\n", s->nombre);
        break;
    case TS_PARAMETRO_DUPLICADO:
        fprintf(salida, "Parametro duplicado en '%s'\n", s->nombre);
        break;
    default:
        fputs("Declaracion invalida\n", salida);
        break;
    }
    if (previo != NULL) {
        fprintf(salida, "Nota: la %s previa de '%s' es de tipo '",
                d->estado == TS_REDEFINICION_FUNCION ? "definicion" : "declaracion",
                previo->nombre);
        imprimir_tipo(salida, previo);
        fprintf(salida, "': %d:%d\n", previo->linea, previo->columna);
    }
}

void imprimir_variables(FILE *salida)
{
    const Simbolo *s;
    int hay = 0;
    if (salida == NULL)
        return;
    fputs("* Listado de variables declaradas (tipo de dato y numero de linea):\n", salida);
    for (s = inicio; s != NULL; s = s->siguiente) {
        if (!s->reportable || s->categoria != SIMBOLO_VARIABLE)
            continue;
        fprintf(salida, "%s: ", s->nombre);
        imprimir_tipo(salida, s);
        fprintf(salida, ", linea %d, columna %d\n", s->linea, s->columna);
        hay = 1;
    }
    if (!hay)
        fputs("-\n", salida);
}

void imprimir_funciones(FILE *salida)
{
    const Simbolo *s;
    const Parametros *p;
    int hay = 0;
    if (salida == NULL)
        return;
    fputs("* Listado de funciones declaradas y definidas:\n", salida);
    for (s = inicio; s != NULL; s = s->siguiente) {
        if (!s->reportable || s->categoria != SIMBOLO_FUNCION)
            continue;
        fprintf(salida, "%s: %s, input: ", s->nombre,
                s->definicion ? "definicion" : "declaracion");
        if (s->parametros == NULL)
            fputs(s->tiene_prototipo ? "void" : "sin especificar", salida);
        for (p = s->parametros; p != NULL; p = p->siguiente) {
            if (p != s->parametros)
                fputs(", ", salida);
            fputs(p->tipo, salida);
            if (p->nombre != NULL && *p->nombre != '\0')
                fprintf(salida, " %s", p->nombre);
        }
        fprintf(salida, ", retorna: %s, linea %d\n", s->tipo, s->linea);
        hay = 1;
    }
    if (!hay)
        fputs("-\n", salida);
}

void liberar_tabla(void)
{
    Simbolo *s, *siguiente;
    for (s = inicio; s != NULL; s = siguiente) {
        siguiente = s->siguiente;
        destruir_registro(s);
    }
    inicio = fin = NULL;
    nivel_actual = 0;
}
