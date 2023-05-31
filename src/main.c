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
		case 0: printf("��"); break;
		case 1: printf("��"); break;
		case 2: printf("��"); break;
		case 3: printf("��"); break;
		default: error("wrong card");
	}
	printf(" %d ī�带 �̾ҽ��ϴ�.", D->list[D->size - 1] % 11 + 1);
}

void show(Deck *D) {
	int i;
	printf("[ ");
	for(i = 0; i < D->size; i++) {
		char mark; 
		switch(D->list[i] / 11) {
			case 0: printf("��"); break;
			case 1: printf("��"); break;
			case 2: printf("��"); break;
			case 3: printf("��"); break;
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
	int score;	// ���� 
	int state;	// ����: Hit / Stand 
} Player;

/* ī�带 ��ο��ϴ� �Լ� */ 
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
	if (pos < D->size) return 1;	// �ߺ��� ������ 1�� ��ȯ  
	return 0;						// �ߺ��� ������ 0�� ��ȯ 
}

int main() {
	printf("���� ���ӿ� ���� ���� ȯ���մϴ�!\n");
	while(1) {
		int isContinue;	// ���� ���� ���� 
		int burst = 0;		// ���� �� ����Ʈ �߻� ����	: 0�̸� ��� ���� ���� 
		int score = 0;		// ���� ���� ����			: 1�̸� �÷��̾� �¸�, -1�̸� ���� �¸�, 0�̸� ���º� 

		// ���¸� �ʱ�ȭ�Ѵ�. 
		Deck player_stack; init(&player_stack);		// �÷��̾� ī�� 
		Deck dealer_stack; init(&dealer_stack);		// ���� ī�� 
		Player player = {.score = 0, .state = 1}; 	// 0: Stand / 1: Hit
		Player dealer = {.score = 0, .state = 1};
		
		// ī�� �� (0 ~ 43)�� �����ϰ�, ���´�. 
		Deck deck; init(&deck);					 
		int i;
		for(i = 0; i < 44; i++) {
			append(&deck, i);
		}
		shuffle(&deck);
		
		// �÷��̾�� ������ ī�带 2�徿 �̴´�. 
		draw(&deck, &player_stack, &player);
		draw(&deck, &player_stack, &player);
		draw(&deck, &dealer_stack, &dealer);
		draw(&deck, &dealer_stack, &dealer);
		
		printf("Player: "); show(&player_stack); printf("  => %d��\n", player.score);
		printf("Dealer: "); show(&dealer_stack); printf("  => %d��\n", dealer.score);

		// ���� ���� 
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
					case 0: printf("\n\n���º�!!"); break;				// ���⼭ ���� ���� �Լ��� �߰��ؾ� ��. 
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
					printf("\n\n���º�!!");
				}
				break;
			}
			if (player.state == HIT) {
				printf("\n====================\nPlayer Turn, ");
				show(&player_stack); printf("  => %d��\n", player.score);
				printf("Hit? (Y: 1, N: 0) "); scanf("%d", &player.state);
				if (player.state == HIT) {
					peek(&deck);
					draw(&deck, &player_stack, &player);
					printf(" => %d��", player.score);
					printf("\n����Ʈ: %d, ���ھ�: %d", burst, score);	
				}
			}
			if (dealer.state == HIT) {
				printf("\n====================\nDealer Turn, ");
				show(&dealer_stack); printf("  => %d��\n", dealer.score);
				printf("Hit? (Y: 1, N: 0) "); scanf("%d", &dealer.state);
				if (dealer.state == HIT) {
					peek(&deck);
					draw(&deck, &dealer_stack, &dealer);
					printf(" => %d��", dealer.score);
					printf("\n����Ʈ: %d, ���ھ�: %d", burst, score);
				}
			}
		} 
		printf("\n����Ͻðڽ��ϱ�? "); scanf("%d", &isContinue);
		if (!isContinue) break;
	}
	return 0;
}
