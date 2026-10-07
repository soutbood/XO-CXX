// ---------------------------------------------------------------------------------------

#include <WiFi.h>  
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
// for ram checking
#include "esp_chip_info.h"
#include "esp_system.h"

// SSID and password of Wifi connection:
const char *ssid = "";
const char *password = "";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

SET_LOOP_TASK_STACK_SIZE( 32*1024 ); // 32KB

class Player {
public:
  bool p_state;         // connected or not
  int p_client_id;             // client id
  int p_game_id;;       // game id currently in
  int p_limit;
  int p_stack_id;
  String p_name;
  String p_ip;
  Player() {
    p_state = false;
    p_ip = "";
    p_name = "";
    p_client_id = -1;
    p_game_id = -1;
    p_limit = 0;
    p_stack_id = -1;
  }
  ~Player(){}
  void set_p_state(bool state);
  void to_json(JsonVariant doc);
  void print_p_debuging();
  void set_p_ip(String ip);
  void set_p_client_id(int id);
  void set_p_stack_id(int id);
  void set_p_game_id(int game_id);
  void set_p_limit(int limit);
  void set_p_name(String name);
  bool is_alive();
  void p_transfer(int client_id);
  int get_p_client_id();
  int get_p_stack_id();
  int get_p_game_id();
  int get_p_limit();
  String get_p_ip();
  String get_p_name();
  void p_reset();
};

class Game {
public:
  bool current_player;
  int move;
  int g_color;        // 1 is X ,2 is O
  int g_cell;         // from 1 to 9 when used do g_cell-1
  int g_play;
  char board[9];
  int g_game_id;

  Player g_player;    // g_player is the one who send the request
  Player g_oppenent;  // g_oppenent is the oppenent

  Game() {
    for (int i = 0; i < 9; i++)
      board[i] = '-';
    move = 0;
    current_player = true;
    g_oppenent = Player();
    g_player = Player();
    g_color = -1;
    g_cell = -1;
    g_play = 1;
    g_game_id = -1;
  }
  ~Game(){}
  void print_debug();
  void print_board();
  void change_turn();
  void input_user();
  char is_winning();
  void print_received();
  void game_init(JsonDocument doc, int player_id, int oppenent_id);
  void game_init(JsonDocument doc);
  void game_loop();
  void set_play(int x);
  void set_cell(int x);
  void set_color(int id);
  void set_player(int id);
  void set_oppenent(int id);
  void set_game_id(int game_id);
  int get_play();
  int get_cell();
  int get_oppenent_client_id(); 
  int get_player_client_id();
  int get_color();
  int get_move();
  String get_oppenent_ip();
  String get_player_ip();

};

const int capacity = 50;
Player ip[capacity];
Player d_player, d_oppenent;
Game game[20];
int m_game_id = 0;

//                   STACK SECTION
// -----------------------------------------------------------

int stack[capacity] = {}; //50
int connection[capacity] = {}; // the value is the client id of the user  
int top = 0;

void stack_init(){
    for(int i=0;i<capacity;i++)
        stack[i]=top++;
      Serial.println("Top: "+ String(top));
}

void connection_init(){
    for(int i=0;i<capacity;i++)
        connection[i]=i;
}

void connection_print(){
    for(int i=0;i<capacity;i++)
        Serial.print(String(connection[i]) + ",");
        Serial.println();
}


void stack_print(){
    for(int i=0;i<capacity;i++)
        Serial.print(String(stack[i]) + ",");
  Serial.println();
}

void stack_push(int &exit, int id){
    if(top < capacity){
        Serial.println("User with id "+ String(id)+ " is leaving entry number " + String(exit));
        stack[top] = exit;
        exit = -1;
        top++;
    }
    else {
        Serial.println("Stack overflow");
    }
}

void stack_pop(int &entry, int id){
    if(top > 0){
        top--;
        Serial.println("user with id " + String(id) + " is assigned to entry number " + String(stack[top]));
        entry = stack[top];
        stack[top] = -1;
    }
    else {
      Serial.println("Stack underflow");
    }
}

// ----------------------------------------------------------

void current_alive(AsyncWebSocket *server){
      String list = ""; // create the container
      JsonDocument list_array; // the document
      JsonArray array = list_array.to<JsonArray>();
      JsonObject obje[50] = {};  
      int obje_counter = 0;
      for(int i=0;i<50;i++) 
        if(ip[i].is_alive()){
          obje[obje_counter] = array.add<JsonObject>();
          ip[i].to_json(obje[obje_counter++]);
        }
      serializeJson(list_array, list); // stuff the document into the container
      server->textAll(list);
      Serial.println("info: " + list);
      
}

void d_init(){
for (int i = 0 ; i < 20 ; i++)
    game[i] = Game();
}


void wsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  int client_id = client->id();
  int num;
  int class_no = -1;
  bool found=false;
  for(int i=0;i<capacity;i++){ // if client is registered then the num will be the registered number in the stack
    if(connection[i] == client_id){
        client_id = i;
        found = true;
        i = capacity;
      }
  }
  if(found){ // client_id is actually a user id not the client id 
    num = client_id;
    ip[num].set_p_ip(client->remoteIP().toString());
    ip[num].set_p_client_id(connection[num]);
    for (int i = 0; i < 50; i++){
      if (ip[i].is_alive()){
        if (ip[num].get_p_ip() == ip[i].get_p_ip() && i != num) {
          String json = "";
          JsonDocument js;
          JsonObject obj = js.to<JsonObject>();
          if (ip[i].get_p_limit() < 5) {
            obj["eror"] = "You cannot play in multiple browsers on a device";
            obj["not_list"] = 1;
            serializeJson(js, json);
            Serial.println(json);
            server->text(num, json);
          }
          ip[i].set_p_ip("");
          ip[i].set_p_limit(ip[i].get_p_limit() + 1);
          client->close(i);
        }
      }
    }
  }
      String json_init = "";
      JsonDocument js_init;
      JsonObject obj_init = js_init.to<JsonObject>();
      
  switch (type) {                                                 // switch on the type of information sent
    case WS_EVT_DISCONNECT:                                       // if a client is disconnected, then type == WStype_DISCONNECTED
      Serial.println("Client (" + String(client->id()) + ") disconnected");
      stack_print();
      ip[client_id] = Player();
      connection_print();
      Serial.println(String(connection[client_id])+ " : " + String(client->id()));
      stack_push(client_id,client->id());                   //stack location that was taken ,client id
      Serial.println(String(connection[client_id])+ " : " + String(client->id()));
      connection_print();
      stack_print();
      current_alive(server);
      break;
    case WS_EVT_CONNECT:                                          // if a client is connected, then type == WStype_CONNECTED
      Serial.println("Client (" + String(client_id) + ") connected");
      stack_print();
      connection_print();
      Serial.println(String(class_no)+" : "+String(client_id));
      stack_pop(class_no,client_id);
      connection[class_no] = client_id;
      stack_print();
      connection_print();
      Serial.println(String(class_no)+" : "+String(client_id));
      num = class_no;
      ip[num] = Player();
      ip[num].set_p_client_id(client_id);
      ip[num].set_p_stack_id(num);
      ip[num].set_p_ip(client->remoteIP().toString());
      ip[num].set_p_state(true);
      ip[num].print_p_debuging();
      obj_init["init"] = 1;
      obj_init["id"] = client_id;
      obj_init["ip"] = client->remoteIP().toString();
      obj_init["not_list"] = 1;
      serializeJson(js_init,json_init);
      server->text(client_id,json_init);
      current_alive(server);
      Serial.printf("Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);
      break;
    case WS_EVT_DATA:  // if a client has sent data, then type == WStype_TEXT
      AwsFrameInfo *info = (AwsFrameInfo *)arg;
      if (ip[num].get_p_ip() == "") {
        return;
      }
      stack_print();
      if (info->final && info->index == 0 && info->len == len) {
        JsonDocument doc;                              // create a JSON container
        DeserializationError error = deserializeJson(doc, data);  // the recieved JSON
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          return;
        } else {
          int a = (ip[num].get_p_game_id() == -1 ? -1 : ip[num].get_p_game_id());

          if (a == -1) {
            a = m_game_id % 20;
            m_game_id++;
            ip[num].set_p_game_id(a);
          }

          if (doc["name"])
            if(doc["ip"] == ip[num].get_p_ip()){
              String name = doc["name"];
              if(name.length() > 30){
                Serial.println("Name not acceptable");
                return;
              }
              ip[num].set_p_name(name);
              Serial.print("name added");
              current_alive(server);
              return;
            }

          game[a].game_init(doc);

          if (doc["request"]) {  //user sends ip of oppenent to request a game
            String oppenent_ip = doc["request"].as<String>(); //convert ip to string
            Serial.println("requesting to " + oppenent_ip); //display the oppenent ip
          
            for (int i = 0; i < 50; i++){
              if (ip[i].is_alive()){
                if (ip[i].get_p_ip() == oppenent_ip) {// find the oppenent client id "i == oppenent client id"
                  String json = "";
                  JsonDocument d;
                  JsonObject ob = d.to<JsonObject>(); //create json to send
                  
                  ob["request"]=num;        //send id of player requesting the game
                  ob["very"] =doc["very"];  //send the confirmation that this person is the one sending the game
                  ob["not_list"] = 1;
                  serializeJson(d, json);
                  int cli=ip[i].get_p_client_id();
                  ip[num].print_p_debuging();
                  ip[i].print_p_debuging();
                  server->text(cli, json);    //send to oppenent the id of the player wanting to play 
                  return; //exit 
                }
              }
            }
          }

          if (doc["decision"]) {
            if(doc["decision"]==122){
              int player_id   = doc["player"];            //id of player requesting 
              int oppenent_id = num;                      //id of player accepting the game
              ip[oppenent_id].set_p_game_id(a);           // set the game id to the oppenent
              ip[player_id].set_p_game_id(a);             // set the game id to the player
              ip[oppenent_id].set_p_client_id(connection[oppenent_id]);      // set own id to the oppenent
              ip[player_id].set_p_client_id(connection[player_id]);          // set own id to the player
              game[a].game_init(doc,player_id,oppenent_id);
              game[a].set_game_id(a);

              Serial.println("game ID " + String(a));

              Serial.println("ip of player "   + ip[player_id].get_p_ip());   //ip of player
              Serial.println("ip of oppennet " + ip[oppenent_id].get_p_ip()); //ip of oppenent
              Serial.println("id of player "   + String(connection[player_id]));          //id of player
              Serial.println("id of oppenent " + String(connection[oppenent_id]));        //id of oppenent
              Serial.println("GAME");

              game[a].print_debug();

              String json_accept = "";
              JsonDocument doc_accept;
              JsonObject obj_accept = doc_accept.to<JsonObject>();
              // the json to send to the person requesting the game
              obj_accept["oppenent"] = connection[oppenent_id];               // send oppenent id
              obj_accept["id"] = connection[player_id];                       // send player id
              obj_accept["cond"] = 0;                             // send differentiate between player and oppenent in the website
              obj_accept["game"] = a;                             // send game id
              obj_accept["very"] = doc["very"];                   // send the confirmation to know how is the one that send the game so that cond works 
              obj_accept["decision"] = 1;
              obj_accept["not_list"] = 1;
              serializeJson(doc_accept, json_accept);

              server->text(connection[oppenent_id], json_accept);
              server->text(connection[player_id], json_accept);

              Serial.println(ip[player_id].p_ip + "vs" + ip[oppenent_id].p_ip);
              return;
            }
            else if(doc["decision"]==121){
              int rejected_id = doc["player"];

              String json_refuse = "";
              JsonDocument doc_refuse;
              JsonObject obj_refuse = doc_refuse.to<JsonObject>();
              obj_refuse["id"] = doc["player"];
              obj_refuse["decision"] = -1;
              obj_refuse["not_list"] = 1;
              serializeJson(doc_refuse, json_refuse);

              server->text(connection[rejected_id], json_refuse);
              Serial.println("game rejected");
            }
          }
          if (game[a].get_oppenent_client_id() == -1) {
            String json = "";
            JsonDocument js;
            JsonObject obj = js.to<JsonObject>();
            obj["eror"] = "You cannot play without an oppenent";
            obj["not_list"] = 1;
            serializeJson(js, json);
            Serial.println(json);
            server->text(num, json);
            return;
          }

          Serial.println("Received cell info from user: " + String(connection[num]));
          game[a].print_received();
          String jsonString = "";                    // create a JSON string for sending data to the client
          JsonDocument do1;               // create a JSON container
          JsonObject object = do1.to<JsonObject>();  // create a JSON Object

          game[a].input_user();
          game[a].print_board();
          object["cell"] = game[a].get_cell();
          object["play"] = game[a].get_play();
          object["turn"] = game[a].get_color();
          object["won"] = String(game[a].is_winning());
          if (game[a].get_play() == 0)
            object["cell"] = 0;                            // write data into the JSON object -> I used "rand1" and "rand2" here, but you can use anything else
          object["not_list"] = 1;
          serializeJson(do1, jsonString);                  // convert JSON object to string
          Serial.println(jsonString);                      // print JSON string to console for debug purposes (you can comment this out)
          server->text(game[a].get_player_client_id(), jsonString);  // send JSON string to clients
          Serial.println("sent to player");
          server->text(game[a].get_oppenent_client_id(), jsonString);
          Serial.println("sent to oppenent");
          if (game[a].is_winning() == 'X'
            or game[a].is_winning() == 'O'
            or game[a].is_winning() == 'D')
            game[a] = Game();
            
        }

        Serial.println("");
        break;
      }
  }
}


void setup() {
  Serial.begin(115200);  // init serial port for debugging

  WiFi.begin(ssid, password);                                                    // start WiFi interface
  Serial.println("Establishing connection to WiFi with SSID: " + String(ssid));  // print SSID to the serial interface for debugging

  while (WiFi.status() != WL_CONNECTED) {  // wait until WiFi is connected
    delay(1000);
    Serial.print(".");
  }
  Serial.print("Connected to network with IP address: ");
  Serial.println(WiFi.localIP());  // show IP address that the ESP32 has received from router
  if (!MDNS.begin("esp32")) {
    Serial.println("Error no MDNS yet.");
    while (1) {
      delay(500);
    }
  }
  Serial.println("Connect to esp32.local .");
  stack_init();
  if(!LittleFS.begin(true)){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  d_init();
  Serial.print(sizeof(Player));
  Serial.println();
  Serial.print(sizeof(Game));
  Serial.println();
  Serial.printf("\nLoop() - Free Stack Space: %d\n", uxTaskGetStackHighWaterMark(NULL));
  Serial.printf("Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);


/*
  server.on("/",HTTP_GET,[](AsyncWebServerRequest *request){
      request->send(200,"text/html",webpage);
  });
*/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/style.css" ,"text/css");
  });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/script.js", "applicat/ion/javascript");
  });
  server.on("/back.jpg", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/back.jpg", "image/jpg");
  });

  ws.onEvent(wsEvent);
  server.addHandler(&ws);
  server.begin();
}

void loop() {
  ws.cleanupClients();  // Needed for the webserver to handle all clients
}

void Game::print_board() {
  Serial.print("\n========================================\n");
  Serial.print(" | ");
  Serial.print(board[0]);
  Serial.print(" | ");
  Serial.print(board[1]);
  Serial.print(" | ");
  Serial.print(board[2]);
  Serial.print(" |");
  Serial.print("\n------------\n");
  Serial.print(" | ");
  Serial.print(board[3]);
  Serial.print(" | ");
  Serial.print(board[4]);
  Serial.print(" | ");
  Serial.print(board[5]);
  Serial.print(" |");
  Serial.print("\n------------\n");
  Serial.print(" | ");
  Serial.print(board[6]);
  Serial.print(" | ");
  Serial.print(board[7]);
  Serial.print(" | ");
  Serial.print(board[8]);
  Serial.print(" |");
  Serial.print("\n========================================\n");
}

void Game::input_user() {
  if (board[g_cell - 1] == '-') {
    if (current_player) {
      if (g_color % 2 == 0) {
        board[g_cell - 1] = 'O';
        move++;
        // set_play(1);
      } else {
        if (g_color % 2 == 1) {
          board[g_cell - 1] = 'X';
          move++;
          // set_play(1);
        }
      }
    }
  }
}

void Game::change_turn() {
  current_player = !current_player;
}

char Game::is_winning() {
  if (board[0] == board[1] and board[1] == board[2] and board[0] != '-') {
    move = 9;
    return board[0];
  }
  if (board[3] == board[4] and board[4] == board[5] and board[3] != '-') {
    move = 9;
    return board[3];
  }
  if (board[6] == board[7] and board[7] == board[8] and board[6] != '-') {
    move = 9;
    return board[6];
  }
  if (board[0] == board[3] and board[3] == board[6] and board[0] != '-') {
    move = 9;
    return board[0];
  }
  if (board[1] == board[4] and board[4] == board[7] and board[1] != '-') {
    move = 9;
    return board[1];
  }
  if (board[2] == board[5] and board[5] == board[8] and board[2] != '-') {
    move = 9;
    return board[2];
  }
  if (board[0] == board[4] and board[4] == board[8] and board[0] != '-') {
    move = 9;
    return board[0];
  }
  if (board[2] == board[4] and board[4] == board[6] and board[2] != '-') {
    move = 9;
    return board[2];
  }
  if (move == 9)
    return 'D';
  return 'C';
}

void Game::game_init(JsonDocument doc, int player_id, int oppenent_id) {
  g_oppenent.p_transfer(oppenent_id);
  g_player.p_transfer(player_id);
  g_color    = doc["turn"];
  g_cell     = doc["cell"];
}

void Game::game_init(JsonDocument doc) {
  g_color    = doc["turn"];
  g_cell     = doc["cell"];
}

void Game::print_received() {
  print_debug();
  Serial.println("Color: " + String(g_color));
  Serial.println("Cell: " + String(g_cell));
}

int Game::get_play() {
  return g_play;
}

int Game::get_cell() {
  return g_cell;
}

int Game::get_oppenent_client_id() {
  return g_oppenent.p_client_id;
}

int Game::get_player_client_id() {
  return g_player.p_client_id;
}

int Game::get_color() {
  return g_color;
}

String Game::get_oppenent_ip(){
  return g_oppenent.p_ip;
}
 
String Game::get_player_ip(){
  return g_player.p_ip;
}

void Game::set_color(int id) {
  g_color = id;
}

void Game::set_cell(int x) {
  g_cell = x;
}

void Game::set_play(int x) {
  g_play = x;
}

void Game::set_player(int id) {
  g_player.p_client_id = id;
  g_player.p_ip = ip[id].get_p_ip();
  g_player.p_game_id = g_game_id;
}

void Game::set_oppenent(int id) {
  g_oppenent.p_client_id = id;
  g_oppenent.p_ip = ip[id].get_p_ip();
  g_oppenent.p_game_id = g_game_id;
}

void Game::set_game_id(int game_id){
  g_oppenent.p_game_id = game_id;
  g_player.p_game_id = game_id;
  g_game_id = game_id;
}

void Game::print_debug(){
  Serial.println("Player ");
  g_player.print_p_debuging();    // g_player is the one who send the request
  Serial.println("Oppenent ");
  g_oppenent.print_p_debuging();  // g_oppenent is the oppenent

}

void Game::game_loop() {
  print_board();
  if (move < 9) {
    if (is_winning() == 'd')
      Serial.print("The game ended in a draw");
    else if (is_winning() != 'c')
      Serial.print("The winner is " + is_winning());
    print_board();
  } else {
    input_user();
    change_turn();
    print_board();
    Serial.print("The winner is " + is_winning());
  }
}


int Game::get_move() {
  return move;
}

String Player::get_p_ip() {
  return p_ip;
}

String Player::get_p_name(){
  return p_name;
}

int Player::get_p_client_id() {
  return p_client_id;
}

int Player::get_p_game_id() {
  return p_game_id;
}

int Player::get_p_limit() {
  return p_limit;
}

int Player::get_p_stack_id(){
  return p_stack_id;
}

bool Player::is_alive(){
  if(p_ip == "" || p_ip == "0.0.0.0")
    return false;
  return p_state;
}

void Player::set_p_stack_id(int id){
  p_stack_id = id;
}

void Player::set_p_state(bool state){
  p_state = state;
}

void Player::set_p_ip(String ip) {
  p_ip = ip;
}

void Player::set_p_name(String name){
  p_name = name;
}

void Player::set_p_client_id(int id) {
  p_client_id = id;
}

void Player::set_p_game_id(int game_id) {
  p_game_id = game_id;
}

void Player::set_p_limit(int limit) {
  p_limit = limit;
}

void Player::print_p_debuging(){
  Serial.println();
  Serial.println("ID: " + String(p_client_id) + " game ID: " + String(p_game_id) + " Name: " + p_name + " Ip: " + p_ip + " limit: " + String(p_limit));
}

void Player::to_json(JsonVariant doc){
  doc["name"] = p_name;
  doc["user_id"] = p_stack_id;
  doc["client_id"] = p_client_id;
  doc["ip"] = p_ip; 
}

void Player::p_reset(){
  p_state = false;
  p_ip = "";
  p_name = "";
  p_client_id = -1;
  p_game_id = -1;
  p_limit = 0;
  p_stack_id = -1;
}
void Player::p_transfer(int client_id){
  p_state = ip[client_id].is_alive();
  p_ip = ip[client_id].get_p_ip();
  p_name = ip[client_id].get_p_name();
  p_client_id = ip[client_id].get_p_client_id();
  p_game_id = ip[client_id].get_p_game_id();
  p_limit = 0;
  p_stack_id = ip[client_id].get_p_stack_id(); 
}
