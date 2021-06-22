/** @file
 * Implementacja kalkulatora wielomianów rzadkich wielu zmiennych
 *
 * @author Filip Głębocki <fg429202@students.mimuw.edu.pl>
 * @copyright Uniwersytet Warszawski
 * @date 2021
 */

/** Makro pozwalające nam na użycie funkcji getline. */
#define _GNU_SOURCE

#include "poly.h"
#include "stack.h"
#include "parsing.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

/**
 * To jest zmienna wyliczeniowa reprezentująca różne kody wyjścia
 * zwracane przez różne funkcje z tego modułu. Pozwala dowiedzieć się,
 * czy wykonanie polecenia się udało, a jeśli nie, to informuje co
 * poszło nie tak, dzięki czemu wiemy, jaką treść powinien mieć błąd.
 * Aby uniknąć niejednoznaczności, kod wyjścia nazywany będzie kodem
 * wyjścia typu exec.
 */
enum EXECCODE {
  OK, ERR_COMMAND, ERR_DEG_BY, ERR_AT, ERR_NULL_IN_ARG,
  ERR_STACK_UNDERFLOW, ERR_WRONG_POLY, ERR_COMPOSE
};

/** To jest typ reprezentujący kod wyjścia typu exec. */
typedef int exec_code_t;

/**
 * To jest makro sprawdzające, czy stos nie jest pusty.
 * Wywoływane jest przy wykonywaniu poleceń, które wymagają, aby stos
 * nie był pusty. Jeśli stos jest pusty, makro zwraca odpowiedni kod błędu
 * odpowiadający zbyt małej liczbie wielomianów na stosie.
 */
#define CHECK_STACK(s)            \
  do {                            \
    if ((s)->top == 0) {          \
      return ERR_STACK_UNDERFLOW; \
    }                             \
  } while (0)

/**
 * Wywołuje polecenie ZERO, które wstawia na wierzchołek stosu
 * wielomian tożsamościowo równy zeru.
 * @param[in] s : stos
 * @return kod wyjścia typu exec
 */
static exec_code_t ExecZero(Stack *s) {
  Poly zero = PolyZero();
  Push(s, &zero);

  return OK;
}

/**
 * Wywołuje polecenie IS_COEFF, które sprawdza, czy wielomian
 * na wierzchołku stosu jest wielomianem stałym. Wypisuje na
 * standardowe wyjście 1 lub 0.
 * @param[in] s : stos
 * @return kod wyjścia typu exec
 */
static exec_code_t ExecIsCoeff(Stack *s) {
  CHECK_STACK(s);

  Poly p = Pop(s);
  printf("%d\n", PolyIsCoeff(&p) ? 1 : 0);
  Push(s, &p);

  return OK;
}

/**
 * Wywołuje polecenie IS_ZERO, które sprawdza, czy wielomian
 * na wierzchołku stosu jest tożsamościowo równy zeru.
 * Wypisuje na standardowe wyjście 1 lub 0.
 * @param[in] s : stos
 * @return kod wyjścia typu exec
 */
static exec_code_t ExecIsZero(Stack *s) {
  CHECK_STACK(s);

  Poly p = Pop(s);
  printf("%d\n", PolyIsZero(&p) ? 1 : 0);
  Push(s, &p);

  return OK;
}

/**
 * Wywołuje polecenie CLONE, które wstawia na stos kopię
 * wielomianu z wierzchołka.
 * @param[in] s : stos
 * @return kod wyjścia typu exec
 */
static exec_code_t ExecClone(Stack *s) {
  CHECK_STACK(s);

  Poly p = Pop(s);
  Poly pClone = PolyClone(&p);
  Push(s, &p);
  Push(s, &pClone);

  return OK;
}

/**
 * Wywołuje polecenie ADD, które dodaje dwa wielomiany z wierzchu stosu,
 * usuwa je i wstawia na wierzchołek stosu ich sumę.
 * @param[in] s : stos
 * @return kod wyjścia typu exec
 */
static exec_code_t ExecAdd(Stack *s) {
  if (s->top < 2)
    return ERR_STACK_UNDERFLOW;

  Poly p = Pop(s);
  Poly q = Pop(s);
  Poly sum = PolyAdd(&p, &q);
  Push(s, &sum);

  PolyDestroy(&p);
  PolyDestroy(&q);

  return OK;
}

/**
 * Wywołuje polecenie MUL, które mnoży dwa wielomiany z wierzchu stosu,
 * usuwa je i wstawia na wierzchołek stosu ich iloczyn.
 * @param[in] s : stos
 * @return kod wyjścia typu exec
 */
static exec_code_t ExecMul(Stack *s) {
  if (s->top < 2)
    return ERR_STACK_UNDERFLOW;

  Poly p = Pop(s);
  Poly q = Pop(s);
  Poly pq = PolyMul(&p, &q);
  Push(s, &pq);

  PolyDestroy(&p);
  PolyDestroy(&q);

  return OK;
}

/**
 * Wywołuje polecenie NEG, negujące wielomian na wierzchołku stosu.
 * @param[in] s : stos
 * @return kod wyjścia typu exec
 */
static exec_code_t ExecNeg(Stack *s) {
  CHECK_STACK(s);

  Poly p = Pop(s);
  Poly pNeg = PolyNeg(&p);
  Push(s, &pNeg);

  PolyDestroy(&p);

  return OK;
}

/**
 * Wywołuje polecenie SUB, które odejmuje od wielomianu z wierzchołka
 * wielomian pod wierzchołkiem, usuwa je i wstawia na wierzchołek
 * stosu ich różnicę.
 * @param[in] s : stos
 * @return kod wyjścia typu exec
 */
static exec_code_t ExecSub(Stack *s) {
  if (s->top < 2)
    return ERR_STACK_UNDERFLOW;

  Poly p = Pop(s);
  Poly q = Pop(s);
  Poly diff = PolySub(&p, &q);
  Push(s, &diff);

  PolyDestroy(&p);
  PolyDestroy(&q);

  return OK;
}

/**
 * Wywołuje polecenie SUB, które sprawdza, czy dwa wielomiany
 * na wierzchu stosu są równe. Wypisuje na standardowe wyjście 1 lub 0.
 * @param[in] s : stos
 * @return kod wyjścia typu exec
 */
static exec_code_t ExecIsEq(Stack *s) {
  if (s->top < 2)
    return ERR_STACK_UNDERFLOW;

  Poly p = Pop(s);
  Poly q = Pop(s);
  printf("%d\n", PolyIsEq(&p, &q) ? 1 : 0);
  Push(s, &q);
  Push(s, &p);

  return OK;
}

/**
 * Wywołuje polecenie DEG, które wypisuje na standardowe wyjście
 * stopień wielomianu. Jeśli wielomian jest tożsamościowo równy zero,
 * wypisana jest wartość -1.
 * @param[in] s : stos
 * @return kod wyjścia typu exec
 */
static exec_code_t ExecDeg(Stack *s) {
  CHECK_STACK(s);

  Poly p = Pop(s);
  printf("%d\n", PolyDeg(&p));
  Push(s, &p);

  return OK;
}

/**
 * Wywołuje polecenie DEG_BY, które wypisuje na standardowe wyjście
 * stopień wielomianu ze względu na zmienną o numerze @p arg.
 * Jeśli wielomian jest tożsamościowo równy zero, wypisana jest wartość -1.
 * @param[in] s : stos
 * @param[in] arg : napis zawierający numer zmiennej
 * @return kod wyjścia typu exec
 */
static exec_code_t ExecDegBy(const char *arg, Stack *s) {
  if (arg == NULL || !isdigit(arg[0]) || arg[0] == '-')
    return ERR_DEG_BY;

  char *endPtr = NULL;
  errno = 0;
  size_t varIdX = strtoul(arg, &endPtr, 10);

  if (endPtr == NULL || *endPtr != '\0' || errno == ERANGE)
    return ERR_DEG_BY;

  CHECK_STACK(s);

  Poly p = Pop(s);
  printf("%d\n", PolyDegBy(&p, varIdX));
  Push(s, &p);

  return OK;
}

/**
 * Wywołuje polecenie AT, które wylicza wartość wielomianu w punkcie x,
 * usuwa wielomian z wierzchołka i wstawia na stos wynik operacji.
 * @param[in] s : stos
 * @param[in] arg : napis zawierający wartość w punkcie x
 * @return kod wyjścia typu exec
 */
static exec_code_t ExecAt(const char *arg, Stack *s) {
  if (arg == NULL || !(isdigit(arg[0]) || arg[0] == '-'))
    return ERR_AT;

  char *endPtr = NULL;
  errno = 0;
  poly_coeff_t x = strtol(arg, &endPtr, 10);
  if (endPtr == NULL || *endPtr != '\0' || errno == ERANGE)
    return ERR_AT;

  CHECK_STACK(s);

  Poly p = Pop(s);
  Poly pAt = PolyAt(&p, x);
  Push(s, &pAt);

  PolyDestroy(&p);

  return OK;
}

/**
 * Wywołuje polecenie COMPOSE, które zdejmuje z wierzchołka stosu
 * najpierw wielomian p, a potem kolejno wielomiany @f$q[k - 1], q[k - 2], ..., q[0]@f$
 * i umieszcza na stosie wynik operacji złożenia.
 * @param[in] s : stos
 * @param[in] arg : napis zawierający liczbę wielomianów do podstawienia
 * @return kod wyjścia typu exec
 */
static exec_code_t ExecCompose(const char *arg, Stack *s) {
  if (arg == NULL || !isdigit(arg[0]) || arg[0] == '-')
    return ERR_COMPOSE;

  char *endPtr = NULL;
  errno = 0;
  size_t k = strtoul(arg, &endPtr, 10);
  if (endPtr == NULL || *endPtr != '\0' || errno == ERANGE)
    return ERR_COMPOSE;

  if (s->top <= k)
    return ERR_STACK_UNDERFLOW;

  Poly p = Pop(s);

  Poly *q = calloc(k, sizeof(Poly));
  for (size_t i = 0; i < k; i++)
    q[k - i - 1] = Pop(s);

  Poly result = PolyCompose(&p, k, q);
  Push(s, &result);

  PolyDestroy(&p);
  for (size_t i = 0; i < k; i++)
    PolyDestroy(&q[i]);
  free(q);

  return OK;
}

static void MonoPrint(Mono *m);

/**
 * Wypisuje na standardowe wyjście zadany wielomian.
 * @param[in] p : wielomian
 */
static void PolyPrint(Poly *p) {
  if (PolyIsCoeff(p)) {
    printf("%ld", p->coeff);
    return;
  }

  for (size_t i = 0; i < p->size; i++) {
    Mono currMono = p->arr[i];
    MonoPrint(&currMono);
    if (i != p->size - 1)
      printf("+");
  }
}

/**
 * Wypisuje na standardowe wyjście zadany jednomian.
 * @param[in] m : jednomian
 */
static void MonoPrint(Mono *m) {
  printf("(");
  PolyPrint(&m->p);
  printf(",%d)", MonoGetExp(m));
}

/**
 * Wywołuje polecenie PRINT, które wypisuje na standardowe wyjście
 * wielomian z wierzchołka stosu.
 * @param[in] s : stos
 * @return kod wyjścia typu exec
 */
static exec_code_t ExecPrint(Stack *s) {
  CHECK_STACK(s);

  Poly p = Pop(s);
  PolyPrint(&p);
  printf("\n");
  Push(s, &p);

  return OK;
}

/**
 * Wywołuje polecenie POP, które usuwa wielomian z wierzchołka stosu.
 * @param[in] s : stos
 * @return kod wyjścia typu exec
 */
static exec_code_t ExecPop(Stack *s) {
  CHECK_STACK(s);

  Poly p = Pop(s);
  PolyDestroy(&p);

  return OK;
}

/**
 * Wywołuje odpowiednią funkcję Exec zależnie od zadanego polecenia
 * w napisie @p command. Jeśli to konieczne, podaje wywoływanej funkcji
 * również napis @p arg.
 * @param[in] command : napis zawierający nazwę polecenia
 * @param[in] arg : napis zawierający argument polecenia
 * @param[in] s : stos
 * @return kod wyjścia typu exec
 */
static exec_code_t Execute(const char *command, const char *arg, Stack *s) {
  if (strcmp(command, "DEG_BY") == 0)
    return ExecDegBy(arg, s);
  if (strcmp(command, "AT") == 0)
    return ExecAt(arg, s);
  if (strcmp(command, "COMPOSE") == 0)
    return ExecCompose(arg, s);

  /* Jeśli polecenie jest postaci ATcoś, DEG_BYcoś lub COMPOSEcoś,
   * gdzie coś jest białym znakiem innym niż spacja, to zwracamy
   * odpowiedni błąd (nie WRONG COMMAND). */
  size_t len = strlen(command);
  if (len >= 7 && strncmp(command, "DEG_BY", 6) == 0 && isblank(command[6]))
    return ERR_DEG_BY;
  if (len >= 3 && strncmp(command, "AT", 2) == 0 && isblank(command[2]))
    return ERR_AT;
  if (len >= 8 && strncmp(command, "COMPOSE", 7) == 0 && isblank(command[7]))
    return ERR_COMPOSE;

  /* Jeśli argument nie jest pusty, to zwracamy ERR_COMMAND,
   * bo wszystkie następne polecenia są bezargumentowe. */
  if (arg != NULL)
    return ERR_COMMAND;

  if (strcmp(command, "ZERO") == 0)
    return ExecZero(s);
  if (strcmp(command, "IS_COEFF") == 0)
    return ExecIsCoeff(s);
  if (strcmp(command, "IS_ZERO") == 0)
    return ExecIsZero(s);
  if (strcmp(command, "CLONE") == 0)
    return ExecClone(s);
  if (strcmp(command, "ADD") == 0)
    return ExecAdd(s);
  if (strcmp(command, "MUL") == 0)
    return ExecMul(s);
  if (strcmp(command, "NEG") == 0)
    return ExecNeg(s);
  if (strcmp(command, "SUB") == 0)
    return ExecSub(s);
  if (strcmp(command, "IS_EQ") == 0)
    return ExecIsEq(s);
  if (strcmp(command, "DEG") == 0)
    return ExecDeg(s);
  if (strcmp(command, "PRINT") == 0)
    return ExecPrint(s);
  if (strcmp(command, "POP") == 0)
    return ExecPop(s);
  return ERR_COMMAND;
}

/**
 * Sprawdza czy wczytany napis zawiera niedozwolony znak końca linii.
 * Zwraca odpowiedni kod wyjścia typu exec zależnie od tego, czy znak
 * ten został znaleziony przed, czy po spacji.
 * @param[in] str : badany napis
 * @param[in] lineLength : długość napisu
 * @return kod wyjścia typu exec
 */
static exec_code_t HasNoNullChar(const char *str, size_t lineLength) {
  size_t i = 0;
  bool seenSpace = false;

  while (str[i] != '\n' && i != lineLength) {
    if (str[i] == ' ') {
      seenSpace = true;
    }
    else if (str[i] == '\0') {
      if (!seenSpace)
        return ERR_COMMAND;
      else
        return ERR_NULL_IN_ARG;
    }
    i++;
  }

  return OK;
}

/**
 * Szuka spacji w napisie i jeśli znajdzie, to zwraca wskaźnik
 * na pierwszy znak po spacji, w przeciwnym wypadku zwraca NULL.
 * @param[in] str : badany napis
 * @param[in] lineLength : długość napisu
 * @return wskaźnik
 */
static char *GetArg(char *str, size_t lineLength) {
  size_t i = 0;
  while (str[i] != '\n' && i != lineLength) {
    if (str[i] == ' ') {
      str[i] = '\0';
      return str + i + 1;
    }
    i++;
  }

  return NULL;
}

/**
 * Wypisuje na wyjście diagnostyczne odpowiedni komunikat o błędzie.
 * @param[in] execCode : kod wyjścia typu exec
 * @param[in] lineIndex : numer linii
 */
static void PrintErr(exec_code_t execCode, long lineIndex) {
  switch (execCode) {
    case ERR_COMMAND:
      fprintf(stderr, "ERROR %ld WRONG COMMAND\n", lineIndex);
      break;
    case ERR_DEG_BY:
      fprintf(stderr, "ERROR %ld DEG BY WRONG VARIABLE\n", lineIndex);
      break;
    case ERR_AT:
      fprintf(stderr, "ERROR %ld AT WRONG VALUE\n", lineIndex);
      break;
    case ERR_COMPOSE:
      fprintf(stderr, "ERROR %ld COMPOSE WRONG PARAMETER\n", lineIndex);
      break;
    case ERR_STACK_UNDERFLOW:
      fprintf(stderr, "ERROR %ld STACK UNDERFLOW\n", lineIndex);
      break;
    case ERR_WRONG_POLY:
      fprintf(stderr, "ERROR %ld WRONG POLY\n", lineIndex);
      break;
    default:
      break;
  }
}

/**
 * Parsuje wczytaną linię, tj. sprawdza istotne warunki i wywołuje
 * Execute albo ParsePoly, zależnie od zawartości napisu.
 * @param[in] line : wczytana linia
 * @param[in] lineLength : długość linii
 * @param[in] s : stos
 * @return kod wyjścia typu exec
 */
static exec_code_t ParseLine(char *line, size_t lineLength, Stack *s) {
  /* Jeśli linia jest komentarzem lub linią pustą, to idziemy dalej. */
  if (line[0] == '#' || line[0] == '\n')
    return OK;

  /* Linia jest poleceniem, bo pierwszy znak to litera. */
  if (isalpha(line[0])) {
    if (HasNoNullChar(line, lineLength) == ERR_COMMAND)
      return ERR_COMMAND;

    if (HasNoNullChar(line, lineLength) == ERR_NULL_IN_ARG) {
      if (memcmp(line, "DEG_BY", 6) == 0)
        return ERR_DEG_BY;
      if (memcmp(line, "AT", 2) == 0)
        return ERR_AT;
      if (memcmp(line, "COMPOSE", 7) == 0)
        return ERR_COMPOSE;
      return ERR_COMMAND;
    }

    char *command = line;
    char *arg = GetArg(line, lineLength);
    if (command[lineLength - 1] == '\n')
      command[lineLength - 1] = '\0';

    return Execute(command, arg, s);
  }

  /* Linia jest wielomianem. */
  Poly p = ParsePoly(line, lineLength);
  if (PolyIsErr(&p))
    return ERR_WRONG_POLY;

  Push(s, &p);

  return OK;
}

/**
 * Wczytuje linie z wejścia i parsuje je, wywołując dla każdej linii
 * funkcję ParseLine i sprawdzając kod wyjścia typu exec.
 */
static void ParseInput() {
  Stack s = InitStack();

  size_t bufSize = 64;
  ssize_t lineLength;
  long lineIndex = 1;
  char *buffer = calloc(bufSize, sizeof(char));
  CHECK_PTR(buffer);

  /* Wczytujemy w pętli linie ze strumienia wejścia, sprawdzając
   * za każdym razem czy funkcja getline nie zasygnalizowała braku pamięci. */
  errno = 0;
  while ((lineLength = getline(&buffer, &bufSize, stdin)) != -1) {
    int execCode = ParseLine(buffer, lineLength, &s);
    if (execCode != OK)
      PrintErr(execCode, lineIndex);

    lineIndex++;
  }

  /* Sprawdzamy, czy wystąpił jakiś błąd. */
  CHECK_PTR(buffer);
  if (errno == ENOMEM || errno == EINVAL) {
    DestroyStack(&s);
    free(buffer);
    exit(1);
  }

  DestroyStack(&s);
  free(buffer);
}

/**
 * Główna funkcja wykonująca program.
 * @return kod wyjścia programu
 */
int main() {
  ParseInput();
  exit(0);
}
