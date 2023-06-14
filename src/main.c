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

unsigned char pattern[3][8] = {
	{0x00, 0x24, 0x24, 0x24, 0x00, 0x42, 0x7E, 0x00},	// 웃는거 
	{0x00, 0x24, 0x24, 0x24, 0x00, 0x7E, 0x42, 0x00},	// 우는거 
	{0x00, 0x24, 0x24, 0x24, 0x00, 0x54, 0x2A, 0x00}	// 비긴거 
};

int fnd_dev;
int lcd_dev;
int tact_dev;
int dot_dev;
char player_arr[32] = "  Player Cards  ";
char dealer_arr[32] = "  Dealer Cards  ";

/* 함수 원형 */ 
void error(char* message);
void print_pattern(int number);
 
/* Deck */
void init(Deck* D);												// 덱 생성 
void print_deck(Deck *D);										// 덱 출력
void get_player_deck(Deck *D);
void get_dealer_deck(Deck *D);
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
void set_fnd(int dealer_score, int player_score);				// FND에 딜러 및 플레이어의 점수 표기
void set_lcd(char P[]);
int get_tact();
void set_dot(int index);
//unsigned char get_tact(int tmo);

// static tactswDev[] = "/dev/tactsw";	// tactsw
// static int tactswFd = (-1);

int main(int argc, char *argv[]) {

	
	while(1) {
		/* 게임 루프 변후 */ 
		int isContinue;
		/* 게임 내 버스트 발생 여부	: 0이면 계속 게임 진행 */
		int burst = 0;		
		/* 승자 결정 변수			: 1이면 플레이어 승리, -1이면 딜러 승리, 0이면 무승부 */ 
		int score = 0;	
		set_lcd("                   Game Start   ");
		usleep(3 * 500000);
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
			char new_player_arr[32] = "  Player Cards  ";
			char new_dealer_arr[32] = "  Dealer Cards  ";
			strcpy(player_arr, new_player_arr);
			strcpy(dealer_arr, new_dealer_arr);
			// 게임 종료 조건
			if ((player.score > 21) && !has_pair(&player_stack)) {
				burst++;
				score--;
			}
			if ((dealer.score > 21) && !has_pair(&dealer_stack)) {
				burst++;
				score++;
			}
			if (burst) break;
			if (player.state == STAND && dealer.state == STAND) {
				double score_difference = abs(player.score - 20.9) - abs(dealer.score -20.9);
				if (score_difference < 0) {
					score = 1;
				} else if (score_difference > 0) {
					score = -1;
				} else {
					score = 0;
				}
				break;
			}
			
			// 게임 진행 
			int tt;
			set_lcd("                Dealer----Player");
			set_fnd(dealer.score, player.score);
			get_player_deck(&player_stack);
			set_lcd(player_arr);
			usleep(3 * 500000);
			while(player.state == HIT) {
				printf("\n====================\nPlayer Turn, ");
				print_deck(&player_stack);
				printf("  => %d점\n", player.score);
				set_lcd("  Player Turn!  Hit ? or Stand ?");
				tt = get_tact();		
				if (tt == 5) {
					set_lcd("                Dealer    Player");
					set_fnd(dealer.score, player.score);
					continue;
				}
				while(1) {
					if (tt == 4) {
						player.state = HIT;	
					} else if (tt == 6) {
						player.state = STAND;
					} else {
						continue;	
					}
					break;
				}

				if (player.state == HIT) {
					peek(&deck);
					draw(&deck, &player_stack, &player);
					printf(" => %d점", player.score);
					set_lcd("                Dealer    Player");
					set_fnd(dealer.score, player.score);
				}
				break;
			}
			get_dealer_deck(&dealer_stack);
			set_lcd(dealer_arr);
			usleep(3 * 500000);
			while(dealer.state == HIT) {
				printf("\n====================\nDealer Turn, ");
				print_deck(&dealer_stack);
				printf("  => %d점\n", dealer.score);
				set_lcd("                  Dealer Turn!  ");
				set_fnd(dealer.score, player.score);
				if (dealer.score >= 17) {
					dealer.state = STAND;
					set_lcd("                Dealer     Stand");
					
				} else {
					set_lcd("                Dealer       Hit");	
				}
				usleep(3 * 500000);
				if (dealer.state == HIT) {
					peek(&deck);
					draw(&deck, &dealer_stack, &dealer);
					printf(" => %d점", dealer.score);
					set_lcd("                Dealer    Player");
					set_fnd(dealer.score, player.score);
				}
				break;
			}
		}
		switch(score) {
			case 1: 
				printf("\n\nPlayer Win!");
				set_lcd("                  Player Win!!  ");
				set_dot(0);
				break;
			case -1:
				printf("\n\nDealer Win!");
				set_lcd("                  Dealer Win!!  ");
				set_dot(1);
				break;
			case 0:
				printf("\n\n무승부!");
				set_lcd("                     Draw!!     ");
				set_dot(2);
				break;
			default: error("burst error");
		}
		while(1) {
			set_lcd("   Continue?        Press 5!    ");
			int tt;
			tt = get_tact();
			if (tt == 5) {
				isContinue = 1;
			} else {
				isContinue = 0;
			}
			break;
		}
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

void get_player_deck(Deck *D) {
	int i;
	for(i = 0; i < D->size; i++) {
		char temp[1];
		int num = D->list[i];
		int number = num % 11 + 1;
		strcat(player_arr, "-");
		if (number == 11) {
			strcat(player_arr, "J");
		} else {
			sprintf(temp, "%d", number);
			strcat(player_arr, temp);
		}
	}
}

void get_dealer_deck(Deck *D) {
	int i;
	for(i = 0; i < D->size; i++) {
		char temp[1];
		int num = D->list[i];
		int number = num % 11 + 1;
		strcat(dealer_arr, "-");
		if (number == 11) {
			strcat(dealer_arr, "J");
		} else {
			sprintf(temp, "%d", number);
			strcat(dealer_arr, temp);
		}
	}
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
		case 2: c = ~0x5b; break;
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

void set_fnd(int dealer_score, int player_score) {
	fnd_dev = open("/dev/fnd", O_RDWR);
	if (fnd_dev < 0) {
		fprintf(stderr, "can't open FND (%d)", fnd_dev);
		exit(2);
	}
	int fnd_int = dealer_score * 100 + player_score;
	unsigned char fnd_char[4];
	fnd_char[0] = asc_to_fnd(fnd_int / 1000);
	fnd_int = fnd_int % 1000;
	fnd_char[1] = asc_to_fnd(fnd_int / 100);
	fnd_int = fnd_int % 100;
	fnd_char[2] = asc_to_fnd(fnd_int / 10);
	fnd_char[3] = asc_to_fnd(fnd_int % 10);
	write(fnd_dev, fnd_char, sizeof(fnd_char));
	usleep(3 * 500000);
	close(fnd_dev);
}

void set_lcd(char P[]) {
	lcd_dev = open("/dev/clcd", O_RDWR);
	if (lcd_dev < 0) {
		fprintf(stderr, "can't open LCD (%d)", lcd_dev);
		exit(2);
	}
	write(lcd_dev, P, strlen(P)); // 또는 MAX_CLCD_CHAR
	close(lcd_dev);
}

int get_tact() {
	tact_dev = open("/dev/tactsw", O_RDWR);
	if (tact_dev < 0) {
		fprintf(stderr, "can't open TACT (%d)", tact_dev);
		exit(2);
	}
	unsigned char c;
	while(1) {
		read(tact_dev, &c, sizeof(c));
		if(c) break;
	}
	close(tact_dev);
	return c;
}

void set_dot(int index) {
	dot_dev = open("/dev/dot", O_RDWR);
	if (dot_dev < 0) {
		fprintf(stderr, "can't open DOT (%d)", dot_dev);
		exit(2);
	}
	write(dot_dev, &pattern[index], sizeof(pattern[index]));
  	usleep(3 * 500000);
  	close(dot_dev);
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
