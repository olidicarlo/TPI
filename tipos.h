#ifndef TIPOS_H
#define TIPOS_H

/* Auxiliares compartidos para las partes 3, 4 y 5. No registran errores.
 * Reciben tipos BASE canonicos obtenidos de la TS o de una expresion.
 * Ejemplos: signed int debe normalizarse a int; const se guarda por separado.
 * Se limitan a los tipos aritmeticos simples del TP, no a todo ANSI C.
 */
int tipo_es_numerico(const char *tipo);
/* Conversion aritmetica permitida en inicializacion, asignacion, argumentos
 * y return con valor. No compara firmas, no comprueba valor-L ni const y no
 * calcula el tipo resultante de una operacion. void, cadenas y designadores
 * de funcion no son tipos numericos. Los punteros generales no estan modelados.
 */
int tipos_compatibles_asignacion(const char *destino, const char *origen);

#endif
