let storedScores = [];
let gameState = {
 score: 0,
 highScore: 0,
 gamesPlayed: 0,
 sessionStartTime: Date.now()
};
let cameraStream = null;
let serialPort = null;
let serialReader = null;
let currentDifficulty = "Unknown";
let serialBuffer = "";

function cleanSerialText(s){
 return s.replace(/[^\x09\x0A\x0D\x20-\x7E]/g, "").trim();
}

function addLogEntry(msg){
 const container = document.getElementById('activityLog');
 const entry = document.createElement('div');
 entry.className = 'log-entry';
 const timeDiv = document.createElement('div');
 timeDiv.className = 'log-time';
 timeDiv.textContent = new Date().toLocaleTimeString();
 const msgDiv = document.createElement('div');
 msgDiv.textContent = msg;
 entry.appendChild(timeDiv);
 entry.appendChild(msgDiv);
 container.prepend(entry);
}

function updateDateTime(){
 const now = new Date();
 document.getElementById('sessionDate').textContent = `Session: ${now.toLocaleDateString()} ${now.toLocaleTimeString()}`;
}
setInterval(updateDateTime,1000);
updateDateTime();

function updateSessionTime(){
 const elapsedSec = Math.floor((Date.now() - gameState.sessionStartTime) / 1000);
 document.getElementById('sessionTime').textContent = `${Math.floor(elapsedSec/60)}:${(elapsedSec%60).toString().padStart(2,'0')}`;
}
setInterval(updateSessionTime,1000);
updateSessionTime();

function updateScore(n){
 gameState.score = n;
 document.getElementById('currentScore').textContent = n;
 if(n > gameState.highScore){
 gameState.highScore = n;
 document.getElementById('highScore').textContent = n;
 addLogEntry(`New high score: ${n}!`);
 }
}

function resetScore(){
 gameState.score = 0;
 updateScore(0);
 addLogEntry('Score reset');
}

function storeCurrentScore(){
 const patientName = document.getElementById('patientName').value || "Unknown";
 const entry = {
 score: gameState.score,
 time: Date.now(),
 patient: patientName,
 rounds: gameState.gamesPlayed || 0,
 difficulty: currentDifficulty || "Unknown"
 };
 storedScores.push(entry);
 localStorage.setItem('pg_storedScores', JSON.stringify(storedScores));
 renderStoredScores();
 addLogEntry(`Stored score for ${entry.patient} — Score: ${entry.score}, Rounds: ${entry.rounds}, Difficulty: ${entry.difficulty}`);
}

function renderStoredScores(){
 const el = document.getElementById('storedScores');
 if(storedScores.length === 0){
 el.innerHTML = '<div>No scores stored yet.</div>';
 return;
 }
 el.innerHTML = storedScores.map(s => `
 <div class="log-entry">
 <div class="log-time">${new Date(s.time).toLocaleString()}</div>
 <div><strong>Patient:</strong> ${s.patient || "Unknown"}</div>
 <div><strong>Score:</strong> ${s.score}</div>
 <div><strong>Rounds played:</strong> ${s.rounds ?? "N/A"}</div>
 <div><strong>Difficulty:</strong> ${s.difficulty || "Unknown"}</div>
 </div>
 `).join('');
}

function exportScoresPdf(){
 if(!storedScores.length){
 alert("No scores to export.");
 return;
 }
 if(!window.jspdf){
 alert("PDF library not loaded.");
 return;
 }
 const { jsPDF } = window.jspdf;
 const doc = new jsPDF();
 let y = 10;
 doc.setFontSize(16);
 doc.text("Patient Game Log - Stored Scores", 10, y);
 y += 8;
 doc.setFontSize(11);
 storedScores.forEach((s, idx) => {
 if(y > 270){
 doc.addPage();
 y = 10;
 }
 const timeStr = new Date(s.time).toLocaleString();
 doc.text(`Entry ${idx+1}:`, 10, y); y += 5;
 doc.text(`Patient: ${s.patient || "Unknown"}`, 12, y); y += 5;
 doc.text(`Score: ${s.score}`, 12, y); y += 5;
 doc.text(`Rounds played: ${s.rounds ?? "N/A"}`, 12, y); y += 5;
 doc.text(`Difficulty: ${s.difficulty || "Unknown"}`, 12, y); y += 5;
 doc.text(`Time: ${timeStr}`, 12, y); y += 7;
 });
 doc.save("patient_scores.pdf");
}

async function startCamera(){
 try{
 if(!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) throw new Error('getUserMedia not supported');
 cameraStream = await navigator.mediaDevices.getUserMedia({video:true, audio:false});
 const video = document.getElementById('cameraFeed');
 video.srcObject = cameraStream;
 await video.play();
 addLogEntry('Camera started');
 }catch(err){
 console.error('Camera error:', err);
 addLogEntry('Camera error: ' + (err.message || err));
 alert('Unable to access camera: ' + (err.message || err));
 }
}

function stopCamera(){
 if(cameraStream){
 cameraStream.getTracks().forEach(t => t.stop());
 cameraStream = null;
 const video = document.getElementById('cameraFeed');
 video.srcObject = null;
 addLogEntry('Camera stopped');
 } else addLogEntry('Camera was not running');
}

function setConnectedUI(connected){
 const ind = document.getElementById('statusIndicator');
 const text = document.getElementById('connectionText');
 const btn = document.getElementById('connectSerialBtn');
 if(connected){
 ind.style.background = '#4caf50';
 text.textContent = 'Connected';
 btn.textContent = 'Disconnect Arduino';
 } else {
 ind.style.background = '#9e9e9e';
 text.textContent = 'Disconnected';
 btn.textContent = 'Connect Arduino';
 }
}

async function connectSerial(){
 try{
 if (!('serial' in navigator)){
 alert('Web Serial API not available in this browser. Use Chrome/Edge/Brave.');
 return;
 }
 if (serialPort){
 await disconnectSerial();
 return;
 }
 try {
 serialPort = await navigator.serial.requestPort();
 } catch(e){
 addLogEntry('User cancelled port selection');
 return;
 }
 await serialPort.open({ baudRate: 9600 });
 setConnectedUI(true);
 addLogEntry('Serial connected — reading from Arduino');
 const textDecoder = new TextDecoderStream();
 serialPort.readable.pipeTo(textDecoder.writable);
 serialReader = textDecoder.readable.getReader();
 while(true){
 const { value, done } = await serialReader.read();
 if (done) break;
 if (!value) continue;
 serialBuffer += value;
 const lines = serialBuffer.split(/\r?\n/);
 serialBuffer = lines.pop() || "";
 lines.forEach(line => {
 const trimmed = line.trim();
 if(trimmed) handleSerialLine(trimmed);
 });
 }
 }catch(err){
 console.error('Serial error', err);
 addLogEntry('Serial error: ' + (err.message || err));
 alert('Serial error: ' + (err.message || err));
 try{ await disconnectSerial(); }catch(e){}
 }
}

async function disconnectSerial(){
 try{
 if(serialReader){ try{ await serialReader.cancel(); }catch(e){} try{ serialReader.releaseLock(); }catch(e){} serialReader = null; }
 if(serialPort){ try{ await serialPort.close(); }catch(e){} serialPort = null; }
 serialBuffer = "";
 }catch(e){ console.warn('Disconnect cleanup failed', e); }
 setConnectedUI(false);
 addLogEntry('Serial disconnected');
}

function handleSerialLine(line){
 const trimmed = line.trim();
 if(!trimmed) return;
 const safe = cleanSerialText(trimmed);
 if(!safe) return;

 const scoreMatch = safe.match(/^Score:\s*(\d+)\s*$/i);
 if(scoreMatch){
 const n = parseInt(scoreMatch[1],10);
 if(!isNaN(n)){
 updateScore(n);
 addLogEntry('Score updated: ' + n);
 }
 return;
 }

 if(safe.indexOf('New Round') !== -1 || safe.indexOf('===') !== -1){
 addLogEntry('🎮 New round started');
 gameState.gamesPlayed = (gameState.gamesPlayed || 0) + 1;
 document.getElementById('gamesPlayed').textContent = gameState.gamesPlayed;
 const innerScore = safe.match(/Score:\s*(\d+)/i);
 if(innerScore){
 const n = parseInt(innerScore[1],10);
 if(!isNaN(n)) updateScore(n);
 }
 return;
 }

 const roundsMatch = safe.match(/^Rounds played:\s*(\d+)\s*$/i);
 if(roundsMatch){
 const r = parseInt(roundsMatch[1],10);
 if(!isNaN(r)){
 gameState.gamesPlayed = r;
 document.getElementById('gamesPlayed').textContent = r;
 addLogEntry('Rounds played: ' + r);
 }
 return;
 }

 if(/^Difficulty selected:/i.test(safe)){
 const m = safe.match(/^Difficulty selected:\s*([A-Z]+)/i);
 if(m){
 currentDifficulty = m[1].toUpperCase();
 addLogEntry('🎯 Difficulty: ' + currentDifficulty);
 } else {
 addLogEntry(safe);
 }
 return;
 }

 if(/watch the pattern/i.test(safe)){
 addLogEntry('👀 Watch the pattern...');
 return;
 }

 if(/your turn/i.test(safe)){
 addLogEntry('⌨️ Your turn to play');
 return;
 }

 if(/select difficulty/i.test(safe)){
 addLogEntry('🎮 Select difficulty');
 return;
 }

 if(/game over/i.test(safe)){
 addLogEntry('🏁 GAME OVER');
 return;
 }

 // incorrect/wrong handling (show incorrect + correct pattern)
 if(/(incorrect|wrong)/i.test(safe)){
 addLogEntry('❌ Incorrect pattern.');
 const bothMatch = safe.match(/correct (pattern|sequence)\s*[:\-]?\s*(.+)/i);
 if(bothMatch && bothMatch[2]){
 addLogEntry('🔁 Correct pattern: ' + bothMatch[2].trim());
 }
 return;
 }

 // explicit "correct pattern/sequence" lines
 const correctPatternMatch = safe.match(/correct (pattern|sequence)\s*[:\-]?\s*(.+)/i);
 if(correctPatternMatch && correctPatternMatch[2]){
 addLogEntry('🔁 Correct pattern: ' + correctPatternMatch[2].trim());
 return;
 }

 // success messages (plain "correct")
 if(/\bcorrect\b/i.test(safe)){
 addLogEntry('✅ Correct!');
 return;
 }

 if(/^Step\s+\d+/i.test(safe)){
 addLogEntry('📍 ' + safe);
 return;
 }

 if(/^LED\s+\d+/i.test(safe)){
 addLogEntry('💡 ' + safe);
 return;
 }

 if(/^PATTERN_STEP:/i.test(safe)){
 addLogEntry('🔵 ' + safe.replace('PATTERN_STEP:', 'Pattern:'));
 return;
 }

 if(safe.length === 0 || /^[=\-|]+$/.test(safe)){
 return;
 }

 addLogEntry(safe);
}

function switchTab(target){
 document.querySelectorAll('.tab-btn').forEach(b=>b.classList.remove('active'));
 document.querySelectorAll('.tab-content').forEach(s=>s.classList.remove('active'));
 document.querySelector(`[data-target="${target}"]`)?.classList.add('active');
 document.getElementById(target)?.classList.add('active');
}

function init(){
 document.getElementById('connectSerialBtn').addEventListener('click', connectSerial);
 document.getElementById('resetScoreBtn').addEventListener('click', resetScore);
 document.getElementById('storeScoreBtn').addEventListener('click', storeCurrentScore);
 document.getElementById('exportPdfBtn').addEventListener('click', exportScoresPdf);
 document.getElementById('startCameraBtn').addEventListener('click', startCamera);
 document.getElementById('stopCameraBtn').addEventListener('click', stopCamera);
 document.querySelectorAll('.tab-btn').forEach(btn=>btn.addEventListener('click', ()=>switchTab(btn.dataset.target)));

 const modal = document.getElementById('instructionModal');
 const closeBtn = document.getElementById('closeInstructionBtn');
 const showBtn = document.getElementById('showInstructionsBtn');

 if(closeBtn){
 closeBtn.addEventListener('click', ()=>{ modal.style.display = 'none'; });
 }
 if(showBtn){
 showBtn.addEventListener('click', ()=>{ modal.style.display = 'flex'; });
 }

 try{
 const raw = localStorage.getItem('pg_storedScores');
 if(raw) storedScores = JSON.parse(raw);
 }catch(e){
 storedScores = [];
 }

 renderStoredScores();
 updateDateTime();
 updateSessionTime();
 addLogEntry('App initialized — connect your Arduino to use real scores.');
}

window.addEventListener('DOMContentLoaded', init);
