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
	int score;	// ���� 
	int state;	// ����: Hit / Stand 
} Player;

/* �Լ� ���� */ 
void error(char* message);
void print_pattern(int number);
 
/* Deck */
void init(Deck* D);												// �� ���� 
void print_deck(Deck *D);										// �� ��� 
int is_empty(Deck *D);											// ���� ������� Ȯ��
int is_full(Deck *D);											// ���� á���� Ȯ��
int has_pair(Deck *D);											// ���� �� �ִ��� Ȯ�� 
void append(Deck *D, int element);								// ���� ��� �߰�
void peek(Deck *D);												// ���� ù ��° ��� ��� => ���� ī�� 
int pop(Deck *D);												// ���� ��� ���� �� ��ȯ
void shuffle(Deck *D);											// �� ���� 
void draw(Deck *D, Deck *S, Player *player);					// ī�� ��ο�

/* FND */ 
unsigned char asc_to_fnd(int n); 
void write_fnd(int dev, int dealer_score, int player_score);	// FND�� ���� �� �÷��̾��� ���� ǥ��
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
		/* ���� ���� ���� */ 
		int isContinue;
		/* ���� �� ����Ʈ �߻� ����	: 0�̸� ��� ���� ���� */
		int burst = 0;		
		/* ���� ���� ����			: 1�̸� �÷��̾� �¸�, -1�̸� ���� �¸�, 0�̸� ���º� */ 
		int score = 0;	
		
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
		
		printf("Player: "); print_deck(&player_stack); printf("  => %d��\n", player.score);
		printf("Dealer: "); print_deck(&dealer_stack); printf("  => %d��\n", dealer.score);

		// ���� ���� 
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
					case 0: printf("\n\n���º�!!"); break;	// ���⼭ ���� ���� �Լ��� �߰��ؾ� ��. 
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
				print_deck(&player_stack); printf("  => %d��\n", player.score);
				printf("Hit? (Y: 1, N: 0) "); scanf("%d", &player.state);
				if (player.state == HIT) {
					peek(&deck);
					draw(&deck, &player_stack, &player);
					printf(" => %d��", player.score);
					write_fnd(fnd_dev, dealer.score, player.score);								// ######	
				}
			}
			if (dealer.state == HIT) {
				printf("\n====================\nDealer Turn, ");
				print_deck(&dealer_stack); printf("  => %d��\n", dealer.score);
				if (dealer.score >= 17) {
					dealer.state = STAND; 
					printf("������ STAND�� �����߽��ϴ�.");
				}
				if (dealer.state == HIT) {
					peek(&deck);
					draw(&deck, &dealer_stack, &dealer);
					printf(" => %d��", dealer.score);
					write_fnd(fnd_dev, dealer.score, player.score);								// ######
				}
			}
			// �� ���� 
		} 
		printf("\n����Ͻðڽ��ϱ�? "); scanf("%d", &isContinue);
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
		case 0: printf("��"); break;
		case 1: printf("��"); break;
		case 2: printf("��"); break;
		case 3: printf("��"); break;
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
	if (pos < D->size) return 1;	// �ߺ��� ������ 1�� ��ȯ  
	return 0;						// �ߺ��� ������ 0�� ��ȯ 
}

void append(Deck *D, int element) {
	if (is_full(D)) error("full");
	D->list[D->size++] = element;
}

void peek(Deck *D) {
	int number = D->list[D->size-1];
	print_pattern(number / 11);
	printf(" %d ī�带 �̾ҽ��ϴ�.", number % 11 + 1);
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
	write(dev, message, strlen(message)); // �Ǵ� MAX_CLCD_CHAR
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
