#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "errores_semanticos.h"

static ErrorSemantico *inicio_errores = NULL;
static ErrorSemantico *fin_errores = NULL;

static char *copiar_texto(const char *texto)
{
    char *copia;
    if (texto == NULL)
        return NULL;
    copia = (char *) malloc(strlen(texto) + 1);
    if (copia == NULL) {
        fprintf(stderr, "Sin memoria\n");
        exit(EXIT_FAILURE);
    }
    strcpy(copia, texto);
    return copia;
}

int agregar_error_semantico(int linea, int columna,
                            const char *mensaje, const char *nota)
{
    ErrorSemantico *nuevo;
    if (linea < 1 || columna < 1 || mensaje == NULL || *mensaje == '\0')
        return 0;
    nuevo = (ErrorSemantico *) malloc(sizeof(*nuevo));
    if (nuevo == NULL) {
        fprintf(stderr, "Sin memoria\n");
        exit(EXIT_FAILURE);
    }
    nuevo->linea = linea;
    nuevo->columna = columna;
    nuevo->mensaje = copiar_texto(mensaje);
    nuevo->nota = copiar_texto(nota != NULL && *nota != '\0' ? nota : NULL);
    nuevo->siguiente = NULL;
    if (fin_errores == NULL)
        inicio_errores = nuevo;
    else
        fin_errores->siguiente = nuevo;
    fin_errores = nuevo;
    return 1;
}

const ErrorSemantico *primer_error_semantico(void)
{
    return inicio_errores;
}

void imprimir_errores_semanticos(FILE *salida)
{
    const ErrorSemantico *error;
    if (salida == NULL)
        return;
    fputs("* Listado de errores semanticos:\n", salida);
    if (inicio_errores == NULL) {
        fputs("-\n", salida);
        return;
    }
    for (error = inicio_errores; error != NULL; error = error->siguiente) {
        fprintf(salida, "%d:%d: %s\n", error->linea, error->columna,
                error->mensaje);
        if (error->nota != NULL)
            fprintf(salida, "Nota: %s\n", error->nota);
    }
}

void liberar_errores_semanticos(void)
{
    ErrorSemantico *actual = inicio_errores;
    while (actual != NULL) {
        ErrorSemantico *siguiente = actual->siguiente;
        free(actual->mensaje);
        free(actual->nota);
        free(actual);
        actual = siguiente;
    }
    inicio_errores = NULL;
    fin_errores = NULL;
}
