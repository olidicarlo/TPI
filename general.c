/* En los archivos (*.c) se pueden poner tanto DECLARACIONES como DEFINICIONES de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "general.h"

extern YYLTYPE yylloc;

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

static Simbolo *inicio = NULL;
static Simbolo *fin = NULL;
static int nivel_actual = 0;

static void sin_memoria(void)
{
    fprintf(stderr, "Sin memoria\n");
    exit(EXIT_FAILURE);
}

static char *copiar_texto(const char *s)
{
    char *p;

    if (s == NULL)
        return NULL;
    p = (char *) malloc(strlen(s) + 1);
    if (p == NULL)
        sin_memoria();
    strcpy(p, s);
    return p;
}

Simbolo *crear_simbolo(const char *nombre, CategoriaSimbolo categoria,
                       const char *tipo, int linea, int columna)
{
    Simbolo *s = (Simbolo *) calloc(1, sizeof(Simbolo));

    if (s == NULL)
        sin_memoria();
    s->nombre = copiar_texto(nombre);
    s->tipo = copiar_texto(tipo);
    s->categoria = categoria;
    s->linea = linea;
    s->columna = columna;
    s->alcance = nivel_actual;
    return s;
}

void destruir_simbolo(Simbolo *s)
{
    Parametros *p, *sig;

    if (s == NULL)
        return;
    for (p = s->parametros; p != NULL; p = sig) {
        sig = p->siguiente;
        free(p->tipo);
        free(p->nombre);
        free(p);
    }
    free(s->nombre);
    free(s->tipo);
    free(s);
}

Simbolo *insertar_simbolo(Simbolo *s)
{
    if (s == NULL)
        return NULL;
    s->siguiente = NULL;
    if (fin != NULL)
        fin->siguiente = s;
    else
        inicio = s;
    fin = s;
    return s;
}

void marcar_definido(Simbolo *s)
{
    if (s != NULL)
        s->definicion = 1;
}

void agregar_parametro(Simbolo *f, const char *tipo, const char *nombre)
{
    Parametros *p;
    Parametros *ultimo;

    if (f == NULL)
        return;
    p = (Parametros *) calloc(1, sizeof(Parametros));
    if (p == NULL)
        sin_memoria();
    p->tipo = copiar_texto(tipo);
    p->nombre = copiar_texto(nombre);

    if (f->parametros == NULL) {
        f->parametros = p;
    } else {
        for (ultimo = f->parametros; ultimo->siguiente != NULL;
             ultimo = ultimo->siguiente)
            ;
        ultimo->siguiente = p;
    }
    f->cant_parametros++;
}

Simbolo *buscar_simbolo(const char *nombre)
{
    Simbolo *actual;
    Simbolo *mejor = NULL;

    for (actual = inicio; actual != NULL; actual = actual->siguiente) {
        if (strcmp(actual->nombre, nombre) == 0) {
            if (mejor == NULL || actual->alcance >= mejor->alcance)
                mejor = actual;
        }
    }
    return mejor;
}

Simbolo *buscar_simbolo_en_alcance_actual(const char *nombre)
{
    Simbolo *actual;

    for (actual = inicio; actual != NULL; actual = actual->siguiente)
        if (actual->alcance == nivel_actual &&
            strcmp(actual->nombre, nombre) == 0)
            return actual;
    return NULL;
}

Parametros *obtener_parametro(const Simbolo *f, int n)
{
    Parametros *p;
    int i;

    if (f == NULL || n < 0)
        return NULL;
    p = f->parametros;
    for (i = 0; p != NULL && i < n; i++)
        p = p->siguiente;
    return p;
}

Simbolo *primer_simbolo(void)
{
    return inicio;
}


void entrar_alcance(void)
{
    nivel_actual++;
}


void salir_alcance(void)
{
    Simbolo *actual = inicio;
    Simbolo *anterior = NULL;
    Simbolo *siguiente;

    if (nivel_actual == 0)
        return;

    while (actual != NULL) {
        siguiente = actual->siguiente;
        if (actual->alcance == nivel_actual) {
            if (anterior != NULL)
                anterior->siguiente = siguiente;
            else
                inicio = siguiente;
            if (actual == fin)
                fin = anterior;
            destruir_simbolo(actual);
        } else {
            anterior = actual;
        }
        actual = siguiente;
    }
    nivel_actual--;
}

int alcance_actual(void)
{
    return nivel_actual;
}

int firmas_iguales(const Simbolo *a, const Simbolo *b)
{
    Parametros *pa, *pb;

    if(strcmp(a->tipo, b->tipo) != 0) return 0;
    if(a->cant_parametros != b->cant_parametros) return 0;

    for (pa = a->parametros, pb = b->parametros; pa && pb;
         pa = pa->siguiente, pb = pb->siguiente){

            if (strcmp(pa->tipo, pb->tipo) != 0) 
            return 0;   
        }

    return 1;
}

void liberar_tabla(void)
{
    Simbolo *s = inicio, *sig;
    while(s != NULL){
        sig = s->siguiente;
        destruir_simbolo(s);
        s = sig;
    }
    inicio = fin = NULL;
    nivel_actual = 0;
}