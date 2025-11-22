#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
    srand(time(NULL));
    int numero = rand() % 100 + 1;
    int intento, guess, gano = 0;

    printf("Adivina el numero (1-100).\nTienes 5 intentos.\n");

    for (intento = 1; intento <= 5; intento++) {
        printf("Intento %d: ", intento);
        scanf("%d", &guess);

        if (guess == numero) {
            printf("¡Correcto! Ganaste.\n");
            gano = 1;
            break;
        } else if (guess < numero) {
            printf("Muy bajo.\n");
        } else {
            printf("Muy alto.\n");
        }
    }

    if (!gano) {
        printf("Perdiste. El numero era %d.\n", numero);
    }

    return 0;
}
