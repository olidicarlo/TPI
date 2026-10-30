#ifndef ERRORES_SEMANTICOS_H
#define ERRORES_SEMANTICOS_H

#include <stdio.h>

/* Lista general de la parte 2. Los mensajes los construyen las partes 3, 4 y 5.
 * Cada nota pertenece a un error y se imprime junto a el, no como otro error.
 */
typedef struct ErrorSemantico {
    int linea;
    int columna;
    char *mensaje;
    char *nota;                  /* NULL si no hay nota. */
    struct ErrorSemantico *siguiente;
} ErrorSemantico;

/* Copia mensaje y nota: no guarda referencias a simbolos ni a buffers ajenos.
 * mensaje no incluye L:C; nota no incluye el prefijo "Nota: ".
 * Los textos se pasan terminados en '\0', sin salto de linea final.
 * Se conserva el orden en que las rutinas semanticas registran los errores,
 * como en el ejemplo del enunciado: un error de argumento puede aparecer
 * antes que el de la llamada que lo contiene. No se reordenan por columna.
 * La nota queda unida a su mensaje aunque refiera a otra linea.
 * Devuelve 1 si se agrego, 0 si falta mensaje o la ubicacion no es positiva.
 * Sin memoria: mensaje por stderr y terminacion, igual que en la TS.
 * No detecta errores, no compara tipos y no elimina duplicados.
 */
int agregar_error_semantico(int linea, int columna,
                            const char *mensaje, const char *nota);
/* Puntero prestado para consultas. Valido hasta liberar_errores_semanticos(). */
const ErrorSemantico *primer_error_semantico(void);
void imprimir_errores_semanticos(FILE *salida);
void liberar_errores_semanticos(void);

/* Integracion del reporte (parte 1):
 * imprimir_variables(salida);
 * imprimir_funciones(salida);
 * imprimir_errores_semanticos(salida);
 * ...listados sintacticos y lexicos que aporta la parte 5, en ese orden.
 * Para stdout usar esas llamadas al final, no imprimir errores al detectarlos.
 * Liberar ambas listas despues del reporte; son independientes entre si.
 */
#endif
