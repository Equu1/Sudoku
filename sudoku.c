#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#define BLUE "\033[34m"
#define GRAY "\033[90m"
#define RESET "\033[0m"

typedef enum {
    EASY,
    MEDIUM,
    HARD
} DifficultyLevel;

typedef struct {
    int size;
    int boxSize;
    int** solution;
    int** puzzle;
    int** playingGrid;
    time_t startTime;
    time_t elapsedTime;
    int moveCount;
} SudokuGame;

// Prototypy funkcji
SudokuGame* createGame(int size, DifficultyLevel difficulty);
void destroyGame(SudokuGame* game);
bool generateSudoku(SudokuGame* game, DifficultyLevel difficulty);
void printGrid(const SudokuGame* game);
bool makeMove(SudokuGame* game, int row, int col, int value);
bool isGameComplete(const SudokuGame* game);
void saveGame(const SudokuGame* game, const char* filename);
SudokuGame* loadGame(const char* filename);
void displayMainMenu();
void gameLoop(SudokuGame* game);
void displayStats(const SudokuGame* game);

// Funkcje pomocnicze
static int** allocateGrid(int size);
static void freeGrid(int** grid, int size);
static void shuffleArray(int* array, int n);
static bool fillRemaining(int** grid, int size, int boxSize, int i, int j);
static bool checkIfSafe(int** grid, int size, int boxSize, int row, int col, int num);
static void removeKDigits(int** grid, int size, int k);
static bool unUsedInBox(int** grid, int boxStartRow, int boxStartCol, int num, int boxSize);
static void fillDiagonalBoxes(int** grid, int size, int boxSize);
static bool generateSolutionGrid(int** grid, int size, int boxSize);
static int isOriginalHint(const SudokuGame* game, int row, int col);

SudokuGame* createGame(int size, DifficultyLevel difficulty) {
    SudokuGame* game = malloc(sizeof(SudokuGame));
    game->size = size;
    game->boxSize = (int)sqrt(size);
    game->moveCount = 0;
    game->elapsedTime = 0;
    game->startTime = time(NULL);
    
    game->solution = allocateGrid(size);
    game->puzzle = allocateGrid(size);
    game->playingGrid = allocateGrid(size);
    
    if(!generateSudoku(game, difficulty)) {
        destroyGame(game);
        return NULL;
    }
    
    for(int i = 0; i < size; i++) {
        memcpy(game->playingGrid[i], game->puzzle[i], size * sizeof(int));
    }
    
    return game;
}

void destroyGame(SudokuGame* game) {
    if(game) {
        freeGrid(game->solution, game->size);
        freeGrid(game->puzzle, game->size);
        freeGrid(game->playingGrid, game->size);
        free(game);
    }
}

static int** allocateGrid(int size) {
    int** grid = malloc(size * sizeof(int*));
    for(int i = 0; i < size; i++) {
        grid[i] = calloc(size, sizeof(int));
    }
    return grid;
}

static void freeGrid(int** grid, int size) {
    if(grid) {
        for(int i = 0; i < size; i++) {
            free(grid[i]);
        }
        free(grid);
    }
}

// Wypełnia losowymi liczbami kwadraty na przekątnej
// Gwarantuje poprawny start dla generowania rozwiązania
// Np. dla planszy 9x9 wypełnia 3 kwadraty 3x3 na przekątnej
static void fillDiagonalBoxes(int** grid, int size, int boxSize) {
    for(int i = 0; i < size; i += boxSize) {
        int* numbers = malloc(size * sizeof(int));
        for(int k = 0; k < size; k++) numbers[k] = k + 1;
        shuffleArray(numbers, size);
        
        for(int boxRow = 0; boxRow < boxSize; boxRow++) {
            for(int boxCol = 0; boxCol < boxSize; boxCol++) {
                grid[i + boxRow][i + boxCol] = numbers[boxRow * boxSize + boxCol];
            }
        }
        free(numbers);
    }
}

// Rekurencyjne wypełnianie planszy z backtrackingiem
// Rozpoczyna od wypełnionych przekątnych
// Losowa kolejność próbowanych liczb zapewnia różnorodność rozwiązań
// Zwraca true jeśli udało się znaleźć rozwiązanie
static bool generateSolutionGrid(int** grid, int size, int boxSize) {
    fillDiagonalBoxes(grid, size, boxSize);
    return fillRemaining(grid, size, boxSize, 0, 0);
}

bool generateSudoku(SudokuGame* game, DifficultyLevel difficulty) {
    int size = game->size;
    int boxSize = game->boxSize;
    
    if(!generateSolutionGrid(game->solution, size, boxSize)) {
        return false;
    }
    
    for(int i = 0; i < size; i++) {
        memcpy(game->puzzle[i], game->solution[i], size * sizeof(int));
    }
    
    int k;
    switch(difficulty) {
        case EASY: k = size * size * 0.3; break;
        case MEDIUM: k = size * size * 0.5; break;
        case HARD: k = size * size * 0.7; break;
        default: k = size * size * 0.3;
    }
    
    removeKDigits(game->puzzle, size, k);
    return true;
}

static void removeKDigits(int** grid, int size, int k) {
    while(k > 0) {
        int cellId = rand() % (size * size);
        int row = cellId / size;
        int col = cellId % size;
        
        if(grid[row][col] != 0) {
            grid[row][col] = 0;
            k--;
        }
    }
}

// Rekurencyjny algorytm z nawrotami:
// 1. Szuka pierwszej pustej komórki
// 2. Losuje kolejność próbowanych liczb
// 3. Sprawdza bezpieczeństwo liczby w komórce
// 4. Jeśli liczba pasuje, rekurencyjnie próbuje wypełnić resztę
// 5. Jeśli brak możliwości, wycofuje się (backtrack)
// 6. Powtarza aż do pełnego wypełnienia lub wyczerpania możliwości
static bool fillRemaining(int** grid, int size, int boxSize, int i, int j) {
    if(i == size) return true;
    if(j == size) return fillRemaining(grid, size, boxSize, i + 1, 0);
    if(grid[i][j] != 0) return fillRemaining(grid, size, boxSize, i, j + 1);
    
    int* numbers = malloc(size * sizeof(int));
    for(int k = 0; k < size; k++) numbers[k] = k + 1;
    shuffleArray(numbers, size);
    
    for(int k = 0; k < size; k++) {
        int num = numbers[k];
        if(checkIfSafe(grid, size, boxSize, i, j, num)) {
            grid[i][j] = num;
            if(fillRemaining(grid, size, boxSize, i, j + 1)) {
                free(numbers);
                return true;
            }
            grid[i][j] = 0;
        }
    }
    free(numbers);
    return false;
}

static bool checkIfSafe(int** grid, int size, int boxSize, int row, int col, int num) {
    for(int j = 0; j < size; j++) {
        if(grid[row][j] == num) return false;
    }
    
    for(int i = 0; i < size; i++) {
        if(grid[i][col] == num) return false;
    }
    
    int boxStartRow = row - row % boxSize;
    int boxStartCol = col - col % boxSize;
    return unUsedInBox(grid, boxStartRow, boxStartCol, num, boxSize);
}

static bool unUsedInBox(int** grid, int boxStartRow, int boxStartCol, int num, int boxSize) {
    for(int i = 0; i < boxSize; i++) {
        for(int j = 0; j < boxSize; j++) {
            if(grid[boxStartRow + i][boxStartCol + j] == num) {
                return false;
            }
        }
    }
    return true;
}

static void shuffleArray(int* array, int n) {
    for(int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

void printGrid(const SudokuGame* game) {
    int size = game->size;
    int boxSize = game->boxSize;
    
    printf("\n");
    for(int i = 0; i < size; i++) {
        if(i % boxSize == 0 && i != 0) {
            for(int j = 0; j < size + boxSize - 1; j++) printf("---");
            printf("\n");
        }
        
        for(int j = 0; j < size; j++) {
            if(j % boxSize == 0 && j != 0) printf(" | ");
            
            int value = game->playingGrid[i][j];
            int isUserInput = (game->puzzle[i][j] == 0 && value != 0);
            int isOriginal = isOriginalHint(game, i, j);
            
            if(value == 0) {
                printf(" . ");
            } else if(isUserInput) {
                printf(BLUE "%2d " RESET, value);
            } else if(isOriginal) {
                printf(GRAY "%2d " RESET, value);
            } else {
                printf("%2d ", value);
            }
        }
        printf("\n");
    }
    displayStats(game);
}

static int isOriginalHint(const SudokuGame* game, int row, int col) {
    return (game->puzzle[row][col] != 0 && 
           game->puzzle[row][col] == game->playingGrid[row][col]);
}

void displayStats(const SudokuGame* game) {
    time_t total = game->elapsedTime + (time(NULL) - game->startTime);
    printf("\nCzas gry: %02ld:%02ld", total/60, total%60);
    printf("\nLiczba ruchow: %d\n", game->moveCount);
}

bool makeMove(SudokuGame* game, int row, int col, int value) {
    if(row < 0 || row >= game->size || col < 0 || col >= game->size) {
        return false;
    }
    
    if(game->puzzle[row][col] != 0) {
        printf("Nie mozesz zmienic poczatkowej wartosci!\n");
        return false;
    }
    
    if(value == 0) {
        game->playingGrid[row][col] = 0;
        game->moveCount++;
        return true;
    }
    
    if(value < 1 || value > game->size) {
        return false;
    }
    
    game->playingGrid[row][col] = value;
    game->moveCount++;
    return true;
}

bool isGameComplete(const SudokuGame* game) {
    for(int i = 0; i < game->size; i++) {
        for(int j = 0; j < game->size; j++) {
            if(game->playingGrid[i][j] != game->solution[i][j]) {
                return false;
            }
        }
    }
    return true;
}

void saveGame(const SudokuGame* game, const char* filename) {
    FILE* file = fopen(filename, "w");
    if(!file) {
        perror("Blad zapisu gry");
        return;
    }
    
    fprintf(file, "%d\n", game->size);
    time_t currentElapsed = game->elapsedTime + (time(NULL) - game->startTime);
    fprintf(file, "%ld %d\n", currentElapsed, game->moveCount);
    
    for(int i = 0; i < game->size; i++) {
        for(int j = 0; j < game->size; j++) {
            fprintf(file, "%d ", game->puzzle[i][j]);
        }
        fprintf(file, "\n");
    }
    
    for(int i = 0; i < game->size; i++) {
        for(int j = 0; j < game->size; j++) {
            fprintf(file, "%d ", game->solution[i][j]);
        }
        fprintf(file, "\n");
    }
    
    for(int i = 0; i < game->size; i++) {
        for(int j = 0; j < game->size; j++) {
            fprintf(file, "%d ", game->playingGrid[i][j]);
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
}

SudokuGame* loadGame(const char* filename) {
    FILE* file = fopen(filename, "r");
    if(!file) {
        perror("Blad wczytywania gry");
        return NULL;
    }
    
    int size;
    fscanf(file, "%d", &size);
    
    time_t savedElapsed;
    int savedMoves;
    fscanf(file, "%ld %d", &savedElapsed, &savedMoves);
    
    SudokuGame* game = malloc(sizeof(SudokuGame));
    game->size = size;
    game->boxSize = (int)sqrt(size);
    game->moveCount = savedMoves;
    game->elapsedTime = savedElapsed;
    game->startTime = time(NULL);
    
    game->puzzle = allocateGrid(size);
    game->solution = allocateGrid(size);
    game->playingGrid = allocateGrid(size);
    
    for(int i = 0; i < size; i++) {
        for(int j = 0; j < size; j++) {
            fscanf(file, "%d", &game->puzzle[i][j]);
        }
    }
    
    for(int i = 0; i < size; i++) {
        for(int j = 0; j < size; j++) {
            fscanf(file, "%d", &game->solution[i][j]);
        }
    }
    
    for(int i = 0; i < size; i++) {
        for(int j = 0; j < size; j++) {
            fscanf(file, "%d", &game->playingGrid[i][j]);
        }
    }
    
    fclose(file);
    return game;
}

void gameLoop(SudokuGame* game) {
    while(1) {
        printGrid(game);
        
        int row, col, value;
        printf("\nPodaj ruch (wiersz kolumna wartosc) lub 0 0 0 aby zapisac/wyjsc: ");
        scanf("%d %d %d", &row, &col, &value);
        
        if(row == 0 && col == 0 && value == 0) {
            char filename[100];
            printf("Podaj nazwe pliku do zapisu: ");
            scanf("%s", filename);
            saveGame(game, filename);
            printf("Gra zapisana\n");
            break;
        }
        
        if(!makeMove(game, row-1, col-1, value)) {
            printf("Nieprawidlowy ruch!\n");
        }
        
        if(isGameComplete(game)) {
            printGrid(game);
            printf("\nGratulacje! Wygrales!\n");
            break;
        }
    }
}

void displayMainMenu() {
    printf("\n=== MENU GLOWNE ===");
    printf("\n1. Nowa gra");
    printf("\n2. Wczytaj gre");
    printf("\n3. Instrukcja");
    printf("\n4. Wyjscie");
    printf("\nWybierz opcje: ");
}

int main() {
    srand(time(NULL));
    int choice;
    SudokuGame* game = NULL;
    
    do {
        displayMainMenu();
        scanf("%d", &choice);
        
        switch(choice) {
            case 1: {
                int size, difficulty;
                printf("\nWybierz rozmiar planszy (4, 9, 16): ");
                scanf("%d", &size);
                printf("Wybierz poziom trudności (0-łatwy, 1-średni, 2-trudny): ");
                scanf("%d", &difficulty);
                
                game = createGame(size, difficulty);
                if(game) gameLoop(game);
                destroyGame(game);
                break;
            }
            case 2: {
                char filename[100];
                printf("Podaj nazwe pliku do wczytania: ");
                scanf("%s", filename);
                game = loadGame(filename);
                if(game) gameLoop(game);
                destroyGame(game);
                break;
            }
            case 3:
                printf("\nInstrukcja:\n");
                printf("1. Wprowadzaj ruchy w formacie: wiersz kolumna wartosc\n");
                printf("2. Aby usunac wartosc, wpisz 0 jako wartosc\n");
                printf("3. Niebieskie liczby to twoje wprowadzone wartosci\n");
                printf("4. Szare liczby to oryginalne podpowiedzi\n");
                printf("5. Statystyki gry są wyświetlane pod planszą\n");
                printf("6. Aby zapisac i wyjsc, wprowadz 0 0 0\n");
                break;
            case 4:
                printf("Do widzenia!\n");
                break;
            default:
                printf("Nieprawidlowy wybor!\n");
        }
    } while(choice != 4);
    
    return 0;
}