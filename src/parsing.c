/** @file
 * Implementacja funkcji parsujących wielomiany rzadkie wielu zmiennych
 *
 * @author Filip Głębocki <fg429202@students.mimuw.edu.pl>
 * @copyright Uniwersytet Warszawski
 * @date 2021
 */

#include "parsing.h"

static Mono ParseMono(char *str, size_t size);

/**
 * Funkcja pomocnicza, wywoływana przez ParsePoly, która wykonuje
 * właściwe parsowanie. Jeśli napis nie jest poprawnym wielomianem,
 * zwracane jest ERR_POLY.
 * @param[in] str : napis
 * @param[in] size : długość napisu
 * @return sparsowany wielomian
 */
static Poly ParsePolyHelper(char *str, size_t size) {
  /* Sprawdzamy, czy parsowany napis nie jest liczbą, czyli wielomianem stałym. */
  char *endPtr = NULL;
  errno = 0;
  long coeff = strtol(str, &endPtr, 10);
  /* Nie akceptujemy liczb z plusem na początku, a także sprawdzamy,
   * czy liczba nie jest spoza zakresu. */
  if (str[0] == '+' || errno == ERANGE)
    return ERR_POLY;
  if (*endPtr == '\0')
    return PolyFromCoeff(coeff);

  /* Liczymy jednomiany w zadanym napisie i zamieniamy znaki '+', które nie są
   * w środku żadnego nawiasu na znaki '_'. */
  int monoCount = 1, openedParenthesesCount = 0;
  for (size_t i = 0; i < size; i++) {
    if (str[i] == '(') {
      openedParenthesesCount++;
    }
    else if (str[i] == ')') {
      openedParenthesesCount--;
    }
    else if (str[i] == '+' && openedParenthesesCount == 0) {
      monoCount++;
      str[i] = '_';
    }
  }

  /* Dzielimy napis na słowa względem znaków '_' i wpisujemy je
   * razem z ich długością do tablicy struktur typu str_len_t. */
  str_len_t *helperArr = calloc(monoCount, sizeof(str_len_t));
  CHECK_PTR(helperArr);

  char *monoStr = strtok(str, "_");
  size_t i = 0;
  while (monoStr != NULL) {
    helperArr[i].str = monoStr;
    helperArr[i].length = strlen(monoStr) + 1;

    monoStr = strtok(NULL, "_");
    i++;
  }

  Mono *monos = calloc(monoCount, sizeof(Mono));
  CHECK_PTR(monos);

  /* Parsujemy każdy jednomian i wpisujemy go do tablicy monos.
   * Jeśli parsowanie któregoś jednomianu się nie udało, zwalniamy
   * zaalokowaną pamięć i zwracamy ERR_POLY. */
  for (i = 0; i < (size_t) monoCount; i++) {
    monos[i] = ParseMono(helperArr[i].str, helperArr[i].length);

    if (MonoIsErr(&monos[i])) {
      free(helperArr);
      free(monos);
      return ERR_POLY;
    }
  }

  /* Wywołujemy PolyAddMonos na tablicy monos, zwalniamy zaalokowaną
   * pamięć i zwracamy wynikowy wielomian. */
  Poly result = PolyAddMonos(monoCount, monos);
  free(helperArr);
  free(monos);
  return result;
}

/**
 * Funkcja pomocnicza, wywoływana przez ParsePolyHelper, która parsuje
 * kawałek napisu będący jednomianem. Jeśli napis nie jest poprawnym
 * jednomianem, zwracane jest ERR_MONO.
 * @param[in] str : napis
 * @param[in] size : długość napisu
 * @return sparsowany jednomian
 */
static Mono ParseMono(char *str, size_t size) {
  /* Sprawdzamy czy napis nie jest pusty i czy zaczyna się i kończy nawiasami. */
  if (str == NULL || str[0] != '(' || str[size - 2] != ')')
    return ERR_MONO;

  /* Modyfikujemy lekko nasz napis, usuwając nawiasy i przesuwając
   * wskaźnik o jeden do przodu. */
  str[0] = '\0';
  str[size - 2] = '\0';
  str++;

  /* Szukamy od końca, na której pozycji znajduje się przecinek. */
  size_t commaIndex = 0;
  for (size_t i = size - 2; i > 0; i--) {
    if (str[i] == ',') {
      commaIndex = i;
      break;
    }
  }
  /* Jeśli nie znaleziono przecinka, to zwracamy ERR_MONO. */
  if (commaIndex == 0)
    return ERR_MONO;

  /* Parsujemy napis znajdujący się po przecinku, który powinien
   * zawierać wykładnik jednomianu. Jeśli napis nie zawiera liczby,
   * liczba ta jest spoza zakresu, lub ma plus na początku to
   * zwracamy ERR_MONO. */
  char *endPtr = NULL;
  char *expStr = str + commaIndex + 1;
  errno = 0;
  long exp = strtol(expStr, &endPtr, 10);
  if (expStr[0] == '+' || *endPtr != '\0' ||
      errno == ERANGE || exp < 0 || exp > INT_MAX)
    return ERR_MONO;

  /* Wstawiamy znak zerowy zamiast przecinka. */
  str[commaIndex] = '\0';

  /* Wywołujemy ParsePolyHelper na pozostałym napisie i jeśli nie jest błędny,
   * to zwracamy jednomian utworzony z tego wielomianu i wyznaczonego
   * wyżej wykładnika. */
  Poly p = ParsePolyHelper(str, commaIndex + 1);
  if (PolyIsErr(&p))
    return ERR_MONO;

  return MonoFromPoly(&p, exp);
}

Poly ParsePoly(char *str, size_t size) {
  /* Sprawdzamy, czy napis zawiera tylko znaki ze zbioru
   * znaków dopuszczalnych w wielomianie i przy okazji
   * obliczamy długość napisu. */
  size_t i = 0;
  while (str[i] != '\n' && i != size) {
    if (!IsCorrectPolyChar(str[i]))
      return ERR_POLY;
    i++;
  }

  /* Pozbywamy się znaku nowej linii z końca napisu. */
  str[i] = '\0';

  return ParsePolyHelper(str, i);
}