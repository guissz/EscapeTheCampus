#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h> // Formas primitivas
#include <allegro5/allegro_image.h> // Imagens

#define LARGURA_TELA 1280
#define ALTURA_TELA 720
#define FPS 60.0

typedef struct {
	float x, y;
	float largura, altura;
	float velocidade_x, velocidade_y;
	int frame_atual;
	int direcao;
	float tamanho_frame;
	ALLEGRO_BITMAP* spritesheet;
} Jogador;

typedef struct {
	int pontuacao;
	int segundos_restantes;
	int enigmas_resolvidos;
	int total_enigmas;
	int fase_atual;
} HUD;

typedef struct {
	int num1;
	int num2;
	int bits[8];
	int tentativa_atual;
	int tempo_erro;
	int tempo_acerto;
	bool resolvido;
	bool erro;
	bool acertou;
} EnigmaALU;

typedef struct {
	int valores_hex[4];
	int slots[4];
	int selecionado_origem;
	int tentativa_atual;
	int tempo_erro;
	int tempo_acerto;
	bool resolvido;
	bool erro;
	bool acertou;
} EnigmaRegistradores;

typedef struct {
	char codigo_linha1[64];
	char codigo_linha2[64];
	char codigo_linha3[64];
	char resposta_correta[32];
	char input_jogador[32];
	int tamanho_input;
	int tentativa_atual;
	int tempo_erro;
	int tempo_acerto;
	bool resolvido;
	bool erro;
	bool acertou;
} EnigmaAlgoritmo;

typedef enum { ESTADO_MENU, ESTADO_FASE1, ESTADO_ENIGMA1, ESTADO_ENIGMA2, ESTADO_ENIGMA3, ESTADO_SAIR } Estado;

void gerar_numeros_alu(EnigmaALU *alu) {
	alu->num1 = (rand() % 15) + 1;
	alu->num2 = (rand() % 15) + 1;

	for (int i = 0; i < 8; i++) {
		alu->bits[i] = 0;
	}
	alu->tentativa_atual = 0;
	alu->resolvido = false;
	alu->erro = false;
	alu->tempo_erro = 0;
	alu->acertou = false;
	alu->tempo_acerto = 0;
}

void gerar_registradores(EnigmaRegistradores *reg) {
	int conjuntos[4][4] = {
		{ 0x7B, 0x28, 0x8B, 0x59 },
		{ 0xA7, 0x60, 0xBF, 0x7A }, 
		{ 0x41, 0x0D, 0xCA, 0x40 }, 
		{ 0x86, 0x15, 0xB2, 0x43 }  
	};

	int indice = rand() % 4;
	for (int i = 0; i < 4; i++) {
		reg->slots[i] = conjuntos[indice][i];
		reg->valores_hex[i] = conjuntos[indice][i];
	}

	reg->selecionado_origem = -1;
	reg->tentativa_atual = 0;
	reg->resolvido = false;
	reg->erro = false;
	reg->tempo_erro = 0;
	reg->acertou = false;
	reg->tempo_acerto = 0;
}

void gerar_algoritmo(EnigmaAlgoritmo *alg) {
	sprintf_s(alg->codigo_linha1, sizeof(alg->codigo_linha1), "int vetor[5] = {10, 20, 30, 40, 50};");
	sprintf_s(alg->codigo_linha2, sizeof(alg->codigo_linha2), "for(int i = 0; i < ___; i++) {");
	sprintf_s(alg->codigo_linha3, sizeof(alg->codigo_linha3), "    soma += vetor[i]; }");
	sprintf_s(alg->resposta_correta, sizeof(alg->resposta_correta), "5");

	memset(alg->input_jogador, 0, sizeof(alg->input_jogador));
	alg->tamanho_input = 0;
	
	alg->tentativa_atual = 0;
	alg->resolvido = false;
	alg->erro = false;
	alg->tempo_erro = 0;
	alg->acertou = false;
	alg->tempo_acerto = 0;
}

int main() {

	al_init();
	al_init_font_addon();
	al_init_ttf_addon();
	al_install_keyboard();
	al_install_mouse();
	al_init_primitives_addon();
	al_init_image_addon();

	ALLEGRO_DISPLAY *display = al_create_display(LARGURA_TELA, ALTURA_TELA);
	al_set_window_title(display, "Escape The Campus - Base");

	ALLEGRO_TIMER *timer = al_create_timer(1.0 / FPS);
	ALLEGRO_EVENT_QUEUE *fila_eventos = al_create_event_queue();
	ALLEGRO_FONT *fonte = al_create_builtin_font(); // ALLEGRO_FONT *fonte = al_load_font("arial.ttf", 24, 0);
	ALLEGRO_BITMAP *mapa_fase1 = al_load_bitmap("mapa_fase1.png");

	al_register_event_source(fila_eventos, al_get_display_event_source(display));
	al_register_event_source(fila_eventos, al_get_timer_event_source(timer));
	al_register_event_source(fila_eventos, al_get_keyboard_event_source());
	al_register_event_source(fila_eventos, al_get_mouse_event_source());

	Estado estado_atual = ESTADO_MENU;
	srand(time(NULL));

	bool rodando = true;
	bool redesenhar = false;

	// Inicialização do jogador
	Jogador jogador;
	jogador.x = 150.0f;
	jogador.y = 300.0f;
	jogador.largura = 48.0f;
	jogador.altura = 48.0f;
	jogador.velocidade_x = 0.0f;
	jogador.velocidade_y = 0.0f;
	jogador.tamanho_frame = 48.0f;
	jogador.frame_atual = 0;
	jogador.direcao = 0;
	jogador.spritesheet = al_load_bitmap("player_spritesheet.png");

	// Inicialização do HUD
	HUD hud;
	hud.pontuacao = 10000;
	hud.segundos_restantes = 600;
	hud.enigmas_resolvidos = 0;
	hud.fase_atual = 1;
	hud.total_enigmas = 3;

	// Inicialização do Enigma ALU
	EnigmaALU alu;
	gerar_numeros_alu(&alu);

	// Inicialização do Enigma dos Registradores
	EnigmaRegistradores reg;
	gerar_registradores(&reg);

	// Inicialização do Enigma de Algoritmos
	EnigmaAlgoritmo alg;
	gerar_algoritmo(&alg);

	// Inicia o relógio para começar a contar o tempo
	static int contador_tempo = 0;
	al_start_timer(timer);

	// Game Loop principal
	while (rodando) {
		ALLEGRO_EVENT evento;

		// Aguarda o próximo evento acontecer na fila; fica em moda espera
		al_wait_for_event(fila_eventos, &evento);

		// Usuário fechou a janela com 'X'
		if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
			rodando = false;
		}

		else if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {
			if (evento.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
				if (estado_atual == ESTADO_ENIGMA1 || estado_atual == ESTADO_ENIGMA2 || estado_atual == ESTADO_ENIGMA3) {
					estado_atual = ESTADO_FASE1;
				} else {
					rodando = false;
				}
			}

			if (estado_atual == ESTADO_MENU) {
				if (evento.keyboard.keycode == ALLEGRO_KEY_ENTER) {
					estado_atual = ESTADO_FASE1; // Vai para a fase 1 ao apertar 'Enter'
				}
			}

			if (estado_atual == ESTADO_FASE1) {
				if (evento.keyboard.keycode == ALLEGRO_KEY_W || evento.keyboard.keycode == ALLEGRO_KEY_UP) {
					jogador.velocidade_y = -4.0f;
					jogador.direcao = 1;
				}

				else if (evento.keyboard.keycode == ALLEGRO_KEY_S || evento.keyboard.keycode == ALLEGRO_KEY_DOWN) {
					jogador.velocidade_y = 4.0f;
					jogador.direcao = 0;
				}

				else if (evento.keyboard.keycode == ALLEGRO_KEY_A || evento.keyboard.keycode == ALLEGRO_KEY_LEFT) {
					jogador.velocidade_x = -4.0f;
					jogador.direcao = 2;
				}

				else if (evento.keyboard.keycode == ALLEGRO_KEY_D || evento.keyboard.keycode == ALLEGRO_KEY_RIGHT) {
					jogador.velocidade_x = 4.0f;
					jogador.direcao = 3;
				}

				if (estado_atual == ESTADO_FASE1 && evento.keyboard.keycode == ALLEGRO_KEY_E) {
					// Interação com a ALU
					float margem = 30.0f;
					float meio_x = 400.0f - margem;
					float meio_y = 317.0f - margem;
					float meio_largura = 400.0f + (margem * 2);
					float meio_altura = 145.0f + (margem * 2);

					bool perto_da_alu = (jogador.x < meio_x + meio_largura && jogador.x + jogador.largura > meio_x &&
						jogador.y < meio_y + meio_altura && jogador.y + jogador.altura > meio_y);

					if (perto_da_alu && !alu.resolvido) {
						gerar_numeros_alu(&alu);
						estado_atual = ESTADO_ENIGMA1;
						redesenhar = true;
					}

					// Interação com os alocadores
					float alocador_x = 880.0f;
					float alocador_y = 200.0f;
					float alocador_largura = 200.0f;
					float alocador_altura = 150.0f;
					float margem_alocador = 30.0f;

					bool perto_do_alocador = (jogador.x < alocador_x + alocador_largura + margem_alocador &&
						jogador.x + jogador.largura > alocador_x - margem_alocador &&
						jogador.y < alocador_y + alocador_altura + margem_alocador &&
						jogador.y + jogador.altura > alocador_y - margem_alocador);
					/*
					if (perto_do_alocador) {
						printf("Perto do alocador! Resolvido: %d\n", reg.resolvido);
					}
					*/

					if (perto_do_alocador && !reg.resolvido) {
						//printf("A abrir o enigma 2...\n");
						gerar_registradores(&reg);
						estado_atual = ESTADO_ENIGMA2;
						redesenhar = true;
					}

					float pc_x = 1000.0f;
					float pc_y = 455.0f;
					float pc_largura = 280.0f;
					float pc_altura = 120.0f;
					float margem_pc = 30.0f;

					bool perto_pc = (jogador.x < pc_x + pc_largura + margem_pc && jogador.x + jogador.largura > pc_x - margem_pc &&
						jogador.y < pc_y + pc_altura + margem_pc && jogador.y + jogador.altura > pc_y - margem_pc);

					if (perto_pc && !alg.resolvido) {
						gerar_algoritmo(&alg);
						estado_atual = ESTADO_ENIGMA3;
						redesenhar = true;
					}
				}
			}
		}

		else if (evento.type == ALLEGRO_EVENT_KEY_UP) {
			if (estado_atual == ESTADO_FASE1) {
				if (evento.keyboard.keycode == ALLEGRO_KEY_W || evento.keyboard.keycode == ALLEGRO_KEY_UP ||
					evento.keyboard.keycode == ALLEGRO_KEY_S || evento.keyboard.keycode == ALLEGRO_KEY_DOWN) {
					jogador.velocidade_y = 0.0f;
				}
				if (evento.keyboard.keycode == ALLEGRO_KEY_A || evento.keyboard.keycode == ALLEGRO_KEY_LEFT ||
					evento.keyboard.keycode == ALLEGRO_KEY_D || evento.keyboard.keycode == ALLEGRO_KEY_RIGHT) {
					jogador.velocidade_x = 0.0f;
				}
			}
		}

		else if (evento.type == ALLEGRO_EVENT_KEY_CHAR) {
			// Lógica enigma 3
			if (estado_atual == ESTADO_ENIGMA3) {
				if (evento.keyboard.keycode == ALLEGRO_KEY_BACKSPACE) {
					if (alg.tamanho_input > 0) {
						alg.tamanho_input--;
						alg.input_jogador[alg.tamanho_input] = '\0';
					}
				}
				else if (evento.keyboard.keycode == ALLEGRO_KEY_ENTER) {
					if (strcmp(alg.input_jogador, alg.resposta_correta) == 0) {
						hud.enigmas_resolvidos++;
						
						if (alg.tentativa_atual == 0) {
							hud.pontuacao += 1000;
						}

						alg.resolvido = true;
						alg.acertou = true;
						alg.tempo_acerto = 90;
						//estado_atual = ESTADO_FASE1;
					}
					else {
						hud.pontuacao -= 500;
						if (hud.pontuacao < 0) hud.pontuacao = 0;
						alg.tentativa_atual = 1;
						alg.erro = true;
						alg.tempo_erro = 120;
					}
				}
				else {
					int keycode = evento.keyboard.keycode;
					char c = '\0';

					if (keycode >= ALLEGRO_KEY_0 && keycode <= ALLEGRO_KEY_9) {
						c = '0' + (keycode - ALLEGRO_KEY_0);
					}
					else if (keycode >= ALLEGRO_KEY_PAD_0 && keycode <= ALLEGRO_KEY_PAD_9) {
						c = '0' + (keycode - ALLEGRO_KEY_PAD_0);
					}

					if (c != '\0' && alg.tamanho_input < 31) {
						alg.input_jogador[alg.tamanho_input] = c;
						alg.tamanho_input++;
						alg.input_jogador[alg.tamanho_input] = '\0';
					}
				}
				redesenhar = true;
			}
		}

		else if (evento.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
			float mouse_x = evento.mouse.x;
			float mouse_y = evento.mouse.y;
			
			// Lógica Enigma 1
			if (estado_atual == ESTADO_ENIGMA1) {
				float x_inicio = LARGURA_TELA / 2 - 220;
				float y_bits = 260;
				float largura_bit = 45;
				float altura_bit = 60;

				for (int i = 0; i < 8; i++) {
					float bit_x = x_inicio + (i * 55);

					if (mouse_x >= bit_x && mouse_x <= bit_x + largura_bit && mouse_y >= y_bits && mouse_y <= y_bits + altura_bit) {
						alu.bits[i] = 1 - alu.bits[i];
						alu.erro = false;
						redesenhar = true;
						break;
					}
				}

				float botao_x = LARGURA_TELA / 2 - 100;
				float botao_y = 380;
				float botao_largura = 200;
				float botao_altura = 50;

				if (mouse_x >= botao_x && mouse_x <= botao_x + botao_largura && mouse_y >= botao_y && mouse_y <= botao_y + botao_altura) {
					int resposta_jogador = 0;
					for (int i = 0; i < 8; i++) {
						resposta_jogador = (resposta_jogador << 1) | alu.bits[i];
					}

					int resposta_correta = alu.num1 + alu.num2;

					if (resposta_jogador == resposta_correta) {
						hud.enigmas_resolvidos++;
						
						if (alu.tentativa_atual == 0) {
							hud.pontuacao += 1000;
						}
						alu.resolvido = true;
						alu.acertou = true;
						alu.tempo_acerto = 90;
						//estado_atual = ESTADO_FASE1;

					} else {
						hud.pontuacao -= 500;
						if (hud.pontuacao < 0) hud.pontuacao = 0;
						alu.tentativa_atual = 1;
						alu.erro = true;
						alu.tempo_erro = 120;
					}
					redesenhar = true;
				}
			}

			// Lógica enigma 2
			else if (estado_atual == ESTADO_ENIGMA2) {
				float largura_slot = 100;
				float altura_slot = 60;
				float inicio_x = LARGURA_TELA / 2 - 230;
				float y_slots = 280;

				for (int i = 0; i < 4; i++) {
					float slot_x = inicio_x + (i * 120);

					if (mouse_x >= slot_x && mouse_x <= slot_x + largura_slot &&
						mouse_y >= y_slots && mouse_y <= y_slots + altura_slot) {

						if (reg.selecionado_origem == -1) {
							reg.selecionado_origem = i; // Seleciona o primeiro bloco
						}
						else {
							// Efetua a troca (swap) exata entre os dois índices selecionados
							int temp = reg.slots[reg.selecionado_origem];
							reg.slots[reg.selecionado_origem] = reg.slots[i];
							reg.slots[i] = temp;

							// Reseta a seleção
							reg.selecionado_origem = -1;
						}
						reg.erro = false;
						redesenhar = true;
						break;
					}
				}

				float botao_x = LARGURA_TELA / 2 - 100;
				float botao_y = 380;
				float botao_largura = 200;
				float botao_altura = 50;

				if (mouse_x >= botao_x && mouse_x <= botao_x + botao_largura &&
					mouse_y >= botao_y && mouse_y <= botao_y + botao_altura) {

					// Validação rigorosa dos slots atuais na memória
					bool ordenado = true;
					for (int i = 0; i < 3; i++) {
						if (reg.slots[i] > reg.slots[i + 1]) {
							ordenado = false;
							break;
						}
					}

					if (ordenado) {
						hud.enigmas_resolvidos++;
						if (reg.tentativa_atual == 0) {
							hud.pontuacao += 1000;
						}
						reg.resolvido = true;
						reg.acertou = true;
						reg.tempo_acerto = 90;
						//estado_atual = ESTADO_FASE1;
					}
					else {
						hud.pontuacao -= 500;
						if (hud.pontuacao < 0) hud.pontuacao = 0;
						reg.tentativa_atual = 1;
						reg.erro = true;
						reg.tempo_erro = 120;
					}
					redesenhar = true;
				}
			}
		}

		// Se o relógio marcou mais 1/60 avos de segundo
		else if (evento.type == ALLEGRO_EVENT_TIMER) {
			redesenhar = true; // Tela será atualizada neste frame

			if (estado_atual != ESTADO_MENU) {
				contador_tempo++;

				if (contador_tempo >= 60) {
					if (hud.segundos_restantes > 0) {
						hud.segundos_restantes--;

						if (hud.pontuacao >= 10) {
							hud.pontuacao -= 10;
						} else {
							hud.pontuacao = 0;
						}
					} else {

					}
					contador_tempo = 0;
				}	
			}

			if (estado_atual == ESTADO_ENIGMA1 && alu.acertou) {
				alu.tempo_acerto--;
				if (alu.tempo_acerto <= 0) {
					alu.acertou = false;
					estado_atual = ESTADO_FASE1;
				}
			}

			if (estado_atual == ESTADO_ENIGMA2 && reg.acertou) {
				reg.tempo_acerto--;
				if (reg.tempo_acerto <= 0) {
					reg.acertou = false;
					estado_atual = ESTADO_FASE1;
				}
			}

			if (estado_atual == ESTADO_ENIGMA3 && alg.acertou) {
				alg.tempo_acerto--;
				if (alg.tempo_acerto <= 0) {
					alg.acertou = false;
					estado_atual = ESTADO_FASE1;
				}
			}

			if (estado_atual == ESTADO_ENIGMA1 && alu.erro) {
				alu.tempo_erro--;
				if (alu.tempo_erro <= 0) {
					alu.erro = false;
				}
			}

			if (estado_atual == ESTADO_ENIGMA2 && reg.erro) {
				reg.tempo_erro--;
				if (reg.tempo_erro <= 0) {
					reg.erro = false;
				}
			}

			if (estado_atual == ESTADO_ENIGMA3 && alg.erro) {
				alg.tempo_erro--;
				if (alg.tempo_erro <= 0) {
					alg.erro = false;
				}
			}

			if (estado_atual == ESTADO_FASE1) {
				jogador.x += jogador.velocidade_x;
				jogador.y += jogador.velocidade_y;

				bool colidiu = false;
				bool movendo = (jogador.velocidade_x != 0 || jogador.velocidade_y != 0);

				// Colisão bancadas e ALU
				if (jogador.x < 1280.0f && jogador.x + jogador.largura > 0.0f && jogador.y < 200.0f && jogador.y + jogador.altura > 0.0f) colidiu = true;
				if (jogador.x < 110.0f && jogador.x + jogador.largura > 0.0f && jogador.y < 720.0f && jogador.y + jogador.altura > 0.0f) colidiu = true;
				if (jogador.x < 1280.0f && jogador.x + jogador.largura > 0.0f && jogador.y < 720.0f && jogador.y + jogador.altura > 560.0f) colidiu = true;

				// Colisão ALU
				float meio_x = 400.0f;
				float meio_y = 317.0f;
				float meio_largura = 400.0f;
				float meio_altura = 145.0f;

				if (jogador.x < meio_x + meio_largura && jogador.x + jogador.largura > meio_x &&
					jogador.y < meio_y + meio_altura && jogador.y + jogador.altura > meio_y) {
					colidiu = true;
				}

				// Colisão Alocadores
				float alocador_x = 880.0f;
				float alocador_y = 200.0f;
				float alocador_largura = 800.0f;
				float alocador_altura = 150.0f;
				if (jogador.x < alocador_x + alocador_largura && jogador.x + jogador.largura > alocador_x &&
					jogador.y < alocador_y + alocador_altura && jogador.y + jogador.altura > alocador_y) {
					colidiu = true;
				}

				// Colisão PC
				float pc_x = 1000.0f;
				float pc_y = 455.0f;
				float pc_largura = 280.0f;
				float pc_altura = 120.0f;
				if (jogador.x < pc_x + pc_largura && jogador.x + jogador.largura > pc_x &&
					jogador.y < pc_y + pc_altura && jogador.y + jogador.altura > pc_y) {
					colidiu = true;
				}

				// Colisão borda direita
				if (jogador.x + jogador.largura > LARGURA_TELA) {
					jogador.x = LARGURA_TELA - jogador.largura;
				}

				if (colidiu) {
					jogador.x -= jogador.velocidade_x;
					jogador.y -= jogador.velocidade_y;
				}

				if (movendo) {
					static int contador_animacao = 0;
					contador_animacao++;
					if (contador_animacao >= 8) {
						jogador.frame_atual = (jogador.frame_atual + 1) % 7;
						contador_animacao = 0;
					}
				} else {
					jogador.frame_atual = 0;
				}
			}
		}

		// Só desenha se a fila estiver vazia
		if (redesenhar && al_is_event_queue_empty(fila_eventos)) {
			redesenhar = false;

			// Limpa a tela com cor de fundo azul escuro
			al_clear_to_color(al_map_rgb(0, 0, 0));

			switch (estado_atual) {
				case ESTADO_MENU:
					al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, ALTURA_TELA / 2 - 10,
						ALLEGRO_ALIGN_CENTRE, "Escape The Campus");
					al_draw_text(fonte, al_map_rgb(0, 255, 200), LARGURA_TELA / 2, ALTURA_TELA / 2 + 10,
						ALLEGRO_ALIGN_CENTRE, "Pressione Enter para jogar");
					break;

				case ESTADO_FASE1:
					al_draw_bitmap(mapa_fase1, 0, 0, 0);
					
					int minutos = hud.segundos_restantes / 60;
					int segundos = hud.segundos_restantes % 60;

					char texto_hud[128];
					sprintf_s(texto_hud, sizeof(texto_hud), "FASE: %d  |  TEMPO: %02d:%02d  |  PONTUACAO: %d  |  ENIGMAS: %d/%d",
						hud.fase_atual, minutos, segundos, hud.pontuacao, hud.enigmas_resolvidos, hud.total_enigmas);

					al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, 20, ALLEGRO_ALIGN_CENTRE, texto_hud);
					
					/* --- MODO DE DEBUG ---
					ALLEGRO_COLOR cor_debug = al_map_rgb(255, 0, 0); // Vermelho vivo

					// 1. Bancada Superior
					al_draw_rectangle(0.0f, 0.0f, 1280.0f, 200.0f, cor_debug, 2.0f);

					// 2. Bancada Lateral Esquerda
					al_draw_rectangle(0.0f, 0.0f, 110.0f, 720.0f, cor_debug, 2.0f);

					// 3. Bancada Inferior
					al_draw_rectangle(0.0f, 580.0f, 1280.0f, 720.0f, cor_debug, 2.0f);

					// 4. Bancada do Meio (usa as mesmas variáveis de tamanho que definiste nas colisões)
					float meio_x = 400.0f;
					float meio_y = 317.0f;
					float meio_largura = 400.0f;
					float meio_altura = 145.0f;
					al_draw_rectangle(meio_x, meio_y, meio_x + meio_largura, meio_y + meio_altura, cor_debug, 2.0f);

					// 5. Alocadores
					float alocador_x = 880.0f;
					float alocador_y = 200.0f;
					float alocador_largura = 800.0f;
					float alocador_altura = 150.0f;
					al_draw_rectangle(alocador_x, alocador_y, alocador_x + alocador_largura, alocador_y + alocador_altura, cor_debug, 2.0f);

					// 6. Pc
					float pc_x = 1000.0f;
					float pc_y = 455.0f;
					float pc_largura = 280.0f;
					float pc_altura = 120.0f;
					al_draw_rectangle(pc_x, pc_y, pc_x + pc_largura, pc_y + pc_altura, cor_debug, 2.0f);
					
					// 7. Caixa do Jogador (para veres exatamente onde a colisão do boneco atua)
					al_draw_rectangle(jogador_x, jogador_y, jogador_x + largura_jogador, jogador_y + altura_jogador, al_map_rgb(0, 255, 0), 2.0f); // Verde para o jogador
					*/

					float sx = jogador.frame_atual * jogador.tamanho_frame;
					float sy = jogador.direcao * jogador.tamanho_frame;
					al_draw_bitmap_region(jogador.spritesheet, sx, sy, jogador.tamanho_frame, jogador.tamanho_frame, jogador.x, jogador.y, 0);
					
					break;

				case ESTADO_ENIGMA1:
					al_clear_to_color(al_map_rgb(20, 30, 40));

					// Título Principal
					al_draw_text(fonte, al_map_rgb(0, 255, 255), LARGURA_TELA / 2, 30, ALLEGRO_ALIGN_CENTRE, "DECODIFICADOR ALU - SOMA BINARIA");

					// Instruções bem espaçadas para não sobrepor
					al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, 75, ALLEGRO_ALIGN_CENTRE, "COMO JOGAR:");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 100, ALLEGRO_ALIGN_CENTRE, "1. O computador le numeros somando potencias de 2 (0 = desligado, 1 = ligado).");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 120, ALLEGRO_ALIGN_CENTRE, "2. Da direita para a esquerda, os blocos valem: 1, 2, 4, 8, 16, 32, 64 e 128.");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 140, ALLEGRO_ALIGN_CENTRE, "3. Clique nos blocos para ligar/desligar ate atingir a soma correta!");

					// Operação matemática (com espaço seguro em relação às instruções)
					char operacao[64];
					sprintf_s(operacao, sizeof(operacao), "RESOLVA A SOMA: %d + %d = ?", alu.num1, alu.num2);
					al_draw_text(fonte, al_map_rgb(255, 255, 0), LARGURA_TELA / 2, 190, ALLEGRO_ALIGN_CENTRE, operacao);

					// Blocos de bits posicionados mais abaixo
					float x = LARGURA_TELA / 2 - 220; // Centralizado para 8 blocos
					float y = 260;

					for (int i = 0; i < 8; i++) {
						float bit_x = x + (i * 55);

						ALLEGRO_COLOR cor_caixa = (alu.bits[i] == 1) ? al_map_rgb(0, 200, 100) : al_map_rgb(80, 90, 100);
						al_draw_filled_rectangle(bit_x, y, bit_x + 45, y + 60, cor_caixa);
						al_draw_rectangle(bit_x, y, bit_x + 45, y + 60, al_map_rgb(255, 255, 255), 2.0f);

						char bit[4];
						sprintf_s(bit, sizeof(bit), "%d", alu.bits[i]);
						al_draw_text(fonte, al_map_rgb(255, 255, 255), bit_x + 22.5f, y + 20, ALLEGRO_ALIGN_CENTRE, bit);
					}

					if (alu.erro) {
						al_draw_text(fonte, al_map_rgb(255, 60, 60), LARGURA_TELA / 2, 380, ALLEGRO_ALIGN_CENTRE, "RESPOSTA INCORRETA! -500 pontos");
					}

					if (alu.acertou) {
						if (alu.tentativa_atual == 0) {
							al_draw_text(fonte, al_map_rgb(0, 255, 100), LARGURA_TELA / 2, 430, ALLEGRO_ALIGN_CENTRE, "CORRETO! +1000 pontos de primeira!");
						}
						else {
							al_draw_text(fonte, al_map_rgb(0, 255, 100), LARGURA_TELA / 2, 430, ALLEGRO_ALIGN_CENTRE, "CORRETO!");
						}
					}

					// Botão Confirmar
					float botao_x = LARGURA_TELA / 2 - 100;
					float botao_y = 360;
					al_draw_filled_rectangle(botao_x, botao_y, botao_x + 200, botao_y + 50, al_map_rgb(0, 120, 200));
					al_draw_rectangle(botao_x, botao_y, botao_x + 200, botao_y + 50, al_map_rgb(255, 255, 255), 2.0f);
					al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, botao_y + 18, ALLEGRO_ALIGN_CENTRE, "CONFIRMAR");
					break;

				case ESTADO_ENIGMA2:
					al_clear_to_color(al_map_rgb(30, 20, 40));

					// Título Principal
					al_draw_text(fonte, al_map_rgb(255, 150, 0), LARGURA_TELA / 2, 30, ALLEGRO_ALIGN_CENTRE, "ALOCACAO DE REGISTRADORES - HEXADECIMAL");

					// Instruções bem espaçadas
					al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, 50, ALLEGRO_ALIGN_CENTRE, "COMO JOGAR:");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 70, ALLEGRO_ALIGN_CENTRE, "1. Hexadecimal usa 0-9 e letras A-F (A=10, B=11, C=12, D=13, E=14, F=15).");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 90, ALLEGRO_ALIGN_CENTRE, "2. Ignore o '0x' e compare os valores (Ex: 0x15 e menor que 0x86).");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 110, ALLEGRO_ALIGN_CENTRE, "3. Objetivo: Organizar os 4 blocos da ESQUERDA (menor) para a DIREITA (maior).");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 130, ALLEGRO_ALIGN_CENTRE, "4. Clique num bloco para seleciona-lo (amarelo) e noutro para troca-los.");

					al_draw_text(fonte, al_map_rgb(255, 255, 0), LARGURA_TELA / 2, 190, ALLEGRO_ALIGN_CENTRE, "ORGANIZE OS SLOTS ABAIXO EM ORDEM CRESCENTE:");

					// Slots de registradores posicionados em segurança
					float largura_slot = 100;
					float altura_slot = 60;
					float inicio_x = LARGURA_TELA / 2 - 230;
					float y_slots = 260;

					for (int i = 0; i < 4; i++) {
						float slot_x = inicio_x + (i * 120);

						ALLEGRO_COLOR cor_slot = (reg.selecionado_origem == i) ? al_map_rgb(255, 200, 0) : al_map_rgb(60, 70, 90);
						al_draw_filled_rectangle(slot_x, y_slots, slot_x + largura_slot, y_slots + altura_slot, cor_slot);
						al_draw_rectangle(slot_x, y_slots, slot_x + largura_slot, y_slots + altura_slot, al_map_rgb(255, 255, 255), 2.0f);

						char texto_hex[16];
						sprintf_s(texto_hex, sizeof(texto_hex), "0x%02X", reg.slots[i]);
						al_draw_text(fonte, al_map_rgb(255, 255, 255), slot_x + (largura_slot / 2), y_slots + 20, ALLEGRO_ALIGN_CENTRE, texto_hex);
					}

					if (reg.erro) {
						al_draw_text(fonte, al_map_rgb(255, 60, 60), LARGURA_TELA / 2, 390, ALLEGRO_ALIGN_CENTRE, "RESPOSTA INCORRETA! -500 pontos");
					}

					if (reg.acertou) {
						if (reg.tentativa_atual == 0) {
							al_draw_text(fonte, al_map_rgb(0, 255, 100), LARGURA_TELA / 2, 430, ALLEGRO_ALIGN_CENTRE, "CORRETO! +1000 pontos de primeira!");
						}
						else {
							al_draw_text(fonte, al_map_rgb(0, 255, 100), LARGURA_TELA / 2, 430, ALLEGRO_ALIGN_CENTRE, "CORRETO!");
						}
					}

					// Botão Confirmar
					float b_x = LARGURA_TELA / 2 - 100;
					float b_y = 360;
					al_draw_filled_rectangle(b_x, b_y, b_x + 200, b_y + 50, al_map_rgb(0, 150, 100));
					al_draw_rectangle(b_x, b_y, b_x + 200, b_y + 50, al_map_rgb(255, 255, 255), 2.0f);
					al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, b_y + 18, ALLEGRO_ALIGN_CENTRE, "CONFIRMAR");
					break;

				case ESTADO_ENIGMA3:
					al_clear_to_color(al_map_rgb(20, 20, 35));

					al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, 55, ALLEGRO_ALIGN_CENTRE, "COMO JOGAR:");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 75, ALLEGRO_ALIGN_CENTRE, "1. Analise o trecho de codigo exibido no monitor.");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 95, ALLEGRO_ALIGN_CENTRE, "2. Digite o parametro que falta e aperte ENTER.");

					// Caixa estilo monitor/consola para o código
					al_draw_filled_rectangle(240, 140, 1040, 270, al_map_rgb(10, 10, 15));
					al_draw_rectangle(240, 140, 1040, 270, al_map_rgb(100, 100, 120), 2.0f);

					// Linhas de código do desafio
					al_draw_text(fonte, al_map_rgb(100, 255, 100), 260, 155, ALLEGRO_ALIGN_LEFT, alg.codigo_linha1);
					al_draw_text(fonte, al_map_rgb(100, 255, 100), 260, 185, ALLEGRO_ALIGN_LEFT, alg.codigo_linha2);
					al_draw_text(fonte, al_map_rgb(100, 255, 100), 260, 215, ALLEGRO_ALIGN_LEFT, alg.codigo_linha3);

					// Caixa onde o jogador escreve a resposta
					al_draw_text(fonte, al_map_rgb(255, 255, 0), LARGURA_TELA / 2, 300, ALLEGRO_ALIGN_CENTRE, "DIGITE A SUA RESPOSTA:");

					float input_x = LARGURA_TELA / 2 - 150;
					float input_y = 330;
					al_draw_filled_rectangle(input_x, input_y, input_x + 300, input_y + 50, al_map_rgb(40, 40, 60));
					al_draw_rectangle(input_x, input_y, input_x + 300, input_y + 50, al_map_rgb(255, 255, 255), 2.0f);

					// Mostra o texto que está a ser escrito pelo jogador com o cursor "_"
					char texto_input[64];
					sprintf_s(texto_input, sizeof(texto_input), "%s_", alg.input_jogador);
					al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, input_y + 18, ALLEGRO_ALIGN_CENTRE, texto_input);

					// Mensagem de erro temporária (se errar)
					if (alg.erro) {
						al_draw_text(fonte, al_map_rgb(255, 60, 60), LARGURA_TELA / 2, 400, ALLEGRO_ALIGN_CENTRE, "RESPOSTA INCORRETA! -500 pontos");
					}

					if (alg.acertou) {
						if (alg.tentativa_atual == 0) {
							al_draw_text(fonte, al_map_rgb(0, 255, 100), LARGURA_TELA / 2, 400, ALLEGRO_ALIGN_CENTRE, "CORRETO! +1000 pontos de primeira!");
						}
						else {
							al_draw_text(fonte, al_map_rgb(0, 255, 100), LARGURA_TELA / 2, 400, ALLEGRO_ALIGN_CENTRE, "CORRETO!");
						}
					}
					break;

				default:
					break;
			}

			// Atualiza o display mostrando tudo que foi desenhado
			al_flip_display();
		}
	}

	// Limpeza de memória
	al_destroy_bitmap(mapa_fase1);
	al_destroy_bitmap(jogador.spritesheet);
	al_destroy_font(fonte);
	al_destroy_event_queue(fila_eventos);
	al_destroy_timer(timer);
	al_destroy_display(display);

	return 0;
}