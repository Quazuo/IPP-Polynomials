/** @file
 * Interfejs klasy stosu przechowującego wielomiany
 *
 * @author Filip Głębocki <fg429202@students.mimuw.edu.pl>
 * @copyright Uniwersytet Warszawski
 * @date 2021
 */

#ifndef __STACK_H__
#define __STACK_H__

#include "poly.h"

/** To jest początkowa wielkość stosu. */
#define INITIAL_SIZE 16

/**
 * To jest struktura przechowująca stos.
 * Implementacja stosu jest tablicowa, tj. elementy stosu przechowywane
 * są w tablicy, która w razie potrzeby jest rozszerzana.
 */
typedef struct Stack {
    /**
     * To jest obecna pojemność stosu, zawsze postaci
     * @f$2^n@f$, gdzie @f$n\in\mathBB{N},\ n\geq5@f$.
     */
    size_t size;
    /**
     * To jest indeks pierwszego wolnego miejsca w tablicy,
     * również liczba elementów obecnie przechowywanych na stosie.
     */
    size_t top;
    /**
     * To jest tablica przechowująca elementy na stosie.
     */
    struct Poly *arr;
} Stack;

/**
 * Inicjuje stos. Początkowy (jak i minimalny) rozmiar stosu wynosi 32.
 * @return zainicjowany stos
 */
Stack InitStack();

/**
 * Rozszerza stos dwukrotnie sprawdzając przy tym, czy liczba
 * nie wykracza poza zakres.
 * @param[in] s : stos
 */
void ExpandStack(Stack *s);

/**
 * Zwalnia pamięć zajmowaną przez stos.
 * @param[in] s : stos
 */
void DestroyStack(Stack *s);

/**
 * Zdejmuje wielomian z wierzchołka stosu.
 * @param[in] s : stos
 * @return wielomian
 */
Poly Pop(Stack *s);

/**
 * Wkłada wielomian na wierzchołek stosu.
 * @param[in] s : stos
 * @param[in] p : wielomian
 */
void Push(Stack *s, Poly *p);

#endif /* __STACK_H__ */
