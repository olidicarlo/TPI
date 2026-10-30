#ifndef GENERAL_H
#define GENERAL_H

#include <stdio.h>

/* Compatible con %locations y con la cabecera generada por Bison. */
#if !defined YYLTYPE && !defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE {
    int first_line;
    int first_column;
    int last_line;
    int last_column;
} YYLTYPE;
#define YYLTYPE_IS_DECLARED 1
#define YYLTYPE_IS_TRIVIAL 1
#endif

#define INICIO_CONTEO_LINEA 1
#define INICIO_CONTEO_COLUMNA 1

void pausa(void);
void inicializarUbicacion(void);
void reinicializarUbicacion(void);

typedef enum {
    SIMBOLO_VARIABLE,
    SIMBOLO_FUNCION
} CategoriaSimbolo;

typedef struct Parametros {
    char *nombre;                 /* NULL en un prototipo sin identificador. */
    char *tipo;
    int linea;
    int columna;
    struct Parametros *siguiente;
} Parametros;

typedef struct Simbolo {
    char *nombre;
    char *tipo;                   /* Tipo de variable o retorno de funcion. */
    int linea;
    int columna;
    CategoriaSimbolo categoria;
    int definicion;
    int es_const;
    int alcance;
    Parametros *parametros;
    int cant_parametros;
    struct Simbolo *siguiente;    /* Orden de aparicion; incluye historial. */
    int activo;                  /* Visible; no equivale a estar en el reporte. */
    int reportable;
    int en_tabla;
    int tiene_prototipo;         /* 1: f(void)/f(int); 0: f() en C89/C90. */
} Simbolo;

typedef enum {
    TS_OK,
    TS_ARGUMENTO_INVALIDO,
    TS_DISTINTA_CATEGORIA,
    TS_CONFLICTO_TIPOS,
    TS_REDECLARACION_VARIABLE,
    TS_REDEFINICION_FUNCION,
    TS_PARAMETRO_INVALIDO,
    TS_PARAMETRO_DUPLICADO
} EstadoTS;

typedef struct DiagnosticoTS {
    EstadoTS estado;
    const Simbolo *previo;        /* Prestado; valido hasta liberar_tabla(). */
} DiagnosticoTS;

/* La tabla copia nombre/tipo. El parser debe pasar tipos simples canonicos:
 * int, unsigned int, long, unsigned long, float, double, etc. Normalizar en
 * las acciones de especificadores: signed == signed int == int, long int ==
 * long, etc. const se representa con es_const, separado del tipo base.
 * No se modelan aqui punteros, arreglos, variadicas ni otros namespaces.
 * Fallos de reserva terminan el proceso con un mensaje en stderr.
 */
Simbolo *crear_simbolo(const char *nombre, CategoriaSimbolo categoria,
                       const char *tipo, int linea, int columna);
/* Solo destruye candidatos NO registrados. La tabla es duena de los insertados. */
void destruir_simbolo(Simbolo *s);
/* Armar completamente el candidato antes de registrarlo (parametros,
 * definicion, es_const y tiene_prototipo). Si tiene exito, transfiere propiedad
 * a la tabla; si falla, el llamador debe destruir el candidato.
 * Las redeclaraciones compatibles de funciones se conservan para resolver
 * visibilidad, pero no duplican su entrada en el reporte.
 * No imprime: permite al parser acumular errores en su orden de aparicion.
 */
EstadoTS registrar_simbolo(Simbolo *s, DiagnosticoTS *diagnostico);
/* Atajo sin diagnostico; NULL ante rechazo. Misma regla de propiedad. */
Simbolo *insertar_simbolo(Simbolo *s);
/* Solo para candidatos: marcar antes de registrar para validar redefinicion. */
void marcar_definido(Simbolo *s);
EstadoTS agregar_parametro_ubicado(Simbolo *f, const char *tipo,
                                  const char *nombre, int linea, int columna);
/* Compatibilidad con la API original; preferir la variante con estado. */
void agregar_parametro(Simbolo *f, const char *tipo, const char *nombre);
/* f(void): cero parametros, tiene_prototipo=1 (valor inicial).
 * f(): cero parametros, tiene_prototipo=0. No agregar un parametro void.
 * En definiciones, cada parametro real debe tener nombre, sin duplicados.
 */
Parametros *obtener_parametro(const Simbolo *f, int n); /* Indice desde cero. */
Simbolo *buscar_simbolo(const char *nombre);
Simbolo *buscar_simbolo_en_alcance_actual(const char *nombre);
/* Recorre el historial; filtrar reportable para el reporte y activo para
 * visibilidad. Los punteros se mantienen validos hasta liberar_tabla(). */
Simbolo *primer_simbolo(void);
void entrar_alcance(void);
void salir_alcance(void);
int alcance_actual(void);
/* Igualdad de firmas con prototipo: ignora nombres y ubicaciones. */
int firmas_iguales(const Simbolo *a, const Simbolo *b);
/* Tipos aritmeticos simples compatibles por conversion en asignaciones,
 * argumentos y return. Esto NO compara firmas de declaraciones. */
int tipo_es_aritmetico(const char *tipo);
int tipos_asignables(const char *destino, const char *origen);
void imprimir_error_declaracion(FILE *salida, const Simbolo *candidato,
                                const DiagnosticoTS *diagnostico);
void imprimir_variables(FILE *salida);
void imprimir_funciones(FILE *salida);
void liberar_tabla(void);

#endif
