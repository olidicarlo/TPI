#ifndef TS_H
#define TS_H

#include <stdio.h>

/* Parte 2: almacenamiento, consultas, firmas y listados.
 * La parte 3 decide que declaraciones son validas ANTES de insertarlas.
 * Las partes 3, 4 y 5 detectan errores y llaman a errores_semanticos.h.
 * Este modulo no valida return, invocaciones, asignaciones ni operadores.
 */
typedef enum {
    SIMBOLO_VARIABLE,
    SIMBOLO_FUNCION
} CategoriaSimbolo;

typedef struct Parametros {
    char *nombre;                /* NULL si el prototipo omite el nombre. */
    char *tipo;
    int linea;
    int columna;
    struct Parametros *siguiente;
} Parametros;

typedef struct Simbolo {
    char *nombre;
    char *tipo;                  /* Tipo de variable o retorno de funcion. */
    int linea;
    int columna;
    CategoriaSimbolo categoria;
    int definicion;
    int es_const;
    int alcance;
    Parametros *parametros;
    int cant_parametros;
    struct Simbolo *siguiente;   /* Orden de insercion, incluido el historial. */
    int activo;                 /* Participa en las busquedas. */
    int reportable;             /* Aparece en los listados aunque este inactivo. */
    int en_tabla;               /* La tabla es duena de su memoria. */
    int tiene_prototipo;        /* 1: f(void)/f(int); 0: f() en C90. */
} Simbolo;

/* Copia nombre/tipo. NULL si faltan datos o la categoria es invalida.
 * Ante falta de memoria se informa por stderr y se termina el proceso.
 * Las rutinas semanticas pasan tipos canonicos: int, long, unsigned int, etc.
 * const se guarda aparte en es_const; no se normalizan tipos en la TS.
 */
Simbolo *crear_simbolo(const char *nombre, CategoriaSimbolo categoria,
                       const char *tipo, int linea, int columna);
/* Solo destruye candidatos no insertados. Los insertados se liberan juntos. */
void destruir_simbolo(Simbolo *s);
/* Inserta SIN comprobar duplicados, conflictos de tipos ni redefiniciones.
 * Devuelve s y toma su propiedad; NULL si es nulo, ya insertado o su alcance
 * no coincide con el actual. Si un candidato no insertado es rechazado,
 * sigue siendo del llamador; un registro ya insertado sigue siendo de la TS.
 * Preparar todos los atributos antes de insertar.
 * reportable vale 1 al crear. La parte 3 lo pone en 0 para prototipos
 * compatibles repetidos y para parametros registrados como variables locales.
 * No modificar los enlaces ni las cadenas de los registros insertados.
 */
Simbolo *insertar_simbolo(Simbolo *s);
void marcar_definido(Simbolo *s); /* Solo candidatos de funcion no insertados. */
/* Copian un parametro y conservan el orden. 1: agregado; 0: uso invalido
 * de la API. No detectan nombres duplicados ni errores de tipos: parte 3.
 * Usar la variante ubicada para conservar la posicion real del parametro.
 * f(void): lista vacia, tiene_prototipo=1 (inicial). f(): lista vacia,
 * tiene_prototipo=0. Las acciones semanticas representan asi estos casos;
 * no deben agregar un nodo void como parametro real.
 */
int agregar_parametro_ubicado(Simbolo *f, const char *tipo,
                              const char *nombre, int linea, int columna);
int agregar_parametro(Simbolo *f, const char *tipo, const char *nombre);
Parametros *obtener_parametro(const Simbolo *f, int n); /* Indice desde 0. */

/* Devuelven punteros prestados: no liberarlos ni modificar sus enlaces.
 * buscar_simbolo elige el alcance visible mas cercano. En el mismo alcance,
 * se prefiere un prototipo a una declaracion sin parametros especificados.
 * buscar_simbolo_en_alcance_actual no considera bloques exteriores.
 */
Simbolo *buscar_simbolo(const char *nombre);
Simbolo *buscar_simbolo_en_alcance_actual(const char *nombre);
/* Consulta para que la parte 3 compruebe redefiniciones aun cuando la ultima
 * declaracion sea un prototipo. No emite ni guarda errores.
 */
Simbolo *buscar_definicion_en_alcance_actual(const char *nombre);
/* Historial completo. Filtrar activo para visibilidad y reportable para
 * listados. Valido hasta liberar_tabla(). */
Simbolo *primer_simbolo(void);
/* Igualdad exacta de retorno, prototipo y tipos de parametros ordenados.
 * Ignora nombres/ubicaciones. La parte 3 decide compatibilidad C90 entre
 * f() y un prototipo: esta funcion no realiza promociones de tipos.
 */
int firmas_iguales(const Simbolo *a, const Simbolo *b);

/* Alcances opcionales: si el equipo usa solo globales, no llamar a entrar.
 * La parte 1/3 equilibra entradas y salidas del parser. Al salir se desactivan
 * registros, sin borrarlos del historial requerido por el reporte.
 */
void entrar_alcance(void);
void salir_alcance(void);
int alcance_actual(void);
/* Imprimen cada encabezado y sus elementos o '-' si no hay. No modifican la
 * tabla. El parser debe insertar en orden de aparicion, de izquierda a derecha.
 */
void imprimir_variables(FILE *salida);
void imprimir_funciones(FILE *salida);
void liberar_tabla(void); /* Libera tambien los simbolos inactivos. */

#endif
