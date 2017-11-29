#include <stdlib.h>
#include <functional>
#include <vector>
#include <algorithm>

#include "constants.h"
#include "dictionnary.h"

typedef void (*InputGenFunc) (int*, int&, int*, int&);

struct Level
{
    int unique_id;
    char* name;
    char* description;
    int safe_inputs[INPUT_MAX];
    int safe_input_count;
    int safe_outputs[OUTPUT_MAX];
    int safe_output_count;
    int ram[RAM_SIZE];
    InputGenFunc input_gen;
    int inputs[INPUT_MAX];
    int input_count;
    int outputs[OUTPUT_MAX];
    int output_count;
    bool passed;
};

bool rand_chance(int chance)
{
    return (rand() % 100) < chance;
}

int rand_number(int min, int max)
{
    ++max;
    return (rand() % (max - min)) + min;
}

void IN_set(int value, int* in, int& in_count)
{
    if (in_count == INPUT_MAX) return;
    in[in_count] = value;
    ++in_count;
}

void IN_add_number(int min, int max, int* in, int& in_count)
{
    if (in_count == INPUT_MAX) return;
    in[in_count] = rand_number(min, max);
    ++in_count;
}

void IN_add_letter(int min, int max, int* in, int& in_count)
{
    if (in_count == INPUT_MAX) return;
    in[in_count] = TO_LETTER(rand_number(min, max));
    ++in_count;
}

std::string get_word(int letter_count)
{
    auto& words = dictionnary[letter_count - 2];
    auto i = rand() % words.count;
    std::string word = words.words[i];
    std::transform(word.begin(), word.end(), word.begin(), toupper);
    return word;
}

std::string IN_add_word(int letter_count, int* in, int& in_count)
{
    auto word = get_word(letter_count);
    for (int i = 0; i < (int)word.size(); ++i)
    {
        IN_set(TO_LETTER((int)toupper(word[i])), in, in_count);
    }
    IN_set(0, in, in_count);
    return word;
}

void OUT_set(int value, int* out, int& out_count)
{
    if (out_count == OUTPUT_MAX) return;
    out[out_count] = value;
    ++out_count;
}

std::string OUT_add_word(int letter_count, int* out, int& out_count)
{
    auto word = get_word(letter_count);
    for (int i = 0; i < (int)word.size(); ++i)
    {
        IN_set(TO_LETTER((int)toupper(word[i])), out, out_count);
    }
    OUT_set(0, out, out_count);
    return word;
}

void OUT_set_word(const std::string& word, int* out, int& out_count)
{
    for (int i = 0; i < (int)word.size(); ++i)
    {
        IN_set(TO_LETTER((int)toupper(word[i])), out, out_count);
    }
    OUT_set(0, out, out_count);
}

Level levels[] = {
    {
        1,
        "In and Out", "Copy inputs to outputs.",
        { 56, LETTER(H), -13, 0, 1, 1, LETTER(V), 0 }, 8,
        { 56, LETTER(H), -13, 0, 1, 1, LETTER(V), 0 }, 8,
        { 0 },
        [](int* in, int& in_count, int* out, int& out_count)
        {
            for (int i = 0; i < 8; ++i)
            {
                if (rand_chance(30)) IN_add_letter('A', 'Z', in, in_count);
                else IN_add_number(-99, 99, in, in_count);
            }
            for (int i = 0; i < in_count; ++i)
            {
                OUT_set(in[i], out, out_count);
            }
        }
    },
    {
        14,
        "Set", "Output the sequence 10, T, -3.",
        { 0 }, 0,
        { 10, LETTER(T), -3 }, 3,
        { 0 }
    },
    {
        2,
        "Add", "For each 2 inputs, output the sum.",
        { -6, 8, -1,   7, -3 ,  1 ,  5 ,  -5, -10 ,  9, -7, -9 ,  0,  10 , 0, 0 }, 16,
        { 2, 6, -2, 0, -1, -16, 10, 0 }, 8,
        { 0 },
        [](int* in, int& in_count, int* out, int& out_count)
        {
            auto count = rand_number(10, 20);
            count = (count / 2) * 2;
            for (int i = 0; i < count; ++i)
            {
                IN_add_number(-40, 40, in, in_count);
            }
            for (int i = 0; i < in_count; i += 2)
            {
                OUT_set(in[i] + in[i + 1], out, out_count);
            }
        }
    },
    {
        13,
        "substract", "For each 2 inputs, output the difference of the first minus the second.",
        { -6,   8, -1,   7, -3 ,  1 ,  5 ,  5, -10 ,  9, -7, -9 ,  0,  10 ,  0, 0 }, 16,
        { -14, -8, -4, 0, -19, 2, -10, 0 }, 8,
        { 0 },
        [](int* in, int& in_count, int* out, int& out_count)
        {
            auto count = rand_number(10, 20);
            count = (count / 2) * 2;
            for (int i = 0; i < count; ++i)
            {
                IN_add_number(-40, 40, in, in_count);
            }
            for (int i = 0; i < in_count; i += 2)
            {
                OUT_set(in[i] - in[i + 1], out, out_count);
            }
        }
    },
    {
        5,
        "No T", "Copy inputs to outputs, except for the letter T. Tip: you can SUB letters together.",
        { LETTER(A), LETTER(B),LETTER(T),LETTER(I),LETTER(V),LETTER(T),LETTER(T),LETTER(U),LETTER(S),LETTER(S),LETTER(Q),LETTER(T) }, 12,
        { LETTER(A), LETTER(B),LETTER(I),LETTER(V),LETTER(U),LETTER(S),LETTER(S),LETTER(Q) }, 8,
        { LETTER(T) },
        [](int* in, int& in_count, int* out, int& out_count)
        {
            auto count = rand_number(10, 20);
            count = (count / 2) * 2;
            for (int i = 0; i < count; ++i)
            {
                if (rand_chance(30)) IN_add_letter('T', 'T', in, in_count);
                else IN_add_letter('A', 'Z', in, in_count);
            }
            for (int i = 0; i < in_count; ++i)
            {
                if (in[i] != TO_LETTER('T'))
                {
                    OUT_set(in[i], out, out_count);
                }
            }
        }
    },
    {
        10,
        "Sequence", "Output a number sequence from 1 to 20.",
        {}, 0,
        { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 }, 20,
        { 20 }
    },
    {
        15,
        "Secret Message", "Each 2 inputs added together is the address of a letter. Output the secret message.",
        {/*20*/15,5,/*8*/9,-1,/*5*/5,0,/*0*/-1,1, /*2*/34,-32,/*9*/4,5,/*18*/9,9,/*4*/0,4,/*0*/0,0, /*9*/1,8,/*19*/20,-1,/*0*/-5,5, /*6*/3,3,/*12*/3,9,/*25*/24,1,/*9*/13,-4,/*14*/0,14,/*7*/3,4,/*0*/0,0, /*12*/85,-73,/*15*/-44,59,/*23*/20,3,/*0*/-12,12 }, 46,
        { WORD_INPUT3(T,H,E), WORD_INPUT4(B,I,R,D), WORD_INPUT2(I,S), WORD_INPUT6(F,L,Y,I,N,G), WORD_INPUT3(L,O,W)}, 23,
        { 0,      LETTER(A),LETTER(B),LETTER(C),LETTER(D),LETTER(E),LETTER(F),LETTER(G),
        LETTER(H),LETTER(I),LETTER(J),LETTER(K),LETTER(L),LETTER(M),LETTER(N),LETTER(O),
        LETTER(P),LETTER(Q),LETTER(R),LETTER(S),LETTER(T),LETTER(U),LETTER(V),LETTER(W),
        LETTER(X),LETTER(Y),LETTER(Z) },
        [](int* in, int& in_count, int* out, int& out_count)
        {
            auto count = rand_number(4, 8);
            for (int i = 0; i < count; ++i)
            {
                auto letter_count = rand_number(2, 8);
                if (in_count + letter_count * 2 + 2 > INPUT_MAX) break;
                auto word = OUT_add_word(letter_count, out, out_count);
                for (int j = 0; j <= (int)word.size(); ++j)
                {
                    auto c = (word[j] == '\0') ? 0 : ((int)word[j] - (int)'A' + 1);
                    auto first = rand_number(-99, 99);
                    auto second = c - first;
                    IN_set(first, in, in_count);
                    IN_set(second, in, in_count);
                }
            }
        }
    },
    {
        11,
        "Sequences", "For each input n, output a number sequence from 1 to n.",
        { 5, 10, 8, 6, 1 }, 5,
        { 1,2,3,4,5,1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,1,2,3,4,5,6,1 }, 30,
        { 0 },
        [](int* in, int& in_count, int* out, int& out_count)
        {
            auto count = rand_number(4, 8);
            for (int i = 0; i < count; ++i)
            {
                IN_add_number(1, 10, in, in_count);
            }
            for (int i = 0; i < in_count; ++i)
            {
                for (int j = 1; j <= in[i]; ++j)
                {
                    OUT_set(j, out, out_count);
                }
            }
        }
    },
    {
        8,
        "Word size", "For each zero terminated word, count how many letters and output the result.",
        { WORD_INPUT2(M, Y),
          WORD_INPUT6(S, H, O, V, E, L),
          WORD_INPUT3(H, A, S),
          WORD_INPUT4(S, N, O, W) }, 19,
        { 2, 6, 3, 4 }, 4,
        { 0 },
        [](int* in, int& in_count, int* out, int& out_count)
        {
            auto count = rand_number(4, 8);
            for (int i = 0; i < count; ++i)
            {
                auto word = IN_add_word(rand_number(2, 6), in, in_count);
                OUT_set((int)word.size(), out, out_count);
            }
        }
    },
    {
        12,
        "Count down", "For each input n, count down from n to 0. If n is negative, count up from n to 0.",
        { 5, -10, -8, 6, 0 }, 5,
        { 5,4,3,2,1,0,-10,-9,-8,-7,-6,-5,-4,-3,-2,-1,0,-8,-7,-6,-5,-4,-3,-2,-1,0,6,5,4,3,2,1,0,0 }, 34,
        { 0 },
        [](int* in, int& in_count, int* out, int& out_count)
        {
            auto count = rand_number(4, 8);
            for (int i = 0; i < count; ++i)
            {
                IN_add_number(-10, 10, in, in_count);
            }
            for (int i = 0; i < in_count; ++i)
            {
                int dir = in[i] < 0 ? 1 : -1;
                for (int j = in[i]; j != 0; j += dir)
                {
                    OUT_set(j, out, out_count);
                }
                OUT_set(0, out, out_count);
            }
        }
    },
    {
        6,
        "Same Sign", "For each 2 inputs, output 0 if the sign is the same. Output 1 if different sign.",
        { 1, -2, -3, -4, 5, 6, -7, 8 }, 8,
        { 1, 0, 0, 1 }, 4,
        { 0 },
        [](int* in, int& in_count, int* out, int& out_count)
        {
            auto count = rand_number(10, 20);
            count = (count / 2) * 2;
            for (int i = 0; i < count; ++i)
            {
                if (rand_chance(50)) IN_add_number(-10, -1, in, in_count);
                else IN_add_number(1, 10, in, in_count);
            }
            for (int i = 0; i < in_count; i += 2)
            {
                OUT_set(
                    ((in[i] < 0 && in[i + 1] < 0) || (in[i] > 0 && in[i + 1] > 0)) ? 0 : 1,
                    out, out_count);
            }
        }
    },
    {
        18,
        "Number or Letter", "Output only letters, discard numbers.",
        { LETTER(A), 5, -10, LETTER(B), LETTER(C), -8, 6, 0 }, 8,
        { LETTER(A), LETTER(B), LETTER(C) }, 3,
        { 0 },
        [](int* in, int& in_count, int* out, int& out_count)
        {
            auto word = get_word(rand_number(3, 8));
            for (int i = 0; i < (int)word.size() + 1; ++i)
            {
                auto ins = rand_number(0, 3);
                for (int j = 0; j < ins; ++j)
                {
                    IN_add_number(-10, 10, in, in_count);
                }
                if (i < (int)word.size())
                {
                    IN_set(TO_LETTER(word[i]), in, in_count);
                    OUT_set(TO_LETTER(word[i]), out, out_count);
                }
            }
        }
    },
    {
        3,
        "Reverse Words", "Inverse each zero terminated word.",
        { WORD_INPUT3(H,O,T), WORD_INPUT6(P,O,T,A,T,O)}, 11,
        { WORD_INPUT3(T,O,H), WORD_INPUT6(O,T,A,T,O,P) }, 11,
        { 0 },
        [](int* in, int& in_count, int* out, int& out_count)
        {
            auto count = rand_number(4, 8);
            for (int i = 0; i < count; ++i)
            {
                auto letter_count = rand_number(2, 6);
                auto word = get_word(letter_count);
                for (int j = 0; j < (int)word.size(); ++j)
                {
                    IN_set(TO_LETTER(word[j]), in, in_count);
                    OUT_set(TO_LETTER(word[(int)word.size() - j - 1]), out, out_count);
                }
                IN_set(0, in, in_count);
                OUT_set(0, out, out_count);
            }
        }
    },
    {
        16,
        "Multiply", "For each 2 inputs, output their product.",
        { 1, 2, 4, 4, 9, 5, 0, 6, 2, 3, 0, 0, 7, 0 }, 14,
        { 2, 16, 45, 0, 6, 0, 0 }, 7,
        { 0 },
        [](int* in, int& in_count, int* out, int& out_count)
        {
            auto count = rand_number(10, 20);
            count = (count / 2) * 2;
            for (int i = 0; i < count; ++i)
            {
                IN_add_number(0, 10, in, in_count);
            }
            for (int i = 0; i < in_count; i += 2)
            {
                OUT_set(in[i] * in[i + 1], out, out_count);
            }
        }
    },
    {
        17,
        "Division", "For each 2 inputs, first one divided by second.",
        { 4, 2, 10, 2, 9, 5, 0, 6, 45, 9, 15, 1 }, 12,
        { 2, 5, 1, 0, 5, 15 }, 6,
        { 0 },
        [](int* in, int& in_count, int* out, int& out_count)
        {
            auto count = rand_number(10, 20);
            count = (count / 2) * 2;
            for (int i = 0; i < count; ++i)
            {
                IN_add_number(i % 2, (i % 2 == 0) ? 99 : 10, in, in_count);
            }
            for (int i = 0; i < in_count; i += 2)
            {
                OUT_set(in[i] / in[i + 1], out, out_count);
            }
        }
    },
    {
        4,
        "Order", "Output the zero terminated sequence in ascending order. Do not include the zero.",
        { 53, 15, 1, 89, 44, 1, 15, 12, 0 }, 9,
        { 1, 1, 12, 15, 15, 44, 53, 89 }, 8,
        { 0 },
        [](int* in, int& in_count, int* out, int& out_count)
        {
            auto count = rand_number(8, 15);
            std::vector<int> sorted;
            for (int i = 0; i < count; ++i)
            {
                IN_add_number(1, 99, in, in_count);
                sorted.push_back(in[i]);
            }
            IN_set(0, in, in_count);
            std::sort(sorted.begin(), sorted.end());
            for (auto i : sorted)
            {
                OUT_set(i, out, out_count);
            }
        }
    },
    {
        9,
        "Alphabetical", "For each two words, output the word that would come first if sorted in alphabetical order.",
        { WORD_INPUT4(C,O,L,D), WORD_INPUT4(H,E,A,T),
          WORD_INPUT7(A,N,T,I,Q,U,E), WORD_INPUT7(A,N,T,I,G,E,L),
          WORD_INPUT3(B,I,G), WORD_INPUT6(B,I,G,G,E,R),
          WORD_INPUT6(B,I,G,G,E,R), WORD_INPUT3(B,I,G)
        }, 48,
        { WORD_INPUT4(C,O,L,D), WORD_INPUT7(A,N,T,I,G,E,L), WORD_INPUT3(B,I,G) ,WORD_INPUT3(B,I,G) }, 21,
        { 0, 0, 0, 0, 0, 0, 0, 0,
          0, 0, 0, 0, 0, 0, 0, 0,
          0, 0, 0, 0, 0, 0, 0, 0,
          0, 0, 0, 0, 0, 0, 0, 0,
          16},
        [](int* in, int& in_count, int* out, int& out_count)
        {
            auto count = 4;
            for (int i = 0; i < count; ++i)
            {
                auto word1 = IN_add_word(rand_number(3, 7), in, in_count);
                auto word2 = IN_add_word(rand_number(3, 7), in, in_count);
                if (word1 <= word2)
                {
                    OUT_set_word(word1, out, out_count);
                }
                else
                {
                    OUT_set_word(word2, out, out_count);
                }
            }
        }
    },
    {
        7,
        "Fibonacci", "For each input, output the Fibonacci sequence starting with 1 1. Do not go over the input number.",
        { 5,  12, 240, 36,  25,  400, 7,   3 }, 8,
        { 1,  1,  2,   3,   5,   1,   1,   2,
          3,  5,  8,   1,   1,   2,   3,   5,
          8,  13, 21,  34,  55,  89,  144, 233,
          1,  1,  2,   3,   5,   8,   13,  21,
          34, 1,  1,   2,   3,   5,   8,   13,
          21, 1,  1,   2,   3,   5,   8,   13,
          21, 34, 55,  89,  144, 233, 377, 1,
          1,  2,  3,   5,   1,   1,   2,   3 }, 64,
        { 0 },
        [](int* in, int& in_count, int* out, int& out_count)
        {
            const int fib_seq[] = { 1,	1,	2,	3,	5,	8,	13,	21,	34,	55,	89,	144, 233, 377, 610 };
            for (int i = 0; i < 8; ++i)
            {
                auto num = rand_number(1, 500);
                num = (num * num) / 500;
                num = std::max(num, 1);
                IN_set(num, in, in_count);
                int last_count = out_count;
                bool stop = false;
                for (int j = 0; fib_seq[j] <= in[i]; ++j)
                {
                    if (out_count == OUTPUT_MAX)
                    {
                        stop = true;
                        out_count = last_count;
                        in[in_count] = 0;
                        in_count--;
                        break;
                    }
                    OUT_set(fib_seq[j], out, out_count);
                }
                if (stop || out_count == OUTPUT_MAX) break;
            }
        }
    },
};

const int level_count = sizeof(levels) / sizeof(Level);
