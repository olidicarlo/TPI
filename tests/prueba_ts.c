/* Desde la raiz del repositorio:
 * gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -I. \
 *     tests/prueba_ts.c ts.c errores_semanticos.c tipos.c -o /tmp/prueba_ts
 * /tmp/prueba_ts
 * Resultado esperado: "Todas las pruebas de TS y reporte pasaron."
 * No necesita Flex, Bison, general.c ni yylloc. No prueba el parser ausente.
 */
#ifdef NDEBUG
#error Estas pruebas necesitan assert habilitado
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ts.h"
#include "errores_semanticos.h"
#include "tipos.h"

static void comprobar_texto(char *obtenido, const char *esperado)
{
    assert(obtenido != NULL);
    assert(strcmp(obtenido, esperado) == 0);
    free(obtenido);
}

static void comprobar_salida(void (*imprimir)(FILE *), const char *esperado)
{
    FILE *archivo = tmpfile();
    size_t i;
    assert(archivo != NULL);
    imprimir(archivo);
    assert(fflush(archivo) == 0);
    assert(fseek(archivo, 0, SEEK_SET) == 0);
    for (i = 0; esperado[i] != '\0'; i++)
        assert(fgetc(archivo) == (unsigned char) esperado[i]);
    assert(fgetc(archivo) == EOF);
    assert(!ferror(archivo));
    assert(fclose(archivo) == 0);
}

static Simbolo *crear_funcion(const char *nombre, const char *retorno,
                              const char *parametro, int linea)
{
    Simbolo *f = crear_simbolo(nombre, SIMBOLO_FUNCION, retorno, linea, 5);
    assert(f != NULL);
    if (parametro != NULL)
        assert(agregar_parametro_ubicado(f, parametro, "x", linea, 15));
    return f;
}

static void probar_formatos_y_tipos(void)
{
    Simbolo *f = crear_funcion("imprimir", "void", NULL, 1);
    Simbolo *variable = crear_simbolo("e", SIMBOLO_VARIABLE, "float", 2, 5);
    assert(variable != NULL);
    variable->es_const = 1;
    comprobar_texto(firma_funcion_texto(f), "void(void)");
    comprobar_texto(tipo_valor_texto(f), "void (*)(void)");
    f->tiene_prototipo = 0;
    comprobar_texto(firma_funcion_texto(f), "void()");
    comprobar_texto(tipo_valor_texto(f), "void (*)()");
    destruir_simbolo(f);
    f = crear_funcion("incremento", "int", "int", 3);
    comprobar_texto(firma_funcion_texto(f), "int(int)");
    comprobar_texto(tipo_valor_texto(f), "int (*)(int)");
    destruir_simbolo(f);
    f = crear_funcion("potencia", "double", "double", 4);
    assert(agregar_parametro_ubicado(f, "unsigned int", NULL, 4, 28));
    comprobar_texto(firma_funcion_texto(f), "double(double, unsigned int)");
    comprobar_texto(tipo_valor_texto(f), "double (*)(double, unsigned int)");
    comprobar_texto(tipo_variable_texto(variable, 1), "const float");
    comprobar_texto(tipo_variable_texto(variable, 0), "float");
    comprobar_texto(tipo_valor_texto(variable), "float");
    assert(tipo_variable_texto(f, 1) == NULL);
    assert(firma_funcion_texto(variable) == NULL);
    assert(tipo_valor_texto(NULL) == NULL);
    assert(tipo_es_numerico("unsigned long"));
    assert(tipo_es_numerico("long double"));
    assert(!tipo_es_numerico("char *"));
    assert(!tipo_es_numerico("void (*)(void)"));
    assert(!tipo_es_numerico(NULL));
    assert(tipos_compatibles_asignacion("float", "int"));
    assert(tipos_compatibles_asignacion("unsigned int", "double"));
    assert(!tipos_compatibles_asignacion("int", "char *"));
    assert(!tipos_compatibles_asignacion("void", "void"));
    destruir_simbolo(f);
    destruir_simbolo(variable);
}

static void probar_rechazados_y_copias(void)
{
    Simbolo *original = crear_funcion("incremento", "int", "int", 9);
    Simbolo *rechazado = crear_funcion("incremento", "long", "long", 10);
    Simbolo *imprimir = crear_funcion("imprimir", "void", NULL, 1);
    Simbolo *colision;
    Simbolo *mismo = crear_funcion("otro", "int", "int", 11);
    char *firma_previa = firma_funcion_texto(original);
    char mensaje[200];
    char nota[200];
    const ErrorSemantico *error;
    assert(firmas_iguales(original, mismo));
    assert(!firmas_iguales(original, rechazado));
    destruir_simbolo(mismo);
    assert(finalizar_declaracion(original, 1) == original);
    assert(finalizar_declaracion(imprimir, 1) == imprimir);
    assert(snprintf(mensaje, sizeof(mensaje),
                    "conflicto de tipos para 'incremento'") > 0);
    assert(snprintf(nota, sizeof(nota),
                    "la declaracion previa es de tipo '%s': 9:5", firma_previa) > 0);
    free(firma_previa);
    assert(agregar_error_semantico(10, 6, mensaje, nota));
    /* La parte 3 YA decidio el rechazo. No insertar y luego destruir. */
    assert(finalizar_declaracion(rechazado, 0) == NULL);
    strcpy(mensaje, "buffer reutilizado");
    strcpy(nota, "otra nota");
    error = primer_error_semantico();
    assert(error != NULL);
    assert(strcmp(error->mensaje, "conflicto de tipos para 'incremento'") == 0);
    assert(strcmp(error->nota, "la declaracion previa es de tipo 'int(int)': 9:5") == 0);
    assert(buscar_simbolo("incremento") == original);
    assert(buscar_simbolo("incremento")->linea == 9);
    colision = crear_simbolo("imprimir", SIMBOLO_VARIABLE, "int", 7, 5);
    assert(colision != NULL);
    assert(finalizar_declaracion(colision, 0) == NULL);
    assert(buscar_simbolo("imprimir") == imprimir);
    comprobar_texto(tipo_valor_texto(buscar_simbolo("imprimir")), "void (*)(void)");
    liberar_tabla();
    /* Los errores son independientes de los simbolos ya liberados. */
    assert(strcmp(primer_error_semantico()->mensaje,
                  "conflicto de tipos para 'incremento'") == 0);
    liberar_errores_semanticos();
}

static void probar_parametros_y_alcances(void)
{
    Simbolo *f = crear_funcion("incremento", "int", "int", 43);
    Simbolo *parametro;
    Simbolo *local;
    marcar_definido(f);
    assert(finalizar_declaracion(f, 1) == f);
    assert(!agregar_parametro(f, "int", "tarde"));
    assert(!cargar_parametros_en_alcance(f));
    entrar_alcance();
    assert(cargar_parametros_en_alcance(f));
    parametro = buscar_simbolo("x");
    assert(parametro != NULL && parametro->categoria == SIMBOLO_VARIABLE);
    assert(parametro->alcance == 1 && !parametro->reportable);
    assert(strcmp(parametro->tipo, "int") == 0);
    local = crear_simbolo("local", SIMBOLO_VARIABLE, "int", 44, 9);
    assert(local != NULL && insertar_simbolo(local) == local);
    comprobar_salida(imprimir_variables,
        "* Listado de variables declaradas (tipo de dato y numero de linea):\n"
        "local: int, linea 44, columna 9\n");
    salir_alcance();
    assert(buscar_simbolo("x") == NULL);
    assert(buscar_simbolo("local") == NULL);
    assert(!local->activo && local->reportable);
    entrar_alcance();
    assert(buscar_simbolo("x") == NULL); /* Bloque hermano. */
    salir_alcance();
    comprobar_salida(imprimir_variables,
        "* Listado de variables declaradas (tipo de dato y numero de linea):\n"
        "local: int, linea 44, columna 9\n");
    liberar_tabla();
    /* Ningun parametro parcial si una definicion aun no tiene nombres. */
    f = crear_funcion("pendiente", "int", "int", 50);
    assert(agregar_parametro_ubicado(f, "int", NULL, 50, 20));
    marcar_definido(f);
    entrar_alcance();
    assert(!cargar_parametros_en_alcance(f));
    assert(primer_simbolo() == NULL);
    salir_alcance();
    destruir_simbolo(f);
}

static void probar_listado_funciones(void)
{
    Simbolo *imprimir = crear_funcion("imprimir", "void", NULL, 1);
    Simbolo *prototipo_tardio = crear_funcion("imprimir", "void", NULL, 6);
    Simbolo *prototipo = crear_funcion("incremento", "int", "int", 9);
    Simbolo *definicion = crear_funcion("incremento", "int", "int", 43);
    marcar_definido(imprimir);
    marcar_definido(definicion);
    assert(insertar_simbolo(imprimir) == imprimir);
    /* Se supone que la parte 3 ya valido que es compatible. */
    assert(insertar_simbolo(prototipo_tardio) == prototipo_tardio);
    assert(!prototipo_tardio->reportable);
    assert(insertar_simbolo(prototipo) == prototipo);
    assert(insertar_simbolo(definicion) == definicion);
    assert(definicion->reportable);
    assert(buscar_definicion_en_alcance_actual("incremento") == definicion);
    comprobar_salida(imprimir_funciones,
        "* Listado de funciones declaradas y definidas:\n"
        "imprimir: definicion, input: void, retorna: void, linea 1\n"
        "incremento: declaracion, input: int x, retorna: int, linea 9\n"
        "incremento: definicion, input: int x, retorna: int, linea 43\n");
    liberar_tabla();
}

static void probar_parametro_const(void)
{
    Simbolo *f = crear_funcion("f", "void", "int", 1);
    Simbolo *prototipo = crear_funcion("f", "void", "int", 2);
    Simbolo *x;
    obtener_parametro(f, 0)->es_const = 1;
    /* const por valor no cambia la compatibilidad de la firma. */
    assert(firmas_iguales(f, prototipo));
    destruir_simbolo(prototipo);
    marcar_definido(f);
    assert(insertar_simbolo(f) == f);
    entrar_alcance();
    assert(cargar_parametros_en_alcance(f));
    x = buscar_simbolo("x");
    assert(x != NULL && x->es_const && !x->reportable);
    comprobar_texto(tipo_variable_texto(x, 1), "const int");
    comprobar_texto(tipo_valor_texto(x), "int");
    comprobar_texto(firma_funcion_texto(f), "void(int)");
    comprobar_salida(imprimir_funciones,
        "* Listado de funciones declaradas y definidas:\n"
        "f: definicion, input: const int x, retorna: void, linea 1\n");
    salir_alcance();
    liberar_tabla();
}

static void probar_cuerpo_rechazado(void)
{
    Simbolo *original = crear_funcion("incremento", "int", "int", 43);
    Simbolo *rechazado = crear_funcion("incremento", "int", "int", 47);
    marcar_definido(original);
    marcar_definido(rechazado);
    assert(insertar_simbolo(original) == original);
    /* La parte 3 rechazo la segunda definicion, pero analiza su cuerpo. */
    entrar_alcance();
    assert(cargar_parametros_en_alcance(rechazado));
    assert(buscar_simbolo("x") != NULL);
    assert(buscar_simbolo("incremento") == original);
    salir_alcance();
    assert(finalizar_declaracion(rechazado, 0) == NULL);
    assert(buscar_definicion_en_alcance_actual("incremento") == original);
    liberar_tabla();
}

static void probar_orden_errores_y_vacios(void)
{
    comprobar_salida(imprimir_variables,
        "* Listado de variables declaradas (tipo de dato y numero de linea):\n-\n");
    comprobar_salida(imprimir_funciones,
        "* Listado de funciones declaradas y definidas:\n-\n");
    comprobar_salida(imprimir_errores_semanticos,
        "* Listado de errores semanticos:\n-\n");
    assert(agregar_error_semantico(17, 11, "'x' sin declarar", NULL));
    assert(agregar_error_semantico(17, 9,
        "El objeto invocado 'f' no es una funcion o un puntero a una funcion",
        "declarado aqui: 14:64"));
    comprobar_salida(imprimir_errores_semanticos,
        "* Listado de errores semanticos:\n"
        "17:11: 'x' sin declarar\n"
        "17:9: El objeto invocado 'f' no es una funcion o un puntero a una funcion\n"
        "Nota: declarado aqui: 14:64\n");
    liberar_errores_semanticos();
    assert(primer_simbolo() == NULL && primer_error_semantico() == NULL);
    assert(alcance_actual() == 0);
    liberar_tabla();
    liberar_errores_semanticos();
}

int main(void)
{
    probar_formatos_y_tipos();
    probar_rechazados_y_copias();
    probar_parametros_y_alcances();
    probar_listado_funciones();
    probar_parametro_const();
    probar_cuerpo_rechazado();
    probar_orden_errores_y_vacios();
    puts("Todas las pruebas de TS y reporte pasaron.");
    return 0;
}
