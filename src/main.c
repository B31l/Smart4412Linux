#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

//#include <fnctl.h>
//#include <sys/types.h>
//#include <asm/ioctls.h>
//
//#define tact_d "/dev/tactsw"

#define MAX_DECK_SIZE 100
#define HIT 1
#define STAND 0 
typedef struct _Deck {
	int list[MAX_DECK_SIZE];
	int size;
} Deck ;

void error(char* message) {
	fprintf(stderr, "%s\n", message);
	exit(1);
}

void init(Deck* D) {
	D->size = 0;
}

int is_empty(Deck *D) {
	return D->size == 0;
}

int is_full(Deck *D) {
	return D->size == MAX_DECK_SIZE;
}

void append(Deck *D, int element) {
	if (is_full(D)) error("full");
	D->list[D->size++] = element;
}

int pop(Deck *D) {
	if (is_empty(D)) error("empty");
	int res = D->list[D->size - 1];
	D->size--;
	return res;
}

void peek(Deck *D) {
	switch(D->list[D->size - 1] / 11) {
		case 0: printf("♧"); break;
		case 1: printf("♡"); break;
		case 2: printf("◇"); break;
		case 3: printf("♤"); break;
		default: error("wrong card");
	}
	printf(" %d 카드를 뽑았습니다.", D->list[D->size - 1] % 11 + 1);
}

void show(Deck *D) {
	int i;
	printf("[ ");
	for(i = 0; i < D->size; i++) {
		char mark; 
		switch(D->list[i] / 11) {
			case 0: printf("♧"); break;
			case 1: printf("♡"); break;
			case 2: printf("◇"); break;
			case 3: printf("♤"); break;
			default: error("wrong card");
		}
		printf(" %d ", D->list[i] % 11 + 1);
	}
	printf(" ]");
}

void shuffle(Deck *D) {
	srand(time(NULL));
	int temp;
	int pos;
	int i;
	for (i = 0; i < D->size; i++) {
		pos = rand() % (D->size - i) + i;
		temp = D->list[i];
		D->list[i] = D->list[pos];
		D->list[pos] = temp;
	}
}

typedef struct _Player {
	int score;	// 점수 
	int state;	// 상태: Hit / Stand 
} Player;

/* 카드를 드로우하는 함수 */ 
void draw(Deck *D, Deck *S, Player *player) {
	int element = pop(D);
	append(S, element);
	player->score += element % 11 + 1;
}

int hasPair(Deck *D) {
	int temp[D->size];
	temp[0] = D->list[0];
	int i, pos = 1;
	for (i = 1; i < D->size; i++) {
		if (D->list[i] % 11 != temp[pos - 1] % 11) temp[pos++] = D->list[i];
	} 
	if (pos < D->size) return 1;	// 중복이 있으면 1을 반환  
	return 0;						// 중복이 없으면 0을 반환 
}

int main() {
	printf("블랙잭 게임에 오신 것을 환영합니다!\n");
	while(1) {
		int isContinue;	// 게임 루프 변수 
		int burst = 0;		// 게임 내 버스트 발생 여부	: 0이면 계속 게임 진행 
		int score = 0;		// 승자 결정 변수			: 1이면 플레이어 승리, -1이면 딜러 승리, 0이면 무승부 

		// 상태를 초기화한다. 
		Deck player_stack; init(&player_stack);		// 플레이어 카드 
		Deck dealer_stack; init(&dealer_stack);		// 딜러 카드 
		Player player = {.score = 0, .state = 1}; 	// 0: Stand / 1: Hit
		Player dealer = {.score = 0, .state = 1};
		
		// 카드 덱 (0 ~ 43)을 생성하고, 섞는다. 
		Deck deck; init(&deck);					 
		int i;
		for(i = 0; i < 44; i++) {
			append(&deck, i);
		}
		shuffle(&deck);
		
		// 플레이어와 딜러는 카드를 2장씩 뽑는다. 
		draw(&deck, &player_stack, &player);
		draw(&deck, &player_stack, &player);
		draw(&deck, &dealer_stack, &dealer);
		draw(&deck, &dealer_stack, &dealer);
		
		printf("Player: "); show(&player_stack); printf("  => %d점\n", player.score);
		printf("Dealer: "); show(&dealer_stack); printf("  => %d점\n", dealer.score);

		// 게임 루프 
		while(1) { 
			if ((player.score > 21) && !hasPair(&player_stack)) {
				burst++;
				score--;
			}
			if ((dealer.score > 21) && !hasPair(&dealer_stack)) {
				burst++;
				score++;
			}
			if (burst) {
				switch(score) {
					case 1: printf("\n\nPlayer Win!"); break;
					case 0: printf("\n\n무승부!!"); break;				// 여기서 점수 산정 함수를 추가해야 함. 
					case -1: printf("\n\nDealer Win!"); break;
					default: error("burst error");
				}
				break;
			}
			if (player.state == STAND && dealer.state == STAND) {
				int score_difference = abs(player.score - 20.9) - abs(dealer.score -20.9);
				if (score_difference < 0) {
					printf("\n\nPlayer Win!");
				} else if (score_difference > 0) {
					printf("\n\nDealer Win!");
				} else {
					printf("\n\n무승부!!");
				}
				break;
			}
			if (player.state == HIT) {
				printf("\n====================\nPlayer Turn, ");
				show(&player_stack); printf("  => %d점\n", player.score);
				printf("Hit? (Y: 1, N: 0) "); scanf("%d", &player.state);
				if (player.state == HIT) {
					peek(&deck);
					draw(&deck, &player_stack, &player);
					printf(" => %d점", player.score);
					printf("\n버스트: %d, 스코어: %d", burst, score);	
				}
			}
			if (dealer.state == HIT) {
				printf("\n====================\nDealer Turn, ");
				show(&dealer_stack); printf("  => %d점\n", dealer.score);
				printf("Hit? (Y: 1, N: 0) "); scanf("%d", &dealer.state);
				if (dealer.state == HIT) {
					peek(&deck);
					draw(&deck, &dealer_stack, &dealer);
					printf(" => %d점", dealer.score);
					printf("\n버스트: %d, 스코어: %d", burst, score);
				}
			}
		} 
		printf("\n계속하시겠습니까? "); scanf("%d", &isContinue);
		if (!isContinue) break;
	}
	return 0;
}
