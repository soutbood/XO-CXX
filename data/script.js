let turn = 1;
let x;
let player_id = -1;
let player_ip = -1;
let oppenent_id = -1;
let cond = 1;
let very = 0;
let m_very = 0;
let game_id = -1;
var Socket;


function accept(){
    hideModals();
    Socket.send(JSON.stringify({'player':oppenent_id,'very':m_very,'decision':122}));
}

function reject(){
    hideModals();
    Socket.send(JSON.stringify({'player':oppenent_id,'very':m_very,'decision':121}));
}

function sendrequest(){
  var message = document.getElementById("message").value;
  request(message);
}

function request(info){
  very = Math.floor(Math.random()*100+1);
  Socket.send(JSON.stringify({'request':info,'very':very}));
  showModal('#modalOverlay-4');
  console.log(very);
}

function createName(){
  var username = document.getElementById("username").value;
  if(username.length > 30){
    alert("name over 30 not acceptable please try again");
    document.getElementById("username").innerHTML = "";
    return;
  }
  var message = {'name':username,'ip':player_ip};
  Socket.send(JSON.stringify(message));
  console.log(message);
}

function cell_condition(cell_web){
  if(turn%2==cond){
    Socket.send(JSON.stringify({'player':player_id,'oppenent':oppenent_id,'game':game_id,'turn':turn,'cell':cell_web}));  
  }
}

function cell_action(cell_esp32){
  if(turn%2==1){
    x='X'
  } else {
    x='O'
  }
  if(document.getElementById(cell_esp32).innerHTML!=''){
    document.getElementById(cell_esp32).innerHTML = document.getElementById(cell_esp32).innerHTML;
  }else{
    document.getElementById(cell_esp32).innerHTML = x;
    turn=turn+1;
  }
}

function reset(){
  turn = 1;
  document.getElementById('1').innerHTML = '';
  document.getElementById('2').innerHTML = '';
  document.getElementById('3').innerHTML = '';
  document.getElementById('4').innerHTML = '';
  document.getElementById('5').innerHTML = '';
  document.getElementById('6').innerHTML = '';
  document.getElementById('7').innerHTML = '';
  document.getElementById('8').innerHTML = '';
  document.getElementById('9').innerHTML = '';
  send_once();
  game_id = -1;
}

function init(){
  Socket = new WebSocket('ws://' + window.location.hostname + '/ws');
  Socket.onmessage = function(event){
    processCommand(event);
  }
}

function send_once(){
  Socket.send(JSON.stringify({'rst': 125}));
}

function processCommand(event){
  var obj = JSON.parse(event.data);
  console.log(very);
  console.log(JSON.stringify(obj));
  if(obj.not_list){
    console.log("is not a list");
  }
  else{
    cardShow(JSON.stringify(obj));
  }
  if(obj.very){
    console.log(obj.very);
  }
  if(obj.request){
    oppenent_id = obj.request; // the id of the player wanting to play aka oppenent of the person recieving 
    m_very = obj.very;
    showModal('#joinRequestTitle');  // show the request modal to oppenent to play 
    return;
  }
  if(obj.decision < 0){
    hideModals();
    alert("game rejected");
  }
  if(obj.decision > 0){
    hideModals();
    if(obj.id = player_id){}
    else{
      alert("game accepted Start");
    }
  }

  if(obj.init){
    player_id = obj.id;
    player_ip = obj.ip;
    
  }
  if(obj.game){
    hideModals();
    game_id = obj.game;
  }
  if(obj.id)
  {
    player_id   = obj.id;
    oppenent_id = obj.oppenent;
    cond=0;
    if(very==obj.very){
    player_id   = obj.oppenent;
    oppenent_id = obj.id;
    cond=1;
    }
  }
  if(obj.eror)
  {
    alert(obj.eror);
  }
  else if(obj.play==0)
  {
    alert('wrong move');
  }
  else
  {
    cell_action(obj.cell);
  }
  if(obj.won=='D')
  {
    showModal('#draw-h3');
    reset();
  }
  else if(obj.won=='X')
  {
    if(cond==1){
      showModal('#win-h3');
    }
    else {
      showModal('#lose-h3');
    }
    reset();
  }
  else if(obj.won=='O')
  {
    if(cond==0){
      showModal('#win-h3');
    }
    else {
      showModal('#lose-h3');
    }
    reset();
  }  
}

window.onload = function(event){
  init();
}

// --- Game Status and Players' Data ---
// const onlinePlayers = [{'name':'ahmed','id':10,'ip':'192.168.0.193'},{'name':'ali','id':11,'ip':'192.168.0.193'}];
const onlineNames = [];
// --- For Testing ---

function cardShow(players) {
    const playersContainer = document.querySelector('.players-list');
    if (!playersContainer) return;
    if (!players) return;
    if(players.not_list) return;
    playersContainer.innerHTML = "";

    const data = (typeof players === 'string') ? JSON.parse(players) : players;
    while(onlineNames.length > 0) {
        onlineNames.pop();
    }
    data.forEach(player => {
      if(player.ip == player_ip){ 
        console.log(player_ip);
      }
      else{
        if(player.name){
          onlineNames.push((player.name).toString()); 
          playersContainer.innerHTML += `
          <div class="player-item" data-id="${player.user_id}">
          <span>Name : <b>"${player.name}"</b></span>
          <span>ID : <b>(${player.client_id})</b></span>
          <button class="play-btn" onclick="request('${player.ip}')">Play</button>
          </div>
          `;
        }
      else{
          onlineNames.push("Player " + (player.user_id).toString()); 
          playersContainer.innerHTML += `
          <div class="player-item" data-id="${player.user_id}">
          <span>Name : <b>"Player ${player.user_id}"</b></span>
          <span>ID : <b>(${player.client_id})</b></span>
          <button class="play-btn" onclick="request('${player.ip}')">Play</button>
          </div>
          `;
        }
    }
    });
    console.log(onlineNames.toString());
}



let currentTurn = "X";

// --- DOM References ---
const playersContainer = document.querySelector('.players-list');
if (playersContainer) {
    playersContainer.innerHTML = "";
}

const modals = document.querySelectorAll('.modal-overlay');

// --- Modal Helper Functions ---
function hideModals() {
    modals.forEach(modal => {
        modal.style.display = 'none';
    });
}

function showModal(selector) {
    hideModals();
    // Searches through all modals to find the one containing the unique header ID
    modals.forEach(modal => {
        if (modal.matches(selector) || modal.querySelector(selector)) {
            modal.style.display = 'flex';
        }
    });
}

// Hide all modals immediately on load
hideModals();
