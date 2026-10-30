#ifndef GENERAL_H
#define GENERAL_H

/* En los archivos de cabecera (header files) (*.h) poner DECLARACIONES (evitar DEFINICIONES) de C, así como directivas de preprocesador */
/* Recordar solamente indicar archivos *.h en las directivas de preprocesador #include, nunca archivos *.c */

#define YYLTYPE YYLTYPE

typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;

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
    char *nombre;              
    char *tipo;
    struct Parametros *siguiente;
} Parametros;
 
typedef struct Simbolo {
    char *nombre;
    char *tipo;                  
    int linea;
    int columna;
 
    CategoriaSimbolo categoria;
    int definicion;               
    int es_const;                
    int alcance;                 
 
    Parametros *parametros;
    int cant_parametros;
    struct Simbolo *siguiente;
} Simbolo;

#endif