// NAME: Carlton Duke
// Program 1
// Operating Systems
// Professor Palacios
// LINUX COMPILE: g++ -std=c++11 -pthread Main.cpp
// LINUX EXECUTE: ./a.out

#include <iostream>
#include <pthread.h>
#include <random>
#include <vector>
#include <semaphore.h>
#include <time.h>
#include <fstream>
#include <sstream>

using namespace std;

pthread_mutex_t mutex;
pthread_cond_t first_thread;
pthread_barrier_t barrier1;

class Shared {
public:
    int first, second, third;
    int game_round;
    vector<int> deck;
};

struct Player {
public:
    int me, wins;
    int next_player;
    bool did_win;
    vector<int> hand;
};

/////////////////////////////////
// SHARED VARIABLES
/////////////////////////////////

Shared data;
ofstream out_file ( "output.txt" );
int next_player_global; // Number of next player to go
int inner_round_ctr_global; // Limit on how many no win rounds can happen
int inner_round_max;
bool player_won;

/////////////////////////////////
// FUNCTIONS
/////////////////////////////////

// Create new random deck
void shuffle(int seed);

// OUTPUT DECK
void print();

// DRAW FIRST CARD FROM TOP OF DECK
int draw();

// DISCARD CARD FROM HAND TO BOTTOM OF DECK
void discard(int card);

// RETURN A RANDOMLY CHOSEN 0 OR 1
int random_discard();

// PRINT OUTPUT TO CONSOLE
void print_player_output(int player_num, vector<int> hand, bool won);

/////////////////////////////////
// THREADS
////////////////////////////////
// Dealer thread of execution
void *dealer_thread(void *arg)
{
    int seed = *((int*)(&arg));

    // Generate deck then wait
    pthread_mutex_lock(&mutex);
    shuffle(seed);

    out_file << "DEALER: shuffle" << '\n';

    pthread_mutex_unlock(&mutex);
    pthread_barrier_wait(&barrier1);
    pthread_exit(NULL);
}

// Player thread of execution
void *player_thread(void *arg)
{
    //Store hand and thread identifier (player number) then wait
    int player_num = ((struct Player*)arg)->me;
    int hand;
    pthread_barrier_wait(&barrier1);

    int drawn_card;
    ((struct Player*)arg)->did_win = false;
    while (inner_round_ctr_global <= inner_round_max) {
        pthread_mutex_lock(&mutex);

        // Wait for current player to signal done
        if (next_player_global != player_num)
            pthread_cond_wait(&first_thread, &mutex);

        // Only execute if no player has won yet
        if (!player_won) {
            // Draw a card, compare only if hand has 2 cards
            drawn_card = draw();
            if (((struct Player *) arg)->hand.empty()) {
                ((struct Player *) arg)->hand.push_back(drawn_card);
            } else {
                ((struct Player *) arg)->hand.push_back(drawn_card);
                out_file << "PLAYER " << player_num;
                out_file << ": hand " <<  ((struct Player *) arg)->hand[0] << '\n';
                out_file << "PLAYER " << player_num;
                out_file << ": draws "<< drawn_card << '\n';
                out_file << "PLAYER " << player_num;
                out_file << ": hand " <<  ((struct Player *) arg)->hand[0] << " " << ((struct Player *) arg)->hand[1] <<'\n';

                // If a player has a matching pair
                if (((struct Player *) arg)->hand[0] == ((struct Player *) arg)->hand[1]) {
                    out_file << "PLAYER " << player_num << ": wins" << '\n';
                    ((struct Player*)arg)->did_win = true;
                    player_won = true;
                    cout << "PLAYER " << player_num << ":\n"
                         << "HAND";
                    cout << " " << ((struct Player *) arg)->hand[0] << " " << ((struct Player *) arg)->hand[1];
                    cout << "\n"
                         << "WIN "
                         << "yes\n";
                } else {
                    int discard = random_discard();
                    out_file << "PLAYER " << player_num << ": discards " << ((struct Player *) arg)->hand[discard] << '\n';

                    // Discard the randomly selected card
                    if (discard == 0) {
                        data.deck.push_back(((struct Player *) arg)->hand[discard]);
                        ((struct Player *) arg)->hand.erase(((struct Player *) arg)->hand.begin());
                    } else {
                        data.deck.push_back(((struct Player *) arg)->hand[discard]);
                        ((struct Player *) arg)->hand.pop_back();
                    }
                    out_file << "PLAYER " << player_num << ": hand " << ((struct Player *) arg)->hand[0] << '\n';
                    inner_round_ctr_global += 1;
                }
            }
            next_player_global = ((struct Player *) arg)->next_player;
        } else {
            if (((struct Player *) arg)->did_win == false){
                if (inner_round_ctr_global < 4) {
                    hand = draw();
                } else {
                    hand = ((struct Player *) arg)->hand[0];
                }
                out_file << "PLAYER " << player_num << ": exits round" << '\n';
                cout << "Player " << player_num << ":\n"
                     << "HAND " << hand
                     << "\n"
                     << "WIN "
                     << "no\n";
            }
            pthread_cond_signal(&first_thread);
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }

        if (inner_round_ctr_global >= inner_round_max) {
            if (((struct Player *) arg)->did_win == false){
                if (inner_round_ctr_global < 4) {
                    hand = draw();
                } else {
                    hand = ((struct Player *) arg)->hand[0];
                }
                out_file << "PLAYER " << player_num << ": exits round" << '\n';
                cout << "Player " << player_num << ":\n"
                     << "HAND " << hand
                     << "\n"
                     << "WIN "
                     << "no\n";
            }
        }

        pthread_cond_signal(&first_thread);
        pthread_mutex_unlock(&mutex);
    }

    pthread_exit(NULL);
}

/////////////////////////////////
// MAIN
////////////////////////////////
int main() {

    int total_threads = 4;
    inner_round_ctr_global = 1;
    inner_round_max = 6;
    player_won = false;

    //////////
    //SEED
    int seed = 0;
    ///////

    cout << "Enter seed: \n";
    cin >> seed;

    mt19937 gen{seed};

    // Create array of pointers to player data structs
    Player players[3];
    Player *p[3];
    for (int i = 0; i < 3; i++) {
        p[i] = &players[i];
        p[i]->me = i+1;
    }

    // Main loop to run 3 rounds of Pair War
    for (int round = 1; round < 4; round++) {

        int first, second, third;

        // Pick player to start via random number gen, then tell player
        if (round == 1) {

            uniform_int_distribution<int> dist{1,3};
            int guess = dist(gen);
            if (guess == 1) {
                first = 1, second = 2, third = 3;
            } else if (guess == 2) {
                first = 2, second = 3, first = 1;
            } else if (guess == 3) {
                first = 3, second = 1, third = 2;
            }
        } else {
            int temp_first = first;
            first = second, second = third, third = temp_first;
        }

        p[first-1]->next_player = second, p[second-1]->next_player = third, p[third-1]->next_player = first;
        next_player_global = first;

        pthread_mutex_init(&mutex,0);
        pthread_cond_init(&first_thread,0);
        pthread_barrier_init(&barrier1, 0, 4);

        // Create threads, pass seed to dealer and a pointer to a player struct to each player
        pthread_t threads[total_threads];

        pthread_create(&threads[0], NULL, &dealer_thread, (void*)(seed));
        pthread_create(&threads[1], NULL, &player_thread, (void*)(p[0]));
        pthread_create(&threads[2], NULL, &player_thread, (void*)(p[1]));
        pthread_create(&threads[3], NULL, &player_thread, (void*)(p[2]));

        //Break the barrier, signaling that dealer has dealt
        pthread_barrier_destroy(&barrier1);

        // Tell main to wait for the child threads
        for(int j = 0; j <  total_threads; j++) {
            pthread_join(threads[j],0);
        }

        // Output deck to file and console
        print();
        out_file << "< ";
        for (int p = 0; p < data.deck.size(); p++) {
            out_file << data.deck[p] << " ";
        }
        out_file << " >" << '\n';

        // Reset global counter and player win flag
        inner_round_ctr_global = 1;
        player_won = false;
    }

    out_file.close();
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&first_thread);
    pthread_exit(NULL);
}

/////////////////////////////////
// FUNCTION DEFINITIONS
////////////////////////////////

// CREATE NEW RANDOM DECK
void shuffle(int seed) {

    vector<int> new_deck;
    auto deck_size = 54;
    auto rand_max = 9;

    mt19937 gen{seed};
    uniform_int_distribution<int> dist{1,rand_max};

    for (int i = 0; i < deck_size; i++) {
        int guess = dist(gen);
        new_deck.push_back(guess);
    }
    data.deck = new_deck;
}

// DRAW FIRST CARD FROM TOP OF DECK
int draw() {
    int card = data.deck[0];
    data.deck.erase(data.deck.begin());
    return card;
}

// DISCARD CARD FROM HAND TO BOTTOM OF DECK
void discard(int card) {
    data.deck.push_back(card);
}

// OUTPUT DECK
void print() {
    cout << "DECK: < ";
    for (int z = 0; z < data.deck.size(); z++) {
        cout << data.deck[z] << " ";
    }
    cout << ">" << endl;
}

// RANDOMLY PICK NUMBER BETWEEN 0 AND 1
int random_discard() {
    int this_seed = time(NULL);
    mt19937 gen{this_seed};
    uniform_int_distribution<int> dist{0,1};
    int guess = dist(gen);
    return guess;
}
