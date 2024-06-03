#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>

#define NUM_CHILDREN 4

sem_t *sem;

// Gestionnaires de signaux pour les processus fils
void sigusr1_handler(int sig) {
    printf("Fils %d a reçu SIGUSR1, exécution de la tâche...\n", getpid());
    sleep(2); // Simuler une tâche
    printf("Fils %d a terminé la tâche, envoi de la confirmation...\n", getpid());
    kill(getppid(), SIGUSR2); // Envoyer la confirmation au père
}

// Gestionnaire de signaux pour le processus père
void sigusr2_handler(int sig) {
    static int confirmations = 0;
    confirmations++;
    printf("Père a reçu la confirmation d'un fils (%d/%d)\n", confirmations, NUM_CHILDREN);
    if (confirmations == NUM_CHILDREN) {
        sem_post(sem); // Permettre aux enfants de continuer
    }
}

int main() {
    pid_t enfants[NUM_CHILDREN];
    sem = sem_open("/sync_sem", O_CREAT, 0644, 0);
    if (sem == SEM_FAILED) {
        perror("sem_open échoué");
        exit(EXIT_FAILURE);
    }

    // Définir les gestionnaires de signaux
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGUSR2, sigusr2_handler);

    // Créer les processus fils
    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork échoué");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // Processus fils
            pause(); // Attendre les signaux
            sem_wait(sem); // Synchronisation avec les autres enfants
            printf("Fils %d commence la tâche synchronisée...\n", getpid());
            exit(EXIT_SUCCESS);
        } else {
            enfants[i] = pid; // Processus père
        }
    }

    // Le processus père envoie SIGUSR1 à tous les enfants
    sleep(1); // Donner le temps aux enfants de définir leurs gestionnaires de signaux
    for (int i = 0; i < NUM_CHILDREN; i++) {
        kill(enfants[i], SIGUSR1);
    }

    // Attendre que tous les enfants envoient la confirmation
    sem_wait(sem);
    printf("Tous les enfants ont envoyé leur confirmation, poursuite des tâches synchronisées...\n");

    // Attendre que tous les processus fils se terminent
    for (int i = 0; i < NUM_CHILDREN; i++) {
        wait(NULL);
    }

    sem_close(sem);
    sem_unlink("/sync_sem");

    return 0;
}
