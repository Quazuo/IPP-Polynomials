/** @file
 * Implementacja klasy wielomianów rzadkich wielu zmiennych
 *
 * @author Filip Głębocki <fg429202@students.mimuw.edu.pl>
 * @copyright Uniwersytet Warszawski
 * @date 2021
 */

#include "poly.h"

void PolyDestroy(Poly *p) {
  if (PolyIsCoeff(p))
    return;

  for (size_t i = 0; i < p->size; i++) {
    MonoDestroy(&p->arr[i]);
  }

  free(p->arr);
}

Poly PolyClone(const Poly *p) {
  if (PolyIsCoeff(p))
    return PolyFromCoeff(p->coeff);

  Mono *newArr = calloc(p->size, sizeof(Mono));
  CHECK_PTR(newArr);

  for (size_t i = 0; i < p->size; i++)
    newArr[i] = MonoClone(&p->arr[i]);

  return (Poly) {.size = p->size, .arr = newArr};
}

int CompareMonos(const void *a, const void *b) {
  Mono monoA = *(Mono *) a;
  Mono monoB = *(Mono *) b;

  if (monoA.exp < monoB.exp)
    return -1;
  if (monoA.exp == monoB.exp)
    return 0;
  return 1;
}

/**
 * Sprawdza, czy wielomian @p p jest zagłębionym wielomianem stałym.
 * Formalnie, zagłębiony wielomian stały definiujemy jako wielomian postaci
 * @f$ax_0x_1x_2\ldotsx_i@f$, gdzie @f$a@f$ to dowolny współczynnik całkowity.
 * @param[in] p : wielomian
 * @return Czy @p p jest zagłębionym wielomianem stałym?
 */
static bool PolyIsDeepCoeff(const Poly *p) {
  if (PolyIsCoeff(p))
    return true;

  if (p->size != 1)
    return false;

  return (PolyIsDeepCoeff(&p->arr[0].p) && MonoGetExp(&p->arr[0]) == 0);
}

/**
 * Dla zadanego zagłębionego wielomianu stałego @p p zwraca współczynnik
 * jednomianu @f$px_i^0@f$, gdzie @f$i@f$ jest największym indeksem
 * zmiennej @f$x@f$ występującym w wielomianie @p p.
 * @param[in] p : wielomian
 * @return współczynnik zagłębionego wielomianu stałego @p p
 */
static poly_coeff_t PolyGetDeepCoeff(const Poly *p) {
  assert(PolyIsDeepCoeff(p));

  if (PolyIsCoeff(p))
    return p->coeff;

  return PolyGetDeepCoeff(&p->arr[0].p);
}

Poly PolyAdd(const Poly *p, const Poly *q) {
  /* Sprawdzamy, czy któryś z argumentów jest wielomianem
   * tożsamościowo równym zeru. */
  if (PolyIsZero(p))
    return PolyClone(q);
  if (PolyIsZero(q))
    return PolyClone(p);

  /* Sprawdzamy, czy któryś z argumentów jest wielomianem stałym. */
  if (PolyIsCoeff(p) && PolyIsCoeff(q)) {
    poly_coeff_t newCoeff = p->coeff + q->coeff;
    return PolyFromCoeff(newCoeff);
  }
  if (PolyIsCoeff(p)) {
    Mono *newArr = calloc(1, sizeof(Mono));
    CHECK_PTR(newArr);
    newArr[0] = (Mono) {.p = PolyFromCoeff(p->coeff), .exp = 0};

    Poly newP = {.size = 1, .arr = newArr};
    Poly result = PolyAdd(&newP, q);
    PolyDestroy(&newP);
    return result;
  }
  if (PolyIsCoeff(q)) {
    Mono *newArr = calloc(1, sizeof(Mono));
    CHECK_PTR(newArr);
    newArr[0] = (Mono) {.p = PolyFromCoeff(q->coeff), .exp = 0};

    Poly newQ = {.size = 1, .arr = newArr};
    Poly result = PolyAdd(p, &newQ);
    PolyDestroy(&newQ);
    return result;
  }

  size_t resultSize = p->size + q->size;
  Mono *resultArr = calloc(resultSize, sizeof(Mono));
  CHECK_PTR(resultArr);

  /* Przechodzimy po tablicach .arr wielomianów p i q, wrzucając
   * na przemian do tablicy resultArr jednomiany z p->arr i q->arr
   * tak, aby wynikowa tablica była posortowana i nie zawierała
   * dwóch jednomianów o równym wykładniku. */
  size_t pI = 0, qI = 0, i = 0;
  while (pI < p->size && qI < q->size) {
    Mono pMono = p->arr[pI], qMono = q->arr[qI];
    poly_exp_t pExp = MonoGetExp(&pMono);
    poly_exp_t qExp = MonoGetExp(&qMono);
    if (pExp < qExp) {
      resultArr[i] = MonoClone(&pMono);
      i++;
      pI++;
      continue;
    }
    if (pExp > qExp) {
      resultArr[i] = MonoClone(&qMono);
      i++;
      qI++;
      continue;
    }
    /* pExp == qExp */
    resultArr[i] = (Mono) {.exp = pExp,
        .p = PolyAdd(&pMono.p, &qMono.p)};
    i++;
    pI++;
    qI++;
    resultSize--;
  }
  /* Dodajemy do tablicy pozostałe jednomiany. */
  if (pI != p->size) {
    for (; pI < p->size; pI++) {
      resultArr[i] = MonoClone(&p->arr[pI]);
      i++;
    }
  }
  if (qI != q->size) {
    for (; qI < q->size; qI++) {
      resultArr[i] = MonoClone(&q->arr[qI]);
      i++;
    }
  }

  Poly result = PolyAddMonos(resultSize, resultArr);

  free(resultArr);

  return result;
}

Poly PolyAddMonos(size_t count, const Mono monos[]) {
  if (count == 0 || monos == NULL)
    return PolyZero();

  /* Tworzymy kopię tablicy monos, wyrzucając z niej wielomiany
   * tożsamościowo równe zeru. */
  size_t zeroCount = 0;
  Mono *monosCopy = calloc(count, sizeof(Mono));
  CHECK_PTR(monosCopy);

  for (size_t i = 0; i < count; i++) {
    Mono currentMono = monos[i];
    if (PolyIsZero(&currentMono.p)) {
      MonoDestroy(&currentMono);
      zeroCount++;
      continue;
    }
    monosCopy[i - zeroCount] = currentMono;
  }

  size_t newSize = count - zeroCount;
  if (newSize == 0) {
    free(monosCopy);
    return PolyZero();
  }

  SortMonos(newSize, monosCopy);

  /* Przechodzimy po tablicy monosCopy i konstruujemy tablicę monosShort
   * poprzez dodawanie do siebie jednomianów z monosCopy tak, aby tablica
   * monosShort nie zawierała jednomianów o równych wykładnikach.
   *
   * index - indeks wskazujący na odpowiednie pole w tablicy monosShort
   * sizeDiff - różnica rozmiarów tablic monosCopy i monosShort. */
  Mono *monosShort = calloc(newSize, sizeof(Mono));
  CHECK_PTR(monosShort);
  monosShort[0] = monosCopy[0];
  size_t index = 0;
  size_t sizeDiff = 0;

  for (size_t i = 1; i < newSize; i++) {
    if (MonoGetExp(&monosCopy[i]) == MonoGetExp(&monosShort[index])) {
      sizeDiff++;
      Poly p1 = monosShort[index].p;
      monosShort[index].p = PolyAdd(&p1, &monosCopy[i].p);
      PolyDestroy(&p1);
      MonoDestroy(&monosCopy[i]);
      continue;
    }
    if (PolyIsZero(&monosShort[index].p)) {
      MonoDestroy(&monosShort[index]);
      sizeDiff++;
    } else {
      index++;
    }
    monosShort[index] = monosCopy[i];
  }

  free(monosCopy);
  newSize -= sizeDiff;

  /* Sprawdzamy, czy wynikiem nie jest wielomian tożsamościowo równy zeru. */
  if (newSize == 1 && PolyIsZero(&monosShort[0].p)) {
    MonoDestroy(&monosShort[0]);
    Poly result = PolyZero();
    free(monosShort);
    return result;
  }

  /* Na koniec sprawdzamy, czy wynikiem nie jest zagłębiony wielomian stały. */
  Poly result = (Poly) {.size = newSize, .arr = monosShort};
  if (PolyIsDeepCoeff(&result)) {
    Poly newResult = PolyFromCoeff(PolyGetDeepCoeff(&result));
    PolyDestroy(&result);
    return newResult;
  }
  return result;
}

Poly PolyOwnMonos(size_t count, Mono *monos) {
  if (count == 0 || monos == NULL)
    return PolyZero();
  Poly result = PolyAddMonos(count, monos);
  free(monos);
  return result;
}

Poly PolyCloneMonos(size_t count, const Mono monos[]) {
  if (count == 0 || monos == NULL)
    return PolyZero();
  Mono *monosCopy = calloc(count, sizeof(Mono));
  for (size_t i = 0; i < count; i++)
    monosCopy[i] = MonoClone(&monos[i]);

  Poly result = PolyAddMonos(count, monosCopy);
  free(monosCopy);
  return result;
}

Poly PolyMulCoeff(const Poly *p, poly_coeff_t c) {
  if (PolyIsDeepCoeff(p))
    return PolyFromCoeff(PolyGetDeepCoeff(p) * c);
  if (c == 0)
    return PolyZero();

  Mono *newArr = calloc(p->size, sizeof(Mono));
  CHECK_PTR(newArr);

  /* Rekurencyjnie konstruujemy wielomian wynikowy. */
  for (size_t i = 0; i < p->size; i++) {
    Mono currentMono = p->arr[i];
    newArr[i] = (Mono) {.p = PolyMulCoeff(&currentMono.p, c),
        .exp = currentMono.exp};
  }

  /* Na wszelki wypadek sprawdzamy, czy wynikiem nie jest wielomian
   * tożsamościowo równy zeru, np. w wyniku pomnożenia zbyt dużych wartości. */
//  if (PolyIsZero(&newArr[0].p)) {
//    for (size_t i = 0; i < p->size; i++) {
//      MonoDestroy(&newArr[i]);
//    }
//    free(newArr);
//    return PolyZero();
//  }

  //return (Poly) {.size = p->size, .arr = newArr};
  Poly result = PolyAddMonos(p->size, newArr);
  free(newArr);
  return result;
}

Poly PolyMul(const Poly *p, const Poly *q) {
  /* Sprawdzamy, czy któryś z argumentów jest wielomianem stałym. */
  if (PolyIsCoeff(p) && PolyIsCoeff(q))
    return PolyFromCoeff(p->coeff * q->coeff);

  if (PolyIsCoeff(p)) {
    Poly result = PolyMulCoeff(q, p->coeff);
    return result;
  }

  if (PolyIsCoeff(q)) {
    Poly result = PolyMulCoeff(p, q->coeff);
    return result;
  }

  /* Konstruujemy tablicę jednomianów resultArr, wrzucając do niej
   * każdy jednomian postaci m_i * n_j, gdzie m_i to i-ty jednomian
   * z tablicy p->arr, a n_j to j-ty jednomian z tablicy q->arr. */
  Mono *resultArr = calloc(p->size * q->size, sizeof(Mono));
  CHECK_PTR(resultArr);

  size_t i = 0;
  for (size_t pI = 0; pI < p->size; pI++) {
    for (size_t qI = 0; qI < q->size; qI++) {
      Mono pMono = p->arr[pI];
      Mono qMono = q->arr[qI];

      Poly pq = PolyMul(&pMono.p, &qMono.p);
      resultArr[i] = (Mono) {.p = pq, .exp = pMono.exp + qMono.exp};

      i++;
    }
  }

  Poly result = PolyAddMonos(p->size * q->size, resultArr);
  free(resultArr);
  return result;
}

Poly PolyNeg(const Poly *p) {
  /* Mnożymy wielomian p przez stałą -1. */
  Poly result = PolyMulCoeff(p, -1);
  return result;
}

Poly PolySub(const Poly *p, const Poly *q) {
  /* Korzystamy z prostej tożsamości p - q == p + (-1) * q. */
  Poly qNeg = PolyNeg(q);
  Poly result = PolyAdd(p, &qNeg);
  PolyDestroy(&qNeg);
  return result;
}

poly_exp_t PolyDegBy(const Poly *p, size_t varIdx) {
  /* Sprawdzamy, czy argument p jest wielomianem stałym. */
  if (PolyIsZero(p))
    return -1;
  if (PolyIsCoeff(p))
    return 0;

  /* Funkcja działa rekurencyjnie: jeśli var_idx == 0, to po prostu
   * obliczamy stopień wielomianu. Jeśli zaś var_idx > 0, to bierzemy
   * wartość największą ze stopni współczynników jednomianów z tablicy
   * p->arr, wywołując funkcję rekurencyjnie i zmniejszając var_idx o 1. */
  if (varIdx == 0) {
    Mono firstMono = p->arr[0];
    poly_exp_t maxExp = MonoGetExp(&firstMono);
    if (PolyIsZero(&firstMono.p))
      maxExp = -1;

    for (size_t i = 1; i < p->size; i++) {
      Mono currentMono = p->arr[i];
      if (PolyIsZero(&currentMono.p))
        continue;

      if (MonoGetExp(&currentMono) > maxExp)
        maxExp = MonoGetExp(&p->arr[i]);
    }

    return maxExp;
  }

  /* varIdx > 0 */
  Mono firstMono = p->arr[0];
  poly_exp_t maxExp = PolyDegBy(&firstMono.p, varIdx - 1);

  for (size_t i = 1; i < p->size; i++) {
    Mono currentMono = p->arr[i];
    poly_exp_t CurrentMaxExp = PolyDegBy(&currentMono.p, varIdx - 1);
    if (CurrentMaxExp > maxExp)
      maxExp = CurrentMaxExp;
  }

  return maxExp;
}

poly_exp_t PolyDeg(const Poly *p) {
  /* Sprawdzamy, czy argument p jest wielomianem stałym. */
  if (PolyIsZero(p))
    return -1;
  if (PolyIsCoeff(p))
    return 0;

  poly_exp_t maxDeg = 0;

  /* Obliczamy stopień wielomianu p poprzez branie największej wartości
   * z obliczanych rekurencyjnie stopni współczynników jednomianów z tablicy
   * p->arr, pomnożonych przez wykładnik tych jednomianów. */
  for (size_t i = 0; i < p->size; i++) {
    Mono currentMono = p->arr[i];

    if (PolyIsZero(&currentMono.p))
      continue;

    poly_exp_t currentDeg = PolyDeg(&currentMono.p) + MonoGetExp(&currentMono);

    if (currentDeg > maxDeg)
      maxDeg = currentDeg;
  }

  return maxDeg;
}

bool MonoIsEq(const Mono *m, const Mono *n) {
  if (m->exp != n->exp)
    return false;

  return PolyIsEq(&m->p, &n->p);
}

bool PolyIsEq(const Poly *p, const Poly *q) {
  if (PolyIsCoeff(p) && PolyIsCoeff(q))
    return (p->coeff == q->coeff);

  if (PolyIsCoeff(p) || PolyIsCoeff(q))
    return false;

  if (p->size != q->size)
    return false;

  for (size_t i = 0; i < p->size; i++) {
    Mono pMono = p->arr[i];
    Mono qMono = q->arr[i];
    if (!MonoIsEq(&pMono, &qMono))
      return false;
  }

  return true;
}

/**
 * Wykonuje szybkie potęgowanie liczb typu poly_coeff_t.
 * @param[in] a : liczba @f$a@f$
 * @param[in] x : liczba @f$x@f$
 * @return @f$a^x@f$
 */
static poly_coeff_t QuickPow(poly_coeff_t a, poly_coeff_t x) {
  if (x == 0)
    return 1;
  if (x % 2 == 1)
    return a * QuickPow(a, x - 1);
  poly_coeff_t result = QuickPow(a, x / 2);
  return result * result;
}

Poly PolyAt(const Poly *p, poly_coeff_t x) {
  /* Jeśli p jest wielomianem stałym, to nie musimy nic robić. */
  if (PolyIsCoeff(p))
    return PolyFromCoeff(p->coeff);

  /* Wielomian wynikowy konstruujemy poprzez przechodzenie po tablicy jednomianów
   * p->arr i dodając do siebie ich współczynniki pomnożone przez stałą x. */
  Mono firstMono = p->arr[0];
  poly_coeff_t multiplier = QuickPow(x, MonoGetExp(&firstMono));
  Poly result = PolyMulCoeff(&firstMono.p, multiplier);

  for (size_t i = 1; i < p->size; i++) {
    Mono currentMono = p->arr[i];
    multiplier = QuickPow(x, MonoGetExp(&currentMono));
    Poly newPoly = PolyMulCoeff(&currentMono.p, multiplier);

    Poly mem = result;
    result = PolyAdd(&result, &newPoly);
    PolyDestroy(&mem);
    PolyDestroy(&newPoly);
  }

  return result;
}

/**
 * Wykonuje szybkie potęgowanie wielomianów.
 * @param[in] p : wielomian
 * @param[in] x : liczba @f$x@f$
 * @return @f$p^x@f$
 */
static Poly PolyQuickPow(const Poly *p, poly_exp_t x) {
  if (x == 0)
    return PolyFromCoeff(1);
  if (PolyIsZero(p))
    return PolyZero();

  if (x % 2 == 1) {
    Poly q = PolyQuickPow(p, x - 1);
    Poly result = PolyMul(p, &q);
    PolyDestroy(&q);
    return result;
  }

  Poly q = PolyQuickPow(p, x / 2);
  Poly result = PolyMul(&q, &q);
  PolyDestroy(&q);
  return result;
}

/**
 * Funkcja pomocnicza do funkcji PolyCompose, wykonująca
 * właściwe składanie. Pozwala na wykorzystanie rekurencji.
 * Dla opisu operacji składania patrz: opis funkcji PolyCompose.
 * @param[in] p : wielomian, do którego podstawiamy
 * @param[in] k : liczba podstawianych wielomianów
 * @param[in] q : tablica podstawianych wielomianów
 * @param[in] idX : stopień zagłębienia
 * @return wynik operacji złożenia
 */
static Poly PolyComposeHelper(const Poly *p, size_t k, const Poly q[], size_t idX) {
  /* W przypadku (zagłębionego) wielomianu stałego zwracamy odpowiedni wielomian stały. */
  if (PolyIsDeepCoeff(p))
    return PolyFromCoeff(PolyGetDeepCoeff(p));

  Poly result = PolyZero();

  for (size_t i = 0; i < p->size; i++) {
    Mono currMono = p->arr[i];
    poly_exp_t currExp = MonoGetExp(&currMono);

    /* Sprawdzamy */
    Poly temp1 = PolyZero();
    if (currExp == 0)
      temp1 = PolyFromCoeff(1);
    else if (idX <= k)
      temp1 = PolyQuickPow(&q[idX], currExp);

    Poly temp2 = PolyComposeHelper(&currMono.p, k, q, idX + 1);
    Poly temp = PolyMul(&temp1, &temp2);

    PolyDestroy(&temp1);
    PolyDestroy(&temp2);

    Poly newResult = PolyAdd(&result, &temp);
    PolyDestroy(&result);
    PolyDestroy(&temp);
    result = newResult;
  }

  return result;
}

Poly PolyCompose(const Poly *p, size_t k, const Poly q[]) {
  return PolyComposeHelper(p, k, q, 0);
}
