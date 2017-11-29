#define VERSION_MAJOR 1
#define VERSION_MINOR 0

#define GAME_NAME "Scant Assembly (beta)"

#define GUI ImGui
#define PROGRAM_MAX 4096 // 4 KB
#define LINE_MAX_CHAR 27
#define CODE_MAX (PROGRAM_MAX * LINE_MAX_CHAR * 2)
#define INPUT_MAX 64 // 64 B
#define OUTPUT_MAX 64 // 64 B
#define RAM_SIZE 64 // 64 B
#define LETTER_INPUT_BIT 0x8000

#define CHARIFY(x) ((#x)[0])
#define LETTER(__letter__) ((int)CHARIFY(__letter__) | LETTER_INPUT_BIT)
#define WORD_INPUT1(__l1__) LETTER(__l1__), 0
#define WORD_INPUT2(__l1__, __l2__) LETTER(__l1__), LETTER(__l2__), 0
#define WORD_INPUT3(__l1__, __l2__, __l3__) LETTER(__l1__), LETTER(__l2__), LETTER(__l3__), 0
#define WORD_INPUT4(__l1__, __l2__, __l3__, __l4__) LETTER(__l1__), LETTER(__l2__), LETTER(__l3__), LETTER(__l4__), 0
#define WORD_INPUT5(__l1__, __l2__, __l3__, __l4__, __l5__) LETTER(__l1__), LETTER(__l2__), LETTER(__l3__), LETTER(__l4__), LETTER(__l5__), 0
#define WORD_INPUT6(__l1__, __l2__, __l3__, __l4__, __l5__, __l6__) LETTER(__l1__), LETTER(__l2__), LETTER(__l3__), LETTER(__l4__), LETTER(__l5__), LETTER(__l6__), 0
#define WORD_INPUT7(__l1__, __l2__, __l3__, __l4__, __l5__, __l6__, __l7__) LETTER(__l1__), LETTER(__l2__), LETTER(__l3__), LETTER(__l4__), LETTER(__l5__), LETTER(__l6__), LETTER(__l7__), 0
#define WORD_INPUT8(__l1__, __l2__, __l3__, __l4__, __l5__, __l6__, __l7__, __l8__) LETTER(__l1__), LETTER(__l2__), LETTER(__l3__), LETTER(__l4__), LETTER(__l5__), LETTER(__l6__), LETTER(__l7__), LETTER(__l8__), 0

#define IS_LETTER(__x__) ((__x__ & LETTER_INPUT_BIT) && (__x__ & ~LETTER_INPUT_BIT) >= 'A' && (__x__ & ~LETTER_INPUT_BIT) <= 'Z')
#define TO_NUMBER(__x__) (IS_LETTER(__x__) ? (__x__ & ~LETTER_INPUT_BIT) : __x__)
#define TO_LETTER(__x__) (__x__ | LETTER_INPUT_BIT)

#define INST_INP 0
#define INST_OUT 1
#define INST_SET 2
#define INST_LDA 3
#define INST_STA 4
#define INST_ADD 5
#define INST_SUB 6
#define INST_INC 7
#define INST_DEC 8
#define INST_JMP 9
#define INST_JPE 10
#define INST_JPL 11
#define INST_JPG 12
#define INST_NOP 13
