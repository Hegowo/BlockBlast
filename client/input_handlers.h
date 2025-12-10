#ifndef INPUT_HANDLERS_H
#define INPUT_HANDLERS_H

void handle_menu_click(void);
void handle_options_click(void);
void handle_login_click(void);
void handle_multi_choice_click(void);
void handle_join_input_click(void);
void handle_lobby_click(void);
void handle_lobby_right_click(void);
void handle_server_browser_click(void);
void handle_server_browser_scroll(int direction);

void handle_game_click(int is_multi);
void handle_game_mousedown(int is_multi);

void process_network(void);

#endif
