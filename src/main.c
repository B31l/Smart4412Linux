/* include */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/* fcntl */
#include <fcntl.h>					 															// #####
#include <sys/types.h>																			// #####
#include <sys/signal.h>																			// #####
#include <sys/stat.h>																			// #####
#include <asm/ioctls.h>																			// #####

/* define */ 
#define MAX_CLCD_CHAR 32
#define MAX_DECK_SIZE 100
#define HIT 1
#define STAND 0 

static int tactswFd = (-1);

/* struct */
typedef struct _Deck {
	int list[MAX_DECK_SIZE];
	int size;
} Deck ;

typedef struct _Player {
	int score;	// 점수 
	int state;	// 상태: Hit / Stand 
} Player;

/* 함수 원형 */ 
void error(char* message);
void print_pattern(int number);
 
/* Deck */
void init(Deck* D);												// 덱 생성 
void print_deck(Deck *D);										// 덱 출력 
int is_empty(Deck *D);											// 덱이 비었는지 확인
int is_full(Deck *D);											// 덱이 찼는지 확인
int has_pair(Deck *D);											// 덱에 페어가 있는지 확인 
void append(Deck *D, int element);								// 덱에 요소 추가
void peek(Deck *D);												// 덱의 첫 번째 요소 출력 => 뽑은 카드 
int pop(Deck *D);												// 덱의 요소 삭제 및 반환
void shuffle(Deck *D);											// 덱 섞기 
void draw(Deck *D, Deck *S, Player *player);					// 카드 드로우

/* FND */ 
unsigned char asc_to_fnd(int n); 
void write_fnd(int dev, int dealer_score, int player_score);	// FND에 딜러 및 플레이어의 점수 표기
//unsigned char get_tact(int tmo);

// static tactswDev[] = "/dev/tactsw";	// tactsw
// static int tactswFd = (-1);

int main(int argc, char *argv[]) {
	
	int fnd_dev = open("/dev/fnd", O_RDWR);
	if (fnd_dev < 0) {
		fprintf(stderr, "can't open FND (%d)", fnd_dev);
		exit(2);
	}
	int lcd_dev = open("/dev/clcd", O_RDWR);
	if (lcd_dev < 0) {
		fprintf(stderr, "can't open LCD (%d)", lcd_dev);
		exit(2);
	}
	int tact_dev = open("/dev/tactsw", O_RDWR);
	if (tact_dev < 0) {
		fprintf(stderr, "can't open TACT (%d)", tact_dev);
		exit(2);
	}
	
//	unsigned char buf[MAX_CLCD_CHAR];
//	memset(buf, 0, sizeof(buf));
//	char message[MAX_CLCD_CHAR] = "Welcome !!\0";
//	strcpy(message, s)
	printf("%s\n", message);
	write(lcd_dev, "Hello World", MAX_CLCD_CHAR);
	
	while(1) {
		/* 게임 루프 변후 */ 
		int isContinue;
		/* 게임 내 버스트 발생 여부	: 0이면 계속 게임 진행 */
		int burst = 0;		
		/* 승자 결정 변수			: 1이면 플레이어 승리, -1이면 딜러 승리, 0이면 무승부 */ 
		int score = 0;	
		
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
		
		printf("Player: "); print_deck(&player_stack); printf("  => %d점\n", player.score);
		printf("Dealer: "); print_deck(&dealer_stack); printf("  => %d점\n", dealer.score);

		// 게임 루프 
		while(1) {
			write_fnd(fnd_dev, dealer.score, player.score);										// ######
			if ((player.score > 21) && !has_pair(&player_stack)) {
				burst++;
				score--;
			}
			if ((dealer.score > 21) && !has_pair(&dealer_stack)) {
				burst++;
				score++;
			}
			if (burst) {
				switch(score) {
					case 1: printf("\n\nPlayer Win!"); break;
					case 0: printf("\n\n무승부!!"); break;	// 여기서 점수 산정 함수를 추가해야 함. 
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
				print_deck(&player_stack); printf("  => %d점\n", player.score);
				printf("Hit? (Y: 1, N: 0) "); scanf("%d", &player.state);
				if (player.state == HIT) {
					peek(&deck);
					draw(&deck, &player_stack, &player);
					printf(" => %d점", player.score);
					write_fnd(fnd_dev, dealer.score, player.score);								// ######	
				}
			}
			if (dealer.state == HIT) {
				printf("\n====================\nDealer Turn, ");
				print_deck(&dealer_stack); printf("  => %d점\n", dealer.score);
				if (dealer.score >= 17) {
					dealer.state = STAND; 
					printf("딜러는 STAND를 선택했습니다.");
				}
				if (dealer.state == HIT) {
					peek(&deck);
					draw(&deck, &dealer_stack, &dealer);
					printf(" => %d점", dealer.score);
					write_fnd(fnd_dev, dealer.score, player.score);								// ######
				}
			}
			// 턴 종료 
		} 
		printf("\n계속하시겠습니까? "); scanf("%d", &isContinue);
		if (!isContinue) break;
	}
	return 0;
}


void error(char* message) {
	fprintf(stderr, "%s\n", message);
	exit(1);
}

void print_pattern(int number) {
	switch(number) {
		case 0: printf("♧"); break;
		case 1: printf("♡"); break;
		case 2: printf("◇"); break;
		case 3: printf("♤"); break;
		default: error("wrong card");
	}
} 

void init(Deck* D) {
	D->size = 0;
}

void print_deck(Deck *D) {
	int i;
	printf("[ ");
	for(i = 0; i < D->size; i++) {
		int number = D->list[i];
		print_pattern(number / 11);
		printf(" %d ", number % 11 + 1);
	}
	printf(" ]");
}

int is_empty(Deck *D) {
	return D->size == 0;
}

int is_full(Deck *D) {
	return D->size == MAX_DECK_SIZE;
}

int has_pair(Deck *D) {
	int temp[D->size];
	temp[0] = D->list[0];
	int i, pos = 1;
	for (i = 1; i < D->size; i++) {
		if (D->list[i] % 11 != temp[pos - 1] % 11) temp[pos++] = D->list[i];
	} 
	if (pos < D->size) return 1;	// 중복이 있으면 1을 반환  
	return 0;						// 중복이 없으면 0을 반환 
}

void append(Deck *D, int element) {
	if (is_full(D)) error("full");
	D->list[D->size++] = element;
}

void peek(Deck *D) {
	int number = D->list[D->size-1];
	print_pattern(number / 11);
	printf(" %d 카드를 뽑았습니다.", number % 11 + 1);
}

int pop(Deck *D) {
	if (is_empty(D)) error("empty");
	int res = D->list[D->size - 1];
	D->size--;
	return res;
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

void draw(Deck *D, Deck *S, Player *player) {
	int element = pop(D);
	append(S, element);
	player->score += element % 11 + 1;
}

unsigned char asc_to_fnd(int n) {
	unsigned char c;
	switch (n) {
		case 0: c = ~0x3f; break;
		case 1: c = ~0x06; break;
		case 2: c = ~0x4b; break;
		case 3: c = ~0x4f; break;
		case 4: c = ~0x66; break;
		case 5: c = ~0x6d; break;
		case 6: c = ~0x7d; break;
		case 7: c = ~0x07; break;
		case 8: c = ~0x7f; break;
		case 9: c = ~0x67; break;
		default: c = ~0x00; break;
	}
 	return c;
}

void write_fnd(int dev, int dealer_score, int player_score) {
	int fnd_int = dealer_score * 100 + player_score;
	unsigned char fnd_char[4];
	fnd_char[0] = asc_to_fnd(fnd_int / 1000);
	fnd_int = fnd_int % 1000;
	fnd_char[1] = asc_to_fnd(fnd_int / 100);
	fnd_int = fnd_int % 100;
	fnd_char[2] = asc_to_fnd(fnd_int / 10);
	fnd_char[3] = asc_to_fnd(fnd_int % 10);
	write(dev, fnd_char, sizeof(fnd_char));
}

void write_lcd(int dev, char message*) {
	write(dev, message, strlen(message)); // 또는 MAX_CLCD_CHAR
}

//unsigned char get_tact(int tmo) {
//	unsigned char b;
//	if (tmo) {
//		if (tmo < 0) {
//			tmo = ~tmo * 1000;
//		} else {
//			tmo *= 1000000;
//		}
//		while (tmo > 0) {
//			usleep(100000);
//			read(tactswFd, &b, sizeof(b));
//			if (b) return (b);
//			tmo -= 10000;
//		}
//	}
//}
