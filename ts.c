#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "ts.h"

static Simbolo *inicio = NULL;
static Simbolo *fin = NULL;
static int nivel_actual = 0;

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
    s->reportable = 1;
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

Simbolo *finalizar_declaracion(Simbolo *s, int aceptada)
{
    if (s == NULL || s->en_tabla)
        return NULL;
    if (!aceptada) {
        destruir_simbolo(s);
        return NULL;
    }
    /* Si el alcance no coincide, insertar devuelve NULL y el llamador
     * conserva el candidato. No confundir fallo de API con rechazo semantico.
     */
    return insertar_simbolo(s);
}

int agregar_parametro_ubicado(Simbolo *f, const char *tipo,
                              const char *nombre, int linea, int columna)
{
    Parametros *p, *ultimo;
    if (f == NULL || f->en_tabla || f->categoria != SIMBOLO_FUNCION ||
        tipo == NULL || *tipo == '\0' || f->cant_parametros == INT_MAX)
        return 0;
    p = (Parametros *) calloc(1, sizeof(*p));
    if (p == NULL)
        error_fatal("Sin memoria");
    p->nombre = copiar_texto(nombre != NULL && *nombre != '\0' ? nombre : NULL);
    p->tipo = copiar_texto(tipo);
    p->linea = linea;
    p->columna = columna;
    if (f->parametros == NULL) {
        f->parametros = p;
    } else {
        ultimo = f->parametros;
        while (ultimo->siguiente != NULL)
            ultimo = ultimo->siguiente;
        ultimo->siguiente = p;
    }
    f->cant_parametros++;
    return 1;
}

int agregar_parametro(Simbolo *f, const char *tipo, const char *nombre)
{
    return agregar_parametro_ubicado(f, tipo, nombre,
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

Simbolo *insertar_simbolo(Simbolo *s)
{
    Simbolo *previo;
    /* Solo proteger la lista. La parte 3 decide si la declaracion es valida. */
    if (s == NULL || s->en_tabla || s->alcance != nivel_actual)
        return NULL;
    if (s->categoria == SIMBOLO_FUNCION && !s->definicion) {
        previo = buscar_simbolo_en_alcance_actual(s->nombre);
        if (previo != NULL && previo->categoria == SIMBOLO_FUNCION)
            s->reportable = 0;
    }
    s->activo = 1;
    s->en_tabla = 1;
    s->siguiente = NULL;
    if (fin == NULL)
        inicio = s;
    else
        fin->siguiente = s;
    fin = s;
    return s;
}

Simbolo *buscar_definicion_en_alcance_actual(const char *nombre)
{
    Simbolo *s;
    for (s = inicio; s != NULL; s = s->siguiente)
        if (s->activo && s->alcance == nivel_actual &&
            s->categoria == SIMBOLO_FUNCION && s->definicion &&
            texto_igual(s->nombre, nombre))
            return s;
    return NULL;
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

int cargar_parametros_en_alcance(const Simbolo *funcion)
{
    const Parametros *p;
    Simbolo *variable;
    if (funcion == NULL || funcion->categoria != SIMBOLO_FUNCION ||
        !funcion->definicion || funcion->alcance == INT_MAX ||
        nivel_actual != funcion->alcance + 1)
        return 0;
    /* Revisar todos los datos antes de cambiar la tabla. No reemplaza los
     * controles de declaraciones que debe hacer la parte 3.
     */
    for (p = funcion->parametros; p != NULL; p = p->siguiente)
        if (p->nombre == NULL || *p->nombre == '\0' ||
            p->tipo == NULL || *p->tipo == '\0')
            return 0;
    for (p = funcion->parametros; p != NULL; p = p->siguiente) {
        variable = crear_simbolo(p->nombre, SIMBOLO_VARIABLE, p->tipo,
                                 p->linea, p->columna);
        variable->reportable = 0;
        variable->es_const = p->es_const;
        /* El nivel no cambia durante la copia; el candidato es nuevo. */
        insertar_simbolo(variable);
    }
    return 1;
}

static char *tipo_funcion_texto(const Simbolo *f, int como_valor)
{
    const Parametros *p;
    const char *apertura = como_valor ? " (*)(" : "(";
    size_t tamanio;
    char *texto;
    if (f == NULL || f->categoria != SIMBOLO_FUNCION || f->tipo == NULL)
        return NULL;
    tamanio = strlen(f->tipo) + strlen(apertura) + 2; /* ')' y terminador. */
    if (f->parametros == NULL && f->tiene_prototipo)
        tamanio += strlen("void");
    for (p = f->parametros; p != NULL; p = p->siguiente) {
        if (p->tipo == NULL)
            return NULL;
        if (p != f->parametros)
            tamanio += 2; /* ', ' */
        tamanio += strlen(p->tipo);
    }
    texto = (char *) malloc(tamanio);
    if (texto == NULL)
        error_fatal("Sin memoria");
    strcpy(texto, f->tipo);
    strcat(texto, apertura);
    if (f->parametros == NULL && f->tiene_prototipo)
        strcat(texto, "void");
    for (p = f->parametros; p != NULL; p = p->siguiente) {
        if (p != f->parametros)
            strcat(texto, ", ");
        strcat(texto, p->tipo);
    }
    strcat(texto, ")");
    return texto;
}

char *tipo_variable_texto(const Simbolo *s, int incluir_const)
{
    size_t tamanio;
    char *texto;
    if (s == NULL || s->categoria != SIMBOLO_VARIABLE || s->tipo == NULL)
        return NULL;
    if (!incluir_const || !s->es_const)
        return copiar_texto(s->tipo);
    tamanio = strlen(s->tipo) + sizeof("const ");
    texto = (char *) malloc(tamanio);
    if (texto == NULL)
        error_fatal("Sin memoria");
    strcpy(texto, "const ");
    strcat(texto, s->tipo);
    return texto;
}

char *firma_funcion_texto(const Simbolo *f)
{
    return tipo_funcion_texto(f, 0);
}

char *tipo_valor_texto(const Simbolo *s)
{
    if (s == NULL)
        return NULL;
    if (s->categoria == SIMBOLO_FUNCION)
        return tipo_funcion_texto(s, 1);
    return tipo_variable_texto(s, 0);
}

static void imprimir_tipo_variable(FILE *salida, const Simbolo *s)
{
    if (s->es_const)
        fputs("const ", salida);
    fputs(s->tipo, salida);
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
        imprimir_tipo_variable(salida, s);
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
            if (p->es_const)
                fputs("const ", salida);
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
