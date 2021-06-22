/** @file
 * Implementacja stosu przechowującego wielomiany
 *
 * @author Filip Głębocki <fg429202@students.mimuw.edu.pl>
 * @copyright Uniwersytet Warszawski
 * @date 2021
 */

#include "stack.h"

Stack InitStack() {
    Poly *resultArr = calloc(INITIAL_SIZE, sizeof(Poly));
    CHECK_PTR(resultArr);
    return (Stack) { .size = INITIAL_SIZE, .top = 0, .arr = resultArr };
}

void ExpandStack(Stack *s) {
    assert(s->size * 2 > s->size); // stack overflow
    s->size *= 2;
    s->arr = realloc(s->arr, s->size * sizeof(Poly));
    CHECK_PTR(s->arr);
}

void DestroyStack(Stack *s) {
    while (s->top > 0) {
        PolyDestroy(&s->arr[--(s->top)]);
    }
    free(s->arr);
}

Poly Pop(Stack *s) {
    assert(s->top > 0); // stack underflow
    return s->arr[--(s->top)];
}

void Push(Stack *s, Poly *p) {
    s->arr[s->top] = *p;
    (s->top)++;
    if (s->top == s->size)
        ExpandStack(s);
}