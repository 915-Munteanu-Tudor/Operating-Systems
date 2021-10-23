/*
 * The program creates multiple threads, one for each country and each viewer.
 *
 * The country:
 * - each day it generates a number of cases, which can be positive or negative
 * - it adds the new number of cases to its number of cases until that moment
 * - if the number of cases is equal to 0, then the country is free of covid and the thread stops
 *   and the maximum number of cases per day is decreased
 * - if the number of cases is greater than a threshold, then the country is in quarantine
 *   and the maximum number of cases per day is increased for each day the country is still in quarantine
 *
 * The reader:
 * - generates a report of how many countries are free of covid and how many countries are in quarantine
 * - when all countries are free of covid, the thread stops
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define INITIAL_CASES 500
#define DAY_LENGTH 1
#define QUARANTINE_THRESHOLD 250
#define NUMBER_OF_COUNTRIES 5
#define NUMBER_OF_READERS 10

int maxNumberPerDay = 50;
int cases[NUMBER_OF_COUNTRIES];

pthread_rwlock_t casesRwlock = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t maxCasesRwlock = PTHREAD_RWLOCK_INITIALIZER;

void *country(void *arg) {
    int index = *(int *) arg;
    int day = 0;
    for (;;) {
        pthread_rwlock_rdlock(&maxCasesRwlock);
        int todayCases = rand() % maxNumberPerDay;
        pthread_rwlock_unlock(&maxCasesRwlock);

        if (rand() % 3 != 0) {
            todayCases = -todayCases;

            pthread_rwlock_rdlock(&casesRwlock);
            if (todayCases < -cases[index]) {
                todayCases = -cases[index];
            }
            pthread_rwlock_unlock(&casesRwlock);
        }

        pthread_rwlock_wrlock(&casesRwlock);
        cases[index] += todayCases;
        pthread_rwlock_unlock(&casesRwlock);

        pthread_rwlock_rdlock(&casesRwlock);
        printf("day %d, country %d: %d cases; total cases: %d\n", day, index, todayCases, cases[index]);
        if (cases[index] == 0) {
            printf(">>> country %d has no more cases!\n", index);
            pthread_rwlock_unlock(&casesRwlock);

            pthread_rwlock_wrlock(&maxCasesRwlock);
            maxNumberPerDay -= 1;
            pthread_rwlock_unlock(&maxCasesRwlock);

            break;
        }
        if (cases[index] >= QUARANTINE_THRESHOLD) {
            printf(">>> oh no, country %d is in quarantine!\n", index);

            pthread_rwlock_wrlock(&maxCasesRwlock);
            maxNumberPerDay += 1;
            pthread_rwlock_unlock(&maxCasesRwlock);
        }
        fflush(stdout);
        pthread_rwlock_unlock(&casesRwlock);

        sleep(DAY_LENGTH);
        day += 1;
    }
    free(arg);
    return NULL;
}

void *reader(void *arg) {
    int index = *(int *) arg;
    int day = 0;
    for (;;) {
        int freeCountries = 0;
        int quarantinedCountries = 0;
        pthread_rwlock_rdlock(&casesRwlock);
        for (int i = 0; i < NUMBER_OF_COUNTRIES; ++i) {
            if (cases[i] == 0) {
                freeCountries += 1;
            } else if (cases[i] >= QUARANTINE_THRESHOLD) {
                quarantinedCountries += 1;
            }
        }
        pthread_rwlock_unlock(&casesRwlock);
        printf("REPORT: viewer %d, day %d: %d countries with no cases, %d countries in quarantine\n", index, day, freeCountries, quarantinedCountries);
        fflush(stdout);
        if (freeCountries == NUMBER_OF_COUNTRIES) {
            break;
        }
        sleep(DAY_LENGTH);
        day += 1;
    }
    free(arg);
    return NULL;
}


int main() {
    pthread_t threads[NUMBER_OF_COUNTRIES + NUMBER_OF_READERS];
    for (int i = 0; i < NUMBER_OF_COUNTRIES; ++i) {
        cases[i] = INITIAL_CASES;
    }
    for (int i = 0; i < NUMBER_OF_COUNTRIES; ++i) {
        int *iHeap = malloc(sizeof(int));
        *iHeap = i;
        pthread_create(&threads[i], NULL, country, iHeap);
    }
    for (int i = NUMBER_OF_COUNTRIES; i < NUMBER_OF_READERS; ++i) {
        int *iHeap = malloc(sizeof(int));
        *iHeap = i - NUMBER_OF_COUNTRIES;
        pthread_create(&threads[i], NULL, reader, iHeap);
    }
    for (int i = 0; i < NUMBER_OF_COUNTRIES + NUMBER_OF_READERS; ++i) {
        pthread_join(threads[i], NULL);
    }
    pthread_rwlock_destroy(&casesRwlock);
    pthread_rwlock_destroy(&maxCasesRwlock);
    return 0;
}