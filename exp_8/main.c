/* =============================================================
 * main.c - Interface Grafica da Calculadora Binaria
 * Alvo: Raspberry Pi 3 (Cortex-A53, ARM64) - saida HDMI-VGA
 *
 * Fluxo (conforme fluxograma "Arquitetura Logica: ULA"):
 *   1. Interrupcao do Teclado (aqui via evento SDL_KEYDOWN,
 *      equivalente logico ao polling/IRQ do slide)
 *   2. Decodificador de OpCode        -> handle_key()
 *   3. Execucao na ULA (Assembly)     -> alu_add/sub/mul/fat/div
 *   4. Conversao Binario -> ASCII     -> snprintf/SDL_ttf
 *   5. Envio para Buffer de Video     -> SDL_RenderPresent (HDMI)
 * ============================================================= */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <limits.h>

/* ---------- Protótipos das rotinas escritas em alu.s ---------- */
extern int64_t alu_add(int64_t a, int64_t b, uint32_t *err);
extern int64_t alu_sub(int64_t a, int64_t b, uint32_t *err);
extern int64_t alu_mul(int64_t a, int64_t b, uint32_t *err);
extern int64_t alu_fat(int64_t n, uint32_t *err);
extern int64_t alu_div(int64_t a, int64_t b, uint32_t *err);

/* ---------- Configuração de janela / framebuffer --------------- */
#define WIN_W 400
#define WIN_H 560
#define FONT_PATH "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf"

/* ---------- Estado da máquina de estados da calculadora -------- */
typedef enum { OP_NONE, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_FAT } opcode_t;

typedef struct {
    char     display[64];   /* string mostrada no visor (ASCII)   */
    int64_t  operand_a;     /* registrador A (4 bits: 0-15)       */
    int64_t  operand_b;     /* registrador B (4 bits: 0-15)       */
    opcode_t pending_op;    /* operacao decodificada e pendente   */
    int      entering_b;    /* 0 = digitando A, 1 = digitando B   */
    int      error_state;   /* RNF01: flag de erro / exceção      */
} calc_state_t;

static calc_state_t st;

/* Reinicia o estado (usado no boot e apos qualquer erro tratado) */
static void calc_reset(void) {
    memset(&st, 0, sizeof(st));
    strcpy(st.display, "0");
}

/* -----------------------------------------------------------------
 * Decodificador de OpCode (equivalente ao bloco "Decodificador de
 * OpCode" do fluxograma): recebe a tecla e decide o que fazer.
 * ----------------------------------------------------------------- */
static void push_digit(int digit)
{
    int64_t *target = st.entering_b ? &st.operand_b : &st.operand_a;

    /* Qualquer tecla após erro reinicia a calculadora */
    if (st.error_state) {
        calc_reset();
    }

    /* Evita ultrapassar o limite de um inteiro de 32 bits */
    if (*target > (INT32_MAX - digit) / 10) {
        st.error_state = 1;
        strcpy(st.display, "ERRO: OVERFLOW");
        return;
    }

    *target = (*target * 10) + digit;

    snprintf(st.display,
             sizeof(st.display),
             "%lld",
             (long long)*target);
}

static void select_op(opcode_t op) {
    if (st.error_state) { calc_reset(); }
    st.pending_op  = op;
    st.entering_b  = (op != OP_FAT); /* fatorial usa só operando A */
    if (op == OP_FAT) {
        /* fatorial é unário: executa imediatamente */
        uint32_t err = 0;
        int64_t r = alu_fat(st.operand_a, &err);
        if (err) {
            st.error_state = 1;
            strcpy(st.display, "ERRO: OVERFLOW");
        } else {
            snprintf(st.display, sizeof(st.display), "%lld", (long long)r);
        }
    }
}

/* -----------------------------------------------------------------
 * Executa a operação pendente chamando a ULA (Assembly ARM64).
 * Corresponde ao bloco "Envio para ULA" + "Conversao Binario->ASCII"
 * ----------------------------------------------------------------- */
static void calc_equals(void) {
    uint32_t err = 0;
    int64_t result = 0;

    switch (st.pending_op) {
        case OP_ADD: result = alu_add(st.operand_a, st.operand_b, &err); break;
        case OP_SUB: result = alu_sub(st.operand_a, st.operand_b, &err); break;
        case OP_MUL: result = alu_mul(st.operand_a, st.operand_b, &err); break;
        case OP_DIV: result = alu_div(st.operand_a, st.operand_b, &err); break;
        default: return; /* nada pendente */
    }

    if (err) {
        /* RNF01: nunca deixa a aplicacao travar (kernel panic) */
        st.error_state = 1;
        strcpy(st.display, st.pending_op == OP_DIV
                             ? "ERRO: DIV/0" : "ERRO: OPERACAO");
    } else {
        snprintf(st.display, sizeof(st.display), "%lld", (long long)result);
    }
    st.pending_op = OP_NONE;
    st.entering_b = 0;
    st.operand_a  = result;   /* permite encadear operacoes */
    st.operand_b  = 0;
}

/* -----------------------------------------------------------------
 * Decodifica evento de teclado -> opcode (mapa RF01/Escopo requisitos)
 * ----------------------------------------------------------------- */
static void handle_key(SDL_Keycode key) {
    switch (key) {
        case SDLK_0: push_digit(0); break;
        case SDLK_1: push_digit(1); break;
        case SDLK_2: push_digit(2); break;
        case SDLK_3: push_digit(3); break;
        case SDLK_4: push_digit(4); break;
        case SDLK_5: push_digit(5); break;
        case SDLK_6: push_digit(6); break;
        case SDLK_7: push_digit(7); break;
        case SDLK_8: push_digit(8); break;
        case SDLK_9: push_digit(9); break;

        case SDLK_PLUS:
        case SDLK_KP_PLUS:      select_op(OP_ADD); break;
        case SDLK_MINUS:
        case SDLK_KP_MINUS:     select_op(OP_SUB); break;
        case SDLK_ASTERISK:
        case SDLK_KP_MULTIPLY:  select_op(OP_MUL); break;
        case SDLK_SLASH:
        case SDLK_KP_DIVIDE:    select_op(OP_DIV); break;
        case SDLK_EXCLAIM:
        case SDLK_1 & 0: break; /* placeholder, '!' tratado via TEXTINPUT abaixo */

        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_EQUALS:
        case SDLK_KP_EQUALS:    calc_equals(); break;

        case SDLK_c:
        case SDLK_ESCAPE:       calc_reset(); break;

        default: break;
    }
}

/* Trata o caractere '!' separadamente pois depende de layout de teclado */
static void handle_text(const char *text) {
    if (text[0] == '!') select_op(OP_FAT);
}

/* -----------------------------------------------------------------
 * Desenho da UI (equivalente ao "Envio para Buffer de Video HDMI")
 * ----------------------------------------------------------------- */
static void render_text(SDL_Renderer *ren, TTF_Font *font,
                         const char *text, int x, int y,
                         SDL_Color color) {
    SDL_Surface *surf = TTF_RenderText_Blended(font, text, color);
    SDL_Texture *tex  = SDL_CreateTextureFromSurface(ren, surf);
    SDL_Rect dst = { x, y, surf->w, surf->h };
    SDL_RenderCopy(ren, tex, NULL, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

static void draw_button(SDL_Renderer *ren, TTF_Font *font,
                         SDL_Rect rect, const char *label,
                         SDL_Color bg) {
    SDL_SetRenderDrawColor(ren, bg.r, bg.g, bg.b, 255);
    SDL_RenderFillRect(ren, &rect);
    SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
    SDL_RenderDrawRect(ren, &rect);
    SDL_Color white = {255, 255, 255, 255};
    render_text(ren, font, label, rect.x + rect.w/2 - 8, rect.y + rect.h/2 - 10, white);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    calc_reset();

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Erro SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "Erro TTF_Init: %s\n", TTF_GetError());
        return 1;
    }

    /* SDL_WINDOW_FULLSCREEN usa diretamente o framebuffer HDMI do Pi
       quando executado fora do X11 (KMSDRM) - ideal para o modo
       "standalone" do laboratorio. Em modo desktop, usar janela normal. */
    SDL_Window *win = SDL_CreateWindow("Calculadora ARM - Pi3",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_W, WIN_H, SDL_WINDOW_SHOWN);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont(FONT_PATH, 28);
    TTF_Font *font_small = TTF_OpenFont(FONT_PATH, 16);
    if (!font) fprintf(stderr, "Aviso: fonte nao encontrada em %s\n", FONT_PATH);

    const char *labels[4][4] = {
        {"7","8","9","/"},
        {"4","5","6","*"},
        {"1","2","3","-"},
        {"C","0","=","+"}
    };
    SDL_Rect btn_fat = {20, 470, 360, 60}; /* botao fatorial (!) full width */

    int running = 1;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {           /* -> "Interrupcao do Teclado" */
            if (e.type == SDL_QUIT) running = 0;
            else if (e.type == SDL_KEYDOWN) handle_key(e.key.keysym.sym);
            else if (e.type == SDL_TEXTINPUT) handle_text(e.text.text);
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x, my = e.button.y;
                if (mx >= btn_fat.x && mx <= btn_fat.x + btn_fat.w &&
                    my >= btn_fat.y && my <= btn_fat.y + btn_fat.h) {
                    select_op(OP_FAT);
                }
                for (int r = 0; r < 4; r++) {
                    for (int c = 0; c < 4; c++) {
                        SDL_Rect rc = {20 + c*90, 200 + r*65, 80, 55};
                        if (mx >= rc.x && mx <= rc.x + rc.w &&
                            my >= rc.y && my <= rc.y + rc.h) {
                            const char *l = labels[r][c];
                            if (l[0] >= '0' && l[0] <= '9') push_digit(l[0]-'0');
                            else if (!strcmp(l,"+")) select_op(OP_ADD);
                            else if (!strcmp(l,"-")) select_op(OP_SUB);
                            else if (!strcmp(l,"*")) select_op(OP_MUL);
                            else if (!strcmp(l,"/")) select_op(OP_DIV);
                            else if (!strcmp(l,"=")) calc_equals();
                            else if (!strcmp(l,"C")) calc_reset();
                        }
                    }
                }
            }
        }

        /* ---- Renderização (Buffer de Vídeo -> HDMI) ---- */
        SDL_SetRenderDrawColor(ren, 15, 18, 25, 255);
        SDL_RenderClear(ren);

        SDL_Rect display_rect = {20, 20, 360, 100};
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderFillRect(ren, &display_rect);
        SDL_Color green = st.error_state ? (SDL_Color){255,80,80,255}
                                          : (SDL_Color){0,255,120,255};
        if (font) render_text(ren, font, st.display, 30, 55, green);

        if (font_small) {
            char info[64];
            const char *opname = st.pending_op==OP_ADD?"+":st.pending_op==OP_SUB?"-":
                                  st.pending_op==OP_MUL?"*":st.pending_op==OP_DIV?"/":"";
            snprintf(info, sizeof(info), "A=%lld  OP=%s  B=%lld",
                     (long long)st.operand_a, opname, (long long)st.operand_b);
            SDL_Color gray = {150,150,150,255};
            render_text(ren, font_small, info, 30, 130, gray);
        }

        SDL_Color gray_btn = {60,65,80,255};
        SDL_Color op_btn   = {80,110,200,255};
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                SDL_Rect rc = {20 + c*90, 200 + r*65, 80, 55};
                const char *l = labels[r][c];
                int is_op = (!strcmp(l,"+")||!strcmp(l,"-")||!strcmp(l,"*")||
                             !strcmp(l,"/")||!strcmp(l,"="));
                if (font_small) draw_button(ren, font_small, rc, l, is_op?op_btn:gray_btn);
            }
        }
        SDL_Color fat_color = {200,140,40,255};
        if (font_small) draw_button(ren, font_small, btn_fat, "! (Fatorial)", fat_color);

        SDL_RenderPresent(ren);   /* <- efetivamente escreve no framebuffer HDMI */
        SDL_Delay(16);            /* ~60 FPS */
    }

    if (font) TTF_CloseFont(font);
    if (font_small) TTF_CloseFont(font_small);
    TTF_Quit();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
