/** @file
 * Interfejs funkcji parsujących wielomiany rzadkie wielu zmiennych
 *
 * @author Filip Głębocki <fg429202@students.mimuw.edu.pl>
 * @copyright Uniwersytet Warszawski
 * @date 2021
 */

#ifndef PARSING_H
#define PARSING_H

#include "poly.h"
#include <string.h>
#include <errno.h>
#include <limits.h>

/** To jest struktura przechowująca napis i jego długość. */
typedef struct str_len_t {
  /** To jest przechowywany napis. */
  char *str;
  /** To jest długość napisu. */
  size_t length;
} str_len_t;

/**
 * Sprawdza, czy znak należy do zbioru poprawnych znaków dopuszczalnych
 * w parsowanym wielomianie.
 * @param[in] c : znak
 * @return czy znak jest poprawny
 */
static inline bool IsCorrectPolyChar(char c) {
  return (c >= '0' && c <= '9') ||
         c == '(' || c == ')' || c == ',' || c == '+' || c == '-';
}

/**
 * Parsuje napis, zwracając wielomian. Jeśli napis nie jest
 * poprawnym wielomianem, zwracane jest ERR_POLY. Sama ta funkcja
 * sprawdza na początek istotne warunki, a potem wywołuje ParsePolyHelper.
 * @param[in] str : napis
 * @param[in] size : długość napisu
 * @return sparsowany wielomian
 */
Poly ParsePoly(char *str, size_t size);

#endif // PARSING_H
