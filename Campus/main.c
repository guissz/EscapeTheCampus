#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>

#define LARGURA_TELA 1280
#define ALTURA_TELA 720
#define FPS 60.0

// STRUCTS
typedef struct {
	float x, y;
	float largura, altura;
	float velocidade_x, velocidade_y;
	int frame_atual;
	int direcao;
	float tamanho_frame;
	ALLEGRO_BITMAP *spritesheet;
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

// MÁQUINA DE ESTADO
typedef enum { ESTADO_MENU, ESTADO_FASE1, ESTADO_ENIGMA1, ESTADO_ENIGMA2, ESTADO_ENIGMA3, ESTADO_SAIR } Estado;

// FUNÇÕES
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

	// INICIALIZAÇÃO DO ALLEGRO
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

	// INICIALIZAÇÃO DO JOGADOR
	Jogador jogador;
	jogador.x = 150.0f;
	jogador.y = 300.0f;
	jogador.largura = 96.0f;
	jogador.altura = 96.0f;
	jogador.velocidade_x = 0.0f;
	jogador.velocidade_y = 0.0f;
	jogador.tamanho_frame = 48.0f;
	jogador.frame_atual = 0;
	jogador.direcao = 0;
	jogador.spritesheet = al_load_bitmap("player_spritesheet.png");

	// INICIALIZAÇÃO DA HUD
	HUD hud;
	hud.pontuacao = 10000;
	hud.segundos_restantes = 600;
	hud.enigmas_resolvidos = 0;
	hud.fase_atual = 1;
	hud.total_enigmas = 3;

	// INICIALIZAÇÃO DO ENIGMA 1 (ALU)
	EnigmaALU alu;
	gerar_numeros_alu(&alu);

	// INICIALIZAÇÃO DO ENIGMA 2 (REGISTRADORES)
	EnigmaRegistradores reg;
	gerar_registradores(&reg);

	// INICIALIZAÇÃO DO ENIGMA 3 (ALGORITMO)
	EnigmaAlgoritmo alg;
	gerar_algoritmo(&alg);

	// INICIALIZAÇÃO DO TEMPORIZADOR
	static int contador_tempo = 0;
	al_start_timer(timer);

	// Game Loop
	while (rodando) {
		ALLEGRO_EVENT evento;

		al_wait_for_event(fila_eventos, &evento);

		if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
			rodando = false;
		}

		// CONTROLE DE TECLAS
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
					estado_atual = ESTADO_FASE1;
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
					float margem = 20.0f;
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
				float y_bits = 200;
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
				float botao_y = 350;
				float botao_largura = 200;
				float botao_altura = 50;

				if (mouse_x >= botao_x && mouse_x <= botao_x + botao_largura && mouse_y >= botao_y && mouse_y <= botao_y + botao_altura) {
					int resposta_jogador = 0;
					for (int i = 0; i < 8; i++) {
						resposta_jogador = (resposta_jogador * 2) + alu.bits[i];
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

					}
					else {
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
				float y_slots = 210;

				for (int i = 0; i < 4; i++) {
					float slot_x = inicio_x + (i * 120);

					if (mouse_x >= slot_x && mouse_x <= slot_x + largura_slot &&
						mouse_y >= y_slots && mouse_y <= y_slots + altura_slot) {

						if (reg.selecionado_origem == -1) {
							reg.selecionado_origem = i;
						}
						else {
							int temp = reg.slots[reg.selecionado_origem];
							reg.slots[reg.selecionado_origem] = reg.slots[i];
							reg.slots[i] = temp;
							reg.selecionado_origem = -1;
						}
						reg.erro = false;
						redesenhar = true;
						break;
					}
				}

				float botao_x = LARGURA_TELA / 2 - 100;
				float botao_y = 340;
				float botao_largura = 200;
				float botao_altura = 50;

				if (mouse_x >= botao_x && mouse_x <= botao_x + botao_largura &&
					mouse_y >= botao_y && mouse_y <= botao_y + botao_altura) {

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

		else if (evento.type == ALLEGRO_EVENT_TIMER) {
			redesenhar = true;

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

			// Fase 1
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
				float meio_largura = 385.0f;
				float meio_altura = 146.0f;

				if (jogador.x < meio_x + meio_largura && jogador.x + jogador.largura > meio_x &&
					jogador.y < meio_y + meio_altura && jogador.y + jogador.altura > meio_y) {
					colidiu = true;
				}

				// Colisão Alocadores
				float alocador_x = 890.0f;
				float alocador_y = 200.0f;
				float alocador_largura = 210.0f;
				float alocador_altura = 120.0f;
				if (jogador.x < alocador_x + alocador_largura && jogador.x + jogador.largura > alocador_x &&
					jogador.y < alocador_y + alocador_altura && jogador.y + jogador.altura > alocador_y) {
					colidiu = true;
				}

				// Colisão PC
				float pc_x = 1000.0f;
				float pc_y = 460.0f;
				float pc_largura = 300.0f;
				float pc_altura = 115.0f;
				if (jogador.x < pc_x + pc_largura && jogador.x + jogador.largura > pc_x &&
					jogador.y < pc_y + pc_altura && jogador.y + jogador.altura > pc_y) {
					colidiu = true;
				}

				// Colisão borda direita
				if (jogador.x + jogador.largura > LARGURA_TELA) {
					jogador.x = LARGURA_TELA - jogador.largura;
				}

				// Colisão porta
				float porta_x = 1180.0f;
				float porta_y = 200.0f;
				float porta_largura = 100.0f;
				float porta_altura = 160.0f;

				if (jogador.x < porta_x + porta_largura && jogador.x + jogador.largura > porta_x &&
					jogador.y < porta_y + porta_altura && jogador.y + jogador.altura > porta_y) {
					colidiu = true;
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

		if (redesenhar && al_is_event_queue_empty(fila_eventos)) {
			redesenhar = false;

			al_clear_to_color(al_map_rgb(0, 0, 0));

			switch (estado_atual) {
				//MENU PRINCIPAL
				case ESTADO_MENU:
					al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, ALTURA_TELA / 2 - 10,
						ALLEGRO_ALIGN_CENTRE, "Escape The Campus");
					al_draw_text(fonte, al_map_rgb(0, 255, 200), LARGURA_TELA / 2, ALTURA_TELA / 2 + 10,
						ALLEGRO_ALIGN_CENTRE, "Pressione Enter para jogar");
					break;
				
				// FASE 1
				case ESTADO_FASE1:
					al_draw_bitmap(mapa_fase1, 0, 0, 0);
					
					int minutos = hud.segundos_restantes / 60;
					int segundos = hud.segundos_restantes % 60;

					char texto_hud[128];
					sprintf_s(texto_hud, sizeof(texto_hud), "FASE: %d  |  TEMPO: %02d:%02d  |  PONTUACAO: %d  |  ENIGMAS: %d/%d", hud.fase_atual, minutos, segundos, hud.pontuacao, hud.enigmas_resolvidos, hud.total_enigmas);

					al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, 20, ALLEGRO_ALIGN_CENTRE, texto_hud);
					
					/* DEBUG
					ALLEGRO_COLOR cor_debug = al_map_rgb(255, 0, 0); // Vermelho vivo
					// 1. Bancada Superior
					al_draw_rectangle(0.0f, 0.0f, 1280.0f, 200.0f, cor_debug, 2.0f);
					// 2. Bancada Lateral Esquerda
					al_draw_rectangle(0.0f, 0.0f, 110.0f, 720.0f, cor_debug, 2.0f);
					// 3. Bancada Inferior
					al_draw_rectangle(0.0f, 580.0f, 1280.0f, 720.0f, cor_debug, 2.0f);
					// 4. Terminal 1 (ALU)
					al_draw_rectangle(400.0f, 317.0f, 400.0f + 385.0f, 317.0f + 146.0f, cor_debug, 2.0f);
					// 5. Terminal 2 (Registradores) - VERIFICA ONDE ESTE RETÂNGULO APARECE NO MAPA!
					al_draw_rectangle(890.0f, 200.0f, 890.0f + 210.0f, 200.0f + 120.0f, cor_debug, 2.0f);
					// 6. Terminal 3 (Algoritmo)
					al_draw_rectangle(1000.0f, 460.0f, 1000.0f + 300.0f, 460.0f + 115.0f, cor_debug, 2.0f);
					// 7. Porta 
					al_draw_rectangle(1180.0f, 200.0f, 1180.0f + 100.0f, 200.0f + 160.0f, cor_debug, 2.0f);
					// 8. Caixa do Jogador (Verde)
					al_draw_rectangle(jogador.x, jogador.y, jogador.x + jogador.largura, jogador.y + jogador.altura, al_map_rgb(0, 255, 0), 2.0f);
					*/

					float brilho = (float)(sin(al_get_time() * 6.0) + 1.0) * 0.5f; 
					ALLEGRO_COLOR cor_alerta = al_map_rgb(255, (int)(255 * brilho), 0); 

					// Se o Enigma 1 (ALU) não estiver resolvido
					if (!alu.resolvido) {
						al_draw_rectangle(400.0f, 317.0f, 400.0f + 410.0f, 317.0f + 146.0f, cor_alerta, 3.0f);
						al_draw_text(fonte, cor_alerta, 400.0f + (385.0f / 2.0f), 290.0f, ALLEGRO_ALIGN_CENTRE, "[ ALU ]");
					}

					// Se o Enigma 2 (Registradores) não estiver resolvido
					if (!reg.resolvido) {
						al_draw_rectangle(864.0f, 210.0f, 864.0f + 246.0f, 210.0f + 130.0f, cor_alerta, 3.0f);
						al_draw_text(fonte, cor_alerta, 890.0f + (210.0f / 2.0f), 172.0f, ALLEGRO_ALIGN_CENTRE, "[ REGISTRADORES ]");
					}

					// Se o Enigma 3 (Algoritmo) não estiver resolvido
					if (!alg.resolvido) {
						al_draw_rectangle(1000.0f, 480.0f, 1000.0f + 200.0f, 480.0f + 210.0f, cor_alerta, 3.0f);
						al_draw_text(fonte, cor_alerta, 1000.0f + (300.0f / 2.0f), 432.0f, ALLEGRO_ALIGN_CENTRE, "[ ALGORITMO ]");
					}

					if (alu.resolvido && reg.resolvido && alg.resolvido) {
						float brilho_porta = (float)(sin(al_get_time() * 6.0) + 1.0) * 0.5f;
						ALLEGRO_COLOR cor_porta = al_map_rgb(0, (int)(255 * brilho_porta), 255);
						al_draw_rectangle(1180.0f, 200.0f, 1180.0f + 100.0f, 200.0f + 160.0f, cor_porta, 3.0f);
						al_draw_text(fonte, cor_porta, 1180.0f + (100.0f / 2.0f), 172.0f, ALLEGRO_ALIGN_CENTRE, "[ SAIDA ]");
					}

					float sx = jogador.frame_atual * jogador.tamanho_frame;
					float sy = jogador.direcao * jogador.tamanho_frame;
					al_draw_scaled_bitmap(jogador.spritesheet, sx, sy, jogador.tamanho_frame, jogador.tamanho_frame, jogador.x, jogador.y, jogador.largura, jogador.altura, 0);
					
					float margem = 25.0f;

					bool perto_alu = !alu.resolvido && (jogador.x < 400.0f + 385.0f + margem && jogador.x + jogador.largura > 400.0f - margem &&
						jogador.y < 317.0f + 146.0f + margem && jogador.y + jogador.altura > 317.0f - margem);

					bool perto_reg = !reg.resolvido && (jogador.x < 890.0f + 210.0f + margem && jogador.x + jogador.largura > 890.0f - margem &&
						jogador.y < 200.0f + 120.0f + margem && jogador.y + jogador.altura > 200.0f - margem);

					bool perto_alg = !alg.resolvido && (jogador.x < 1000.0f + 300.0f + margem && jogador.x + jogador.largura > 1000.0f - margem &&
						jogador.y < 460.0f + 115.0f + margem && jogador.y + jogador.altura > 460.0f - margem);

					bool perto_porta = (alu.resolvido && reg.resolvido && alg.resolvido) &&
						(jogador.x < 1180.0f + 100.0f + margem && jogador.x + jogador.largura > 1180.0f - margem &&
							jogador.y < 200.0f + 160.0f + margem && jogador.y + jogador.altura > 200.0f - margem);

					if (perto_alu || perto_reg || perto_alg) {
						al_draw_filled_rectangle(LARGURA_TELA / 2 - 220, 560, LARGURA_TELA / 2 + 220, 620, al_map_rgba(0, 0, 0, 200));
						al_draw_rectangle(LARGURA_TELA / 2 - 220, 560, LARGURA_TELA / 2 + 220, 620, al_map_rgb(0, 255, 255), 2.0f);
						al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, 578, ALLEGRO_ALIGN_CENTRE, "Pressione [ E ] para interagir com o Enigma");
					}
					else if (perto_porta) {
						al_draw_filled_rectangle(LARGURA_TELA / 2 - 220, 560, LARGURA_TELA / 2 + 220, 620, al_map_rgba(0, 0, 0, 200));
						al_draw_rectangle(LARGURA_TELA / 2 - 220, 560, LARGURA_TELA / 2 + 220, 620, al_map_rgb(0, 255, 255), 2.0f);
						al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, 578, ALLEGRO_ALIGN_CENTRE, "Pressione [ E ] para abrir a porta e escapar!");
					}

					break;
				
				// ENIGMA 1
				case ESTADO_ENIGMA1:
					al_clear_to_color(al_map_rgb(20, 30, 40));
					al_draw_text(fonte, al_map_rgb(0, 255, 255), LARGURA_TELA / 2, 20, ALLEGRO_ALIGN_CENTRE, "DECODIFICADOR ALU - SOMA BINARIA");
					al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, 55, ALLEGRO_ALIGN_CENTRE, "COMO FUNCIONA O SISTEMA BINARIO:");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 75, ALLEGRO_ALIGN_CENTRE, "- Cada bloco representa uma potencia de 2 (da esquerda para a direita: 128 ate 1).");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 95, ALLEGRO_ALIGN_CENTRE, "- Quando o bloco esta LIGADO (1), somamos o seu valor decimal correspondente.");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 115, ALLEGRO_ALIGN_CENTRE, "- Exemplo: Para formar o 5, ligue os blocos 4 e 1 (4 + 1 = 5).");

					char operacao[64];
					sprintf_s(operacao, sizeof(operacao), "RESOLVA A SOMA: %d + %d = %d", alu.num1, alu.num2, alu.num1 + alu.num2);
					al_draw_text(fonte, al_map_rgb(255, 255, 0), LARGURA_TELA / 2, 155, ALLEGRO_ALIGN_CENTRE, operacao);

					float x = LARGURA_TELA / 2 - 220;
					float y = 200;
					float largura_bit = 45;
					float altura_bit = 60;

					for (int i = 0; i < 8; i++) {
						float bit_x = x + (i * 55);

						ALLEGRO_COLOR cor_caixa = (alu.bits[i] == 1) ? al_map_rgb(0, 200, 100) : al_map_rgb(80, 90, 100);
						al_draw_filled_rectangle(bit_x, y, bit_x + largura_bit, y + altura_bit, cor_caixa);
						al_draw_rectangle(bit_x, y, bit_x + largura_bit, y + altura_bit, al_map_rgb(255, 255, 255), 2.0f);

						char bit[4];
						sprintf_s(bit, sizeof(bit), "%d", alu.bits[i]);
						al_draw_text(fonte, al_map_rgb(255, 255, 255), bit_x + (largura_bit / 2), y + 20, ALLEGRO_ALIGN_CENTRE, bit);

						int valor_pos = 128 >> i;
						char valor_str[8];
						sprintf_s(valor_str, sizeof(valor_str), "%d", valor_pos);
						al_draw_text(fonte, al_map_rgb(255, 165, 0), bit_x + (largura_bit / 2), y + 68, ALLEGRO_ALIGN_CENTRE, valor_str);
					}

					if (alu.erro) {
						al_draw_text(fonte, al_map_rgb(255, 60, 60), LARGURA_TELA / 2, 310, ALLEGRO_ALIGN_CENTRE, "RESPOSTA INCORRETA! Tente novamente (-500 pontos)");
					}
					if (alu.acertou) {
						if (alu.tentativa_atual == 0) {
							al_draw_text(fonte, al_map_rgb(0, 255, 100), LARGURA_TELA / 2, 310, ALLEGRO_ALIGN_CENTRE, "CORRETO! +1000 pontos de primeira!");
						}
						else {
							al_draw_text(fonte, al_map_rgb(0, 255, 100), LARGURA_TELA / 2, 310, ALLEGRO_ALIGN_CENTRE, "CORRETO!");
						}
					}

					float botao_x = LARGURA_TELA / 2 - 100;
					float botao_y = 350;
					float botao_largura = 200;
					float botao_altura = 50;

					al_draw_filled_rectangle(botao_x, botao_y, botao_x + botao_largura, botao_y + botao_altura, al_map_rgb(0, 120, 200));
					al_draw_rectangle(botao_x, botao_y, botao_x + botao_largura, botao_y + botao_altura, al_map_rgb(255, 255, 255), 2.0f);
					al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, botao_y + 18, ALLEGRO_ALIGN_CENTRE, "CONFIRMAR");
					break;
				
				// ENIGMA 2
				case ESTADO_ENIGMA2:
					al_clear_to_color(al_map_rgb(30, 20, 40));
					al_draw_text(fonte, al_map_rgb(255, 150, 0), LARGURA_TELA / 2, 20, ALLEGRO_ALIGN_CENTRE, "ALOCACAO DE REGISTRADORES - HEXADECIMAL");
					al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, 55, ALLEGRO_ALIGN_CENTRE, "COMO FUNCIONA O SISTEMA HEXADECIMAL E O OBJETIVO:");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 75, ALLEGRO_ALIGN_CENTRE, "1. O sistema hexadecimal usa base 16: digitos 0-9 e letras A-F (A=10, B=11, C=12, D=13, E=14, F=15).");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 95, ALLEGRO_ALIGN_CENTRE, "2. Ignore o prefixo '0x' e compare os valores da esquerda para a direita (Ex: 0x0D e menor que 0x41).");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 115, ALLEGRO_ALIGN_CENTRE, "3. OBJETIVO: Clique no primeiro registrador para selecionar (fica amarelo) e clique noutro para");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 135, ALLEGRO_ALIGN_CENTRE, "   troca-los de lugar, organizando os 4 blocos em ORDEM CRESCENTE (do menor para o maior).");

					al_draw_text(fonte, al_map_rgb(255, 255, 0), LARGURA_TELA / 2, 175, ALLEGRO_ALIGN_CENTRE, "ORGANIZE OS REGISTRADORES ABAIXO:");

					float largura_slot = 100;
					float altura_slot = 60;
					float inicio_x = LARGURA_TELA / 2 - 230;
					float y_slots = 210;

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
						al_draw_text(fonte, al_map_rgb(255, 60, 60), LARGURA_TELA / 2, 300, ALLEGRO_ALIGN_CENTRE, "RESPOSTA INCORRETA! Tente novamente (-500 pontos)");
					}
					if (reg.acertou) {
						if (reg.tentativa_atual == 0) {
							al_draw_text(fonte, al_map_rgb(0, 255, 100), LARGURA_TELA / 2, 300, ALLEGRO_ALIGN_CENTRE, "CORRETO! +1000 pontos de primeira!");
						}
						else {
							al_draw_text(fonte, al_map_rgb(0, 255, 100), LARGURA_TELA / 2, 300, ALLEGRO_ALIGN_CENTRE, "CORRETO!");
						}
					}

					float b_x = LARGURA_TELA / 2 - 100;
					float b_y = 340;
					float b_largura = 200;
					float b_altura = 50;

					al_draw_filled_rectangle(b_x, b_y, b_x + b_largura, b_y + b_altura, al_map_rgb(0, 150, 100));
					al_draw_rectangle(b_x, b_y, b_x + b_largura, b_y + b_altura, al_map_rgb(255, 255, 255), 2.0f);
					al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, b_y + 18, ALLEGRO_ALIGN_CENTRE, "CONFIRMAR");
					break;

				// ENIGMA 3
				case ESTADO_ENIGMA3:
					al_clear_to_color(al_map_rgb(20, 20, 35));
					al_draw_text(fonte, al_map_rgb(0, 200, 255), LARGURA_TELA / 2, 15, ALLEGRO_ALIGN_CENTRE, "ANALISE DE ALGORITMO - VETORES E LOOPS");
					al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, 45, ALLEGRO_ALIGN_CENTRE, "EXPLICACAO DIDATICA:");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 65, ALLEGRO_ALIGN_CENTRE, "1. Em C, os vetores comecam no indice 0. As posicoes sao: 0, 1, 2, 3 e 4.");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 85, ALLEGRO_ALIGN_CENTRE, "2. O vetor tem 5 elementos: {10, 20, 30, 40, 50}.");
					al_draw_text(fonte, al_map_rgb(200, 200, 200), LARGURA_TELA / 2, 105, ALLEGRO_ALIGN_CENTRE, "3. Para visitar todos sem falhar nenhum, 'i' deve ir de 0 ate 4 (5 voltas no total).");
					al_draw_text(fonte, al_map_rgb(255, 255, 0), LARGURA_TELA / 2, 125, ALLEGRO_ALIGN_CENTRE, "OBJETIVO: Preencha o espaco (___) com o numero total de iteracoes (5).");
					al_draw_text(fonte, al_map_rgb(100, 255, 100), LARGURA_TELA / 2, 155, ALLEGRO_ALIGN_CENTRE, alg.codigo_linha1);
					al_draw_text(fonte, al_map_rgb(100, 255, 100), LARGURA_TELA / 2, 175, ALLEGRO_ALIGN_CENTRE, alg.codigo_linha2);
					al_draw_text(fonte, al_map_rgb(100, 255, 100), LARGURA_TELA / 2, 195, ALLEGRO_ALIGN_CENTRE, alg.codigo_linha3);

					float caixa_x = LARGURA_TELA / 2 - 100;
					float caixa_y = 225;
					float caixa_largura = 200;
					float caixa_altura = 45;

					al_draw_filled_rectangle(caixa_x, caixa_y, caixa_x + caixa_largura, caixa_y + caixa_altura, al_map_rgb(50, 60, 80));
					al_draw_rectangle(caixa_x, caixa_y, caixa_x + caixa_largura, caixa_y + caixa_altura, al_map_rgb(255, 255, 255), 2.0f);

					char buffer_input[64];
					sprintf_s(buffer_input, sizeof(buffer_input), "%s", alg.input_jogador);
					al_draw_text(fonte, al_map_rgb(255, 255, 255), LARGURA_TELA / 2, caixa_y + 12, ALLEGRO_ALIGN_CENTRE, buffer_input);

					if (alg.erro) {
						al_draw_text(fonte, al_map_rgb(255, 60, 60), LARGURA_TELA / 2, 280, ALLEGRO_ALIGN_CENTRE, "RESPOSTA INCORRETA! Tente novamente (-500 pontos)");
					}
					if (alg.acertou) {
						if (alg.tentativa_atual == 0) {
							al_draw_text(fonte, al_map_rgb(0, 255, 100), LARGURA_TELA / 2, 280, ALLEGRO_ALIGN_CENTRE, "CORRETO! +1000 pontos de primeira!");
						}
						else {
							al_draw_text(fonte, al_map_rgb(0, 255, 100), LARGURA_TELA / 2, 280, ALLEGRO_ALIGN_CENTRE, "CORRETO!");
						}
					}

					al_draw_text(fonte, al_map_rgb(180, 180, 180), LARGURA_TELA / 2, 320, ALLEGRO_ALIGN_CENTRE, "Prima ENTER para confirmar a resposta");
					break;

				default:
					break;
			}

			al_flip_display();
		}
	}

	al_destroy_bitmap(mapa_fase1);
	al_destroy_bitmap(jogador.spritesheet);
	al_destroy_font(fonte);
	al_destroy_event_queue(fila_eventos);
	al_destroy_timer(timer);
	al_destroy_display(display);

	return 0;
}